// InterpreterResult.cpp - result<T>, ok(...) e err(...) no REPL, com o mesmo comportamento do GhostResult<T>
// do programa compilado (include/GhostCore.hpp):
//   function result<double> dividir(double a, double b) { if (b == 0) { return err("divisao por zero"); } return ok(a / b); }
//   result<double> r = dividir(1, 0);
//   r.is_ok()  r.is_err()  r.value()  r.value_or(0)  r.error()      print(r) -> err("divisao por zero")
// O tipo declarado converte o valor como o static_cast<T> do C++: result<int> r = ok(3.9); -> ok(3).
#include "../include/Interpreter.hpp"

#include <cctype>
#include <stdexcept>

namespace {
bool startsWith(const std::string& s, const std::string& p) { return s.compare(0, p.size(), p) == 0; }
bool endsWith(const std::string& s, const std::string& p) { return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0; }
std::string innerOf(const std::string& spec) { return spec.substr(7, spec.size() - 8); } // result<T> -> T
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
} // namespace

bool GhostInterpreter::isResultSpec(const std::string& spec) { return startsWith(spec, "result<") && endsWith(spec, ">"); }

bool GhostInterpreter::isSetSpec(const std::string& spec) { return startsWith(spec, "set<") && endsWith(spec, ">"); }

bool GhostInterpreter::isListSpec(const std::string& spec) {
    return startsWith(spec, "list<") && endsWith(spec, ">") && spec != "list<var>";
}

bool GhostInterpreter::isFnSpec(const std::string& spec) { return startsWith(spec, "fn<") && endsWith(spec, ">"); }

bool GhostInterpreter::knownType(const std::string& spec) const {
    return !spec.empty() && (isResultSpec(spec) || isSetSpec(spec) || isListSpec(spec) || isFnSpec(spec) ||
                             class_memory.count(spec) || interface_memory.count(spec));
}

std::string GhostInterpreter::readResultSpec(Lexer& lex, bool ltTaken, const std::string& head) {
    if (!ltTaken && lex.getNextToken().type != GTokenType::Lt) {
        if (head == "fn") throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)>");
        throw std::runtime_error("use " + head + "<tipo>, ex.: " + head + "<int>");
    }
    std::string s = head + "<";
    int depth = 1;
    while (true) {
        Token g = lex.getNextToken();
        if (g.type == GTokenType::EndOfFile || g.type == GTokenType::Semicolon) throw std::runtime_error(head + "<...> sem '>'");
        if (g.type == GTokenType::Lt) depth++;
        else if (g.type == GTokenType::Gt) depth--;
        else if (g.type == GTokenType::Chain) depth -= 2;
        if (g.type == GTokenType::Comma) s += ",";
        else {
            if (!s.empty() && !g.value.empty() && isWordChar(s.back()) && isWordChar(g.value.front())) s += " ";
            s += g.value;
        }
        if (depth <= 0) break;
    }
    if (depth < 0 || s == head + "<>") throw std::runtime_error("tipo invalido: " + s);
    if (head == "fn") return validateAndNormalizeFnSpec(s);
    // Normaliza tipos internos (ex.: int[] -> list<int>, fn<...>, interfaces/classes genericas)
    try {
        const std::string norm = validateAndNormalizeTypeSpec(s, false);
        ensureTypeReady(norm);
        return norm;
    } catch (...) {
        // Pode conter parametro de tipo ativo (ex.: extends Colecao<T> antes da instanciacao)
        if (head == "fn") throw;
    }
    return s;
}

bool GhostInterpreter::parseFnSignature(const std::string& spec, std::string& ret, std::vector<std::string>& params) {
    if (!isFnSpec(spec) || spec.size() < 6) return false;
    const std::string inner = spec.substr(3, spec.size() - 4); // Ret(P1,P2)
    if (inner.empty() || inner.back() != ')') return false;
    // Acha o '(' que casa com o ')' final no nível externo (fora de < > e [ ]).
    int paren = 0;
    size_t openAt = std::string::npos;
    for (size_t k = inner.size(); k-- > 0;) {
        const char c = inner[k];
        if (c == ')') paren++;
        else if (c == '(') {
            if (--paren == 0) { openAt = k; break; }
        }
    }
    if (openAt == std::string::npos || openAt == 0) return false;
    // Confere que openAt está fora de '<...>' do tipo de retorno (ex.: fn<fn<int(int)>(double)> -> openAt é o '(' de (double)).
    int angle = 0;
    for (size_t k = 0; k < openAt; ++k) {
        if (inner[k] == '<') angle++;
        else if (inner[k] == '>') angle--;
    }
    if (angle != 0) return false;
    ret = inner.substr(0, openAt);
    params.clear();
    const std::string plist = inner.substr(openAt + 1, inner.size() - openAt - 2);
    if (plist.empty()) return true;
    std::string cur;
    int a = 0, p = 0, b = 0;
    for (char c : plist) {
        if (c == '<') a++;
        else if (c == '>') a--;
        else if (c == '(') p++;
        else if (c == ')') p--;
        else if (c == '[') b++;
        else if (c == ']') b--;
        else if (c == ',' && a == 0 && p == 0 && b == 0) {
            params.push_back(cur);
            cur.clear();
            continue;
        }
        cur += c;
    }
    params.push_back(cur);
    return true;
}

std::string GhostInterpreter::validateAndNormalizeTypeSpec(const std::string& raw, bool allowVoid) const {
    Lexer lx(raw);
    int pendingGt = 0;
    Token cur = lx.getNextToken();
    auto nextTok = [&]() { cur = lx.getNextToken(); };
    auto expectGt = [&]() {
        if (pendingGt > 0) { pendingGt--; return; }
        if (cur.type == GTokenType::Gt) { nextTok(); return; }
        if (cur.type == GTokenType::Chain) { nextTok(); pendingGt = 1; return; }
        throw std::runtime_error("esperado '>' no tipo generico, encontrado '" + cur.value + "'");
    };
    std::function<std::string(bool)> parseOne = [&](bool okVoid) -> std::string {
        const Token tok = cur;
        std::string t;
        switch (tok.type) {
            case GTokenType::KwInt: t = "int"; nextTok(); break;
            case GTokenType::KwDouble: t = "double"; nextTok(); break;
            case GTokenType::KwFloat: t = "float"; nextTok(); break;
            case GTokenType::KwBool: t = "bool"; nextTok(); break;
            case GTokenType::KwString: t = "string"; nextTok(); break;
            case GTokenType::KwVoid:
                if (!okVoid) throw std::runtime_error("parametro de fn<...> nao pode ser void (para zero parametros use fn<tipo()>)");
                t = "void";
                nextTok();
                break;
            case GTokenType::KwGhost: t = "ghost"; nextTok(); break;
            case GTokenType::KwComplex: t = "complex"; nextTok(); break;
            case GTokenType::KwQreg: t = "qreg"; nextTok(); break;
            case GTokenType::KwQcircuit: t = "qcircuit"; nextTok(); break;
            case GTokenType::KwList: {
                nextTok();
                if (pendingGt > 0 || cur.type != GTokenType::Lt) throw std::runtime_error("use list<tipo> dentro de fn<...>");
                nextTok();
                const std::string e = parseOne(false);
                expectGt();
                t = "list<" + e + ">";
                break;
            }
            case GTokenType::KwMap: {
                nextTok();
                if (pendingGt > 0 || cur.type != GTokenType::Lt) throw std::runtime_error("use map<chave, valor> dentro de fn<...>");
                nextTok();
                const std::string k = parseOne(false);
                if (pendingGt > 0 || cur.type != GTokenType::Comma) throw std::runtime_error("map<chave, valor> precisa de dois tipos");
                nextTok();
                const std::string v = parseOne(false);
                expectGt();
                t = "map<" + k + "," + v + ">";
                break;
            }
            case GTokenType::Identifier:
                if ((tok.value == "result" || tok.value == "set") && !class_memory.count(tok.value) && !interface_memory.count(tok.value)) {
                    const std::string h = tok.value;
                    nextTok();
                    if (pendingGt > 0 || cur.type != GTokenType::Lt) throw std::runtime_error("use " + h + "<tipo>");
                    nextTok();
                    const std::string e = parseOne(false);
                    expectGt();
                    t = h + "<" + e + ">";
                    break;
                }
                if (tok.value == "fn" && !class_memory.count("fn") && !interface_memory.count("fn")) {
                    nextTok();
                    if (pendingGt > 0 || cur.type != GTokenType::Lt)
                        throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)>");
                    nextTok();
                    const std::string r = parseOne(true);
                    if (pendingGt > 0 || cur.type != GTokenType::LParen)
                        throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)> ou fn<void()>");
                    nextTok();
                    std::string ps;
                    bool first = true;
                    while (pendingGt > 0 || (cur.type != GTokenType::RParen && cur.type != GTokenType::EndOfFile)) {
                        if (!first) {
                            if (pendingGt > 0 || cur.type != GTokenType::Comma)
                                throw std::runtime_error("esperado ',' ou ')' nos parametros de fn<...>");
                            nextTok();
                        }
                        first = false;
                        const std::string pt = parseOne(false);
                        if (pendingGt == 0 && cur.type == GTokenType::Identifier)
                            throw std::runtime_error("dentro de fn<...> informe apenas os tipos, sem nomes de parametros (ex.: fn<int(int, string)>)");
                        if (!ps.empty()) ps += ",";
                        ps += pt;
                    }
                    if (cur.type != GTokenType::RParen) throw std::runtime_error("faltou ')' em fn<...>");
                    nextTok();
                    expectGt();
                    t = "fn<" + r + "(" + ps + ")>";
                    break;
                }
                if (generic_memory.count(tok.value) || generic_iface_memory.count(tok.value)) {
                    const std::string gname = tok.value;
                    const bool isGenCls = generic_memory.count(gname) > 0;
                    const size_t expectedCnt = isGenCls ? generic_memory.at(gname).params.size()
                                                        : generic_iface_memory.at(gname).params.size();
                    nextTok();
                    if (pendingGt > 0 || cur.type != GTokenType::Lt)
                        throw std::runtime_error(gname + (isGenCls ? " e uma classe generica: use " : " e uma interface generica: use ") + gname + "<tipo>");
                    nextTok();
                    std::string args = parseOne(false);
                    size_t cnt = 1;
                    while (pendingGt == 0 && cur.type == GTokenType::Comma) {
                        nextTok();
                        args += "," + parseOne(false);
                        ++cnt;
                    }
                    if (cnt != expectedCnt)
                        throw std::runtime_error(gname + " precisa de " + std::to_string(expectedCnt) + " tipo(s) entre < >");
                    expectGt();
                    t = gname + "<" + args + ">";
                    break;
                }
                t = tok.value;
                nextTok();
                break;
            default:
                throw std::runtime_error("tipo invalido em fn<...>: '" + tok.value + "'");
        }
        while (pendingGt == 0 && cur.type == GTokenType::LBracket) {
            if (t == "void") throw std::runtime_error("void[] nao existe");
            nextTok();
            if (cur.type != GTokenType::RBracket) throw std::runtime_error("esperado ']' no tipo " + t + "[]");
            nextTok();
            t = "list<" + t + ">";
        }
        return t;
    };
    const std::string res = parseOne(allowVoid);
    if (pendingGt > 0 || cur.type != GTokenType::EndOfFile) {
        if (cur.type == GTokenType::Identifier)
            throw std::runtime_error("dentro de fn<...> informe apenas os tipos, sem nomes de parametros (ex.: fn<int(int, string)>)");
        throw std::runtime_error("tipo invalido: '" + raw + "'");
    }
    return res;
}

std::string GhostInterpreter::validateAndNormalizeFnSpec(const std::string& spec) {
    const std::string norm = validateAndNormalizeTypeSpec(spec, false);
    if (!isFnSpec(norm)) throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)>");
    ensureTypeReady(norm);
    return norm;
}

bool GhostInterpreter::canConvertFnReturn(const std::string& from, const std::string& to) const {
    if (to == "void") return true;
    if (from.empty()) return true; // tipo dinâmico desconhecido: será conferido ao executar
    if (from == "void") return false;
    if (from == to) return true;
    const auto isNum = [](const std::string& s) { return s == "int" || s == "double" || s == "float" || s == "bool"; };
    if (isNum(from) || isNum(to)) {
        if (!isNum(from) || !isNum(to)) return false;
        if (to == "double" && (from == "int" || from == "float")) return true;
        if (to == "float" && from == "int") return true;
        return false;
    }
    if (isResultSpec(to)) {
        if (from == "result") return true; // err(...) converte para qualquer result<T>
        if (startsWith(from, "ok<") && endsWith(from, ">")) {
            const std::string u = from.substr(3, from.size() - 4);
            const std::string targetInner = innerOf(to);
            if (u.empty() || u == targetInner) return true;
            if ((targetInner == "double" || targetInner == "float" || targetInner == "int") &&
                (u == "int" || u == "double" || u == "float")) return true; // GhostOk<U> usa static_cast<T>(o.value)
            return canConvertFnReturn(u, targetInner);
        }
        return false;
    }
    if (class_memory.count(to) && class_memory.count(from)) return isSubclass(from, to);
    if (interface_memory.count(to)) {
        return (class_memory.count(from) && implementsIface(from, to)) ||
               (interface_memory.count(from) && ifaceExtends(from, to));
    }
    if (isListSpec(to) && isListSpec(from)) {
        return canConvertFnReturn(from.substr(5, from.size() - 6), to.substr(5, to.size() - 6));
    }
    if (isFnSpec(to) && isFnSpec(from)) {
        std::string fr, tr;
        std::vector<std::string> fp, tp;
        if (!parseFnSignature(from, fr, fp) || !parseFnSignature(to, tr, tp)) return false;
        return fp == tp && canConvertFnReturn(fr, tr);
    }
    return false;
}

GhostValue GhostInterpreter::coerceFn(const GhostValue& v, const std::string& rawSpec, const std::string& what) {
    const std::string spec = validateAndNormalizeFnSpec(rawSpec);
    if (v.kind != GhostValue::Fn)
        throw std::runtime_error(what + ": esperado " + spec + " (passe uma lambda ou o nome de uma funcao), recebido " + toText(v, true));
    std::string targetRet;
    std::vector<std::string> targetParams;
    if (!parseFnSignature(spec, targetRet, targetParams))
        throw std::runtime_error("tipo de funcao invalido: " + spec);
    if (!v.fn) {
        if (!v.str.empty() && v.str != spec) {
            std::string sr;
            std::vector<std::string> sp;
            if (parseFnSignature(v.str, sr, sp) && (sp != targetParams || !canConvertFnReturn(sr, targetRet)))
                throw std::runtime_error(what + ": " + v.str + " nao e compativel com " + spec);
        }
        GhostValue r;
        r.kind = GhostValue::Fn;
        r.str = spec;
        return r;
    }
    auto fillTypePair = [&](const std::string& ps, GTokenType& outTy, std::string& outCls) {
        outCls.clear();
        if (ps == "int") { outTy = GTokenType::KwInt; return; }
        if (ps == "double") { outTy = GTokenType::KwDouble; return; }
        if (ps == "float") { outTy = GTokenType::KwFloat; return; }
        if (ps == "bool") { outTy = GTokenType::KwBool; return; }
        if (ps == "string") { outTy = GTokenType::KwString; return; }
        if (ps == "void") { outTy = GTokenType::KwVoid; return; }
        if (ps == "ghost") { outTy = GTokenType::KwGhost; return; }
        if (ps == "complex") { outTy = GTokenType::KwComplex; return; }
        if (ps == "qreg") { outTy = GTokenType::KwQreg; return; }
        if (ps == "qcircuit") { outTy = GTokenType::KwQcircuit; return; }
        if (isListSpec(ps)) { outTy = GTokenType::KwList; outCls = ps; return; }
        if (startsWith(ps, "list<")) { outTy = GTokenType::KwList; return; }
        if (startsWith(ps, "map<")) { outTy = GTokenType::KwMap; return; }
        outTy = GTokenType::KwVar;
        outCls = ps;
    };
    // 1. Valor obtido pelo nome direto de uma função genérica (ex.: fn<int(int)> f = id;)
    if (v.fn->isGenericFnVal) {
        auto git = generic_functions.find(v.fn->genericFnName);
        if (git == generic_functions.end())
            throw std::runtime_error("funcao generica nao encontrada: " + v.fn->genericFnName);
        const GhostGenericFn& g = git->second;
        if (targetParams.size() != g.paramTypes.size())
            throw std::runtime_error(what + ": funcao generica " + g.name + " de " +
                                     std::to_string(g.paramTypes.size()) + " parametro(s) nao cabe em " + spec);
        // Casa cada tipo de parâmetro de g com targetParams[k]
        std::map<std::string, std::string> got;
        std::function<void(const std::string&, const std::string&)> matchPat = [&](const std::string& pat, const std::string& act) {
            for (const auto& tp : g.params) {
                if (pat == tp) {
                    auto it = got.find(tp);
                    if (it != got.end() && it->second != act)
                        throw std::runtime_error(what + ": conflito ao deduzir " + tp + " em " + g.name + " (" + it->second + " vs " + act + ")");
                    got[tp] = act;
                    return;
                }
            }
            if (isFnSpec(pat) && isFnSpec(act)) {
                std::string pr, ar;
                std::vector<std::string> pps, aps;
                if (parseFnSignature(pat, pr, pps) && parseFnSignature(act, ar, aps) && pps.size() == aps.size()) {
                    matchPat(pr, ar);
                    for (size_t i = 0; i < pps.size(); ++i) matchPat(pps[i], aps[i]);
                }
                return;
            }
            const size_t plt = pat.find('<'), alt = act.find('<');
            if (plt != std::string::npos && alt != std::string::npos && pat.back() == '>' && act.back() == '>' &&
                pat.substr(0, plt) == act.substr(0, alt)) {
                auto splitInner = [](const std::string& s, size_t ltPos) {
                    std::vector<std::string> parts;
                    std::string cur;
                    int d = 0;
                    for (size_t k = ltPos + 1; k + 1 < s.size(); ++k) {
                        const char ch = s[k];
                        if (ch == '<' || ch == '(') d++;
                        else if (ch == '>' || ch == ')') d--;
                        else if (ch == ',' && d == 0) { parts.push_back(cur); cur.clear(); continue; }
                        cur += ch;
                    }
                    parts.push_back(cur);
                    return parts;
                };
                const auto pParts = splitInner(pat, plt);
                const auto aParts = splitInner(act, alt);
                if (pParts.size() == aParts.size()) {
                    for (size_t i = 0; i < pParts.size(); ++i) matchPat(pParts[i], aParts[i]);
                }
            }
        };
        for (size_t k = 0; k < targetParams.size(); ++k) matchPat(g.paramTypes[k], targetParams[k]);
        std::vector<std::string> targs;
        for (const auto& tp : g.params) {
            auto it = got.find(tp);
            if (it == got.end())
                throw std::runtime_error(what + ": nao deu para deduzir o tipo " + tp + " de " + g.name + " para " + spec + " (use " + g.name + "<tipo>)");
            targs.push_back(it->second);
        }
        const std::string instSpec = instantiateFunction(g.name, targs);
        const GhostFunction& instF = functions_memory.at(instSpec);
        if (targetParams.size() < instF.parameters.size()) {
            for (size_t k = targetParams.size(); k < instF.parameters.size(); ++k) {
                if (k >= instF.defaults.size() || instF.defaults[k].empty())
                    throw std::runtime_error(what + ": funcao " + instSpec + " exige " + std::to_string(instF.parameters.size()) + " argumento(s) para " + spec);
            }
        }
        for (size_t k = 0; k < targetParams.size(); ++k) {
            if (instF.paramSpecs[k] != targetParams[k])
                throw std::runtime_error(what + ": parametro " + std::to_string(k + 1) + " de " + instSpec + " e " +
                                         instF.paramSpecs[k] + ", mas " + spec + " espera " + targetParams[k]);
        }
        std::string instRet = instF.returnSpec.empty() ? std::string("void") : instF.returnSpec;
        if (!canConvertFnReturn(instRet, targetRet))
            throw std::runtime_error(what + ": retorno " + instRet + " de " + instSpec + " nao converte para " + targetRet + " em " + spec);
        GhostValue named = makeNamedFnValue(instSpec, instF);
        named.fn->params.resize(targetParams.size());
        named.fn->paramClasses.resize(targetParams.size());
        named.fn->paramSpecs = targetParams;
        named.fn->wrapperRetSpec = targetRet;
        named.str = spec;
        return named;
    }
    // 2. Lambda polimórfica ((x) => x + 1 ou (x, int y) => x + y) convertida para fn<...>
    if (v.fn->isPolyLambda) {
        if (v.fn->paramSpecs.size() != targetParams.size())
            throw std::runtime_error(what + ": funcao de " + std::to_string(v.fn->paramSpecs.size()) +
                                     " parametro(s) nao cabe em " + spec);
        auto copy = std::make_shared<GhostClosure>(*v.fn);
        for (size_t k = 0; k < targetParams.size(); ++k) {
            if (!copy->paramSpecs[k].empty() && copy->paramSpecs[k] != targetParams[k])
                throw std::runtime_error(what + ": parametro " + std::to_string(k + 1) + " da funcao e " +
                                         copy->paramSpecs[k] + ", mas " + spec + " espera " + targetParams[k]);
            copy->paramSpecs[k] = targetParams[k];
            fillTypePair(targetParams[k], copy->params[k].first, copy->paramClasses[k]);
        }
        copy->isPolyLambda = false;
        std::string srcRet = copy->hasExplicitRet ? copy->returnSpec : inferLambdaRetSpec(*copy);
        if (startsWith(srcRet, "ok<") && endsWith(srcRet, ">") && isResultSpec(targetRet)) {
            // ok<U> validado por canConvertFnReturn
        }
        if (!canConvertFnReturn(srcRet, targetRet))
            throw std::runtime_error(what + ": retorno " + srcRet + " da funcao nao converte para " + targetRet + " em " + spec);
        copy->inferredRetSpec = srcRet;
        copy->wrapperRetSpec = targetRet;
        GhostValue r;
        r.kind = GhostValue::Fn;
        r.str = spec;
        r.fn = std::move(copy);
        return r;
    }
    if (v.fn->paramSpecs.size() != targetParams.size())
        throw std::runtime_error(what + ": funcao de " + std::to_string(v.fn->paramSpecs.size()) +
                                 " parametro(s) nao cabe em " + spec);
    for (size_t k = 0; k < targetParams.size(); ++k) {
        if (v.fn->paramSpecs[k] != targetParams[k])
            throw std::runtime_error(what + ": parametro " + std::to_string(k + 1) + " da funcao e " +
                                     v.fn->paramSpecs[k] + ", mas " + spec + " espera " + targetParams[k]);
    }
    std::string srcRet = !v.fn->wrapperRetSpec.empty() ? v.fn->wrapperRetSpec
                                                       : (v.fn->hasExplicitRet ? v.fn->returnSpec : v.fn->inferredRetSpec);
    if (srcRet.empty()) srcRet = inferLambdaRetSpec(*v.fn);
    if (!canConvertFnReturn(srcRet, targetRet))
        throw std::runtime_error(what + ": retorno " + srcRet + " da funcao nao converte para " + targetRet + " em " + spec);

    auto copy = std::make_shared<GhostClosure>(*v.fn);
    copy->wrapperRetSpec = targetRet;
    GhostValue r;
    r.kind = GhostValue::Fn;
    r.str = spec;
    r.fn = std::move(copy);
    return r;
}

// Valor de dentro do result convertido para T (int trunca, double, bool, string, listas, mapas, classes, result<U>, fn<...>)
GhostValue GhostInterpreter::coerceSpec(const GhostValue& v, const std::string& spec, const std::string& what) {
    ensureTypeReady(spec);
    if (endsWith(spec, "[]")) {
        const std::string norm = validateAndNormalizeTypeSpec(spec, false);
        ensureTypeReady(norm);
        return isListSpec(norm) ? coerceList(v, norm, what) : coerce(v, GTokenType::KwList, "", what);
    }
    if (isListSpec(spec)) return coerceList(v, spec, what);
    if (isResultSpec(spec) || isSetSpec(spec) || isFnSpec(spec) || class_memory.count(spec) || interface_memory.count(spec))
        return coerce(v, GTokenType::KwVar, spec, what);
    if (startsWith(spec, "list<") || spec == "list") return coerce(v, GTokenType::KwList, "", what);
    if (startsWith(spec, "map<") || spec == "map") return coerce(v, GTokenType::KwMap, "", what);
    if (spec == "int") return coerce(v, GTokenType::KwInt, "", what);
    if (spec == "double" || spec == "float") return coerce(v, GTokenType::KwDouble, "", what);
    if (spec == "bool") return coerce(v, GTokenType::KwBool, "", what);
    if (spec == "string") return coerce(v, GTokenType::KwString, "", what);
    return v; // var e outros tipos: sem conversão
}

GhostValue GhostInterpreter::coerceResult(const GhostValue& v, const std::string& spec, const std::string& what) {
    if (v.kind != GhostValue::Result)
        throw std::runtime_error(what + ": esperado " + spec + " (use ok(valor) ou err(\"mensagem\")), recebido " + toText(v, true));
    if (!v.str.empty() && v.str != spec) throw std::runtime_error(what + ": " + v.str + " nao e " + spec);
    GhostValue r = v;
    r.str = spec;
    if (r.num != 0.0 && !r.list.empty()) r.list[0] = coerceSpec(r.list[0], innerOf(spec), what + " (valor de " + spec + ")");
    return r;
}

// set<T>: aceita conjunto ou lista; os elementos viram T (como o GhostSet<T> do compilado)
GhostValue GhostInterpreter::coerceList(const GhostValue& v, const std::string& spec, const std::string& what) {
    const std::string norm = validateAndNormalizeTypeSpec(spec, false);
    ensureTypeReady(norm);
    if (v.kind != GhostValue::List)
        throw std::runtime_error(what + ": esperado " + norm + ", recebido " + toText(v, true));
    const std::string elemSpec = norm.substr(5, norm.size() - 6);
    if (elemSpec == "var") return v;
    GhostValue r;
    r.kind = GhostValue::List;
    r.str = elemSpec;
    r.list.reserve(v.list.size());
    for (const auto& e : v.list) r.list.push_back(coerceSpec(e, elemSpec, what + " (elemento de " + norm + ")"));
    return r;
}

GhostValue GhostInterpreter::coerceSet(const GhostValue& v, const std::string& spec, const std::string& what) {
    if (v.kind != GhostValue::Set && v.kind != GhostValue::List)
        throw std::runtime_error(what + ": esperado " + spec + " (use [1, 2] ou set(lista)), recebido " + toText(v, true));
    const std::string inner = spec.substr(4, spec.size() - 5);
    GhostValue r;
    r.kind = GhostValue::Set;
    r.str = inner;
    for (const auto& e : v.list) r.list.push_back(coerceSpec(e, inner, what + " (elemento de " + spec + ")"));
    normalizeSet(r);
    return r;
}

GhostValue GhostInterpreter::defaultOf(const std::string& spec) {
    if (spec == "int") return GhostValue::number(0, true);
    if (spec == "double" || spec == "float") return GhostValue::number(0.0);
    if (spec == "bool") return GhostValue::boolean(false);
    if (spec == "string") return GhostValue::text("");
    if (spec.rfind("map<", 0) == 0 || spec == "map") {
        GhostValue r;
        r.kind = GhostValue::Map;
        return r;
    }
    ensureTypeReady(spec);
    if (isListSpec(spec)) {
        const std::string norm = validateAndNormalizeTypeSpec(spec, false);
        GhostValue r;
        r.kind = GhostValue::List;
        r.str = norm.substr(5, norm.size() - 6);
        return r;
    }
    if (isFnSpec(spec)) { // fn<int(int)> f; -> vazio (chamar sem guardar função lança erro)
        GhostValue r;
        r.kind = GhostValue::Fn;
        r.str = validateAndNormalizeFnSpec(spec);
        return r;
    }
    if (isSetSpec(spec)) { // set<T> s;  -> {}
        GhostValue r;
        r.kind = GhostValue::Set;
        r.str = spec.substr(4, spec.size() - 5);
        return r;
    }
    if (isResultSpec(spec)) { // como GhostResult<T>(): erro "result sem valor"
        GhostValue r;
        r.kind = GhostValue::Result;
        r.num = 0.0;
        r.str = spec;
        r.list.push_back(GhostValue::text("result sem valor"));
        return r;
    }
    if (interface_memory.count(spec)) { // Forma f;  (sem objeto até receber um)
        GhostValue r;
        r.kind = GhostValue::Iface;
        r.str = spec;
        return r;
    }
    return construct(spec, {});
}

GhostValue GhostInterpreter::tryOperator(const GhostValue& r) {
    if (r.kind != GhostValue::Result) throw std::runtime_error("o operador ? precisa de um result<T>, recebido " + toText(r, true));
    if (ret_stack.empty() || !isResultSpec(ret_stack.back()))
        throw std::runtime_error("o operador ? so vale dentro de function result<T> (ex.: function result<double> f() { double v = g()?; ... })");
    if (r.num == 0.0) { // como o return GhostErr{...} gerado pelo compilador
        GhostReturnSignal sig;
        sig.value.kind = GhostValue::Result;
        sig.value.num = 0.0;
        sig.value.list.push_back(GhostValue::text(r.list.empty() ? std::string("result sem valor") : r.list[0].str));
        sig.hasValue = true;
        throw sig;
    }
    return r.list.empty() ? GhostValue::boolean(true) : r.list[0];
}

GhostValue GhostInterpreter::resultMethod(const GhostValue& self, const std::string& m, std::vector<GhostValue>& a) {
    auto need = [&](size_t lo, size_t hi) {
        if (a.size() < lo || a.size() > hi) throw std::runtime_error("." + m + ": numero de argumentos invalido");
    };
    const bool good = self.num != 0.0;
    const GhostValue inner = self.list.empty() ? GhostValue::text("result sem valor") : self.list[0];
    if (m == "is_ok") { need(0, 0); return GhostValue::boolean(good); }
    if (m == "is_err") { need(0, 0); return GhostValue::boolean(!good); }
    if (m == "value") {
        need(0, 0);
        if (!good) throw std::runtime_error(inner.str); // como GhostResult::value(): lança a mensagem do erro
        return inner;
    }
    if (m == "value_or") {
        need(1, 1);
        if (good) return inner;
        return isResultSpec(self.str) ? coerceSpec(a[0], innerOf(self.str), ".value_or") : a[0];
    }
    if (m == "error") { need(0, 0); return GhostValue::text(good ? "" : inner.str); }
    throw std::runtime_error("result nao tem o metodo ." + m + " (use is_ok, is_err, value, value_or ou error)");
}

bool GhostInterpreter::opensTypeAngle(const Token& prev) const {
    if (prev.type == GTokenType::KwList || prev.type == GTokenType::KwMap) return true;
    if (prev.type != GTokenType::Identifier) return false;
    return prev.value == "fn" || prev.value == "result" || prev.value == "set" ||
           generic_memory.count(prev.value) || generic_iface_memory.count(prev.value) ||
           generic_functions.count(prev.value) || generic_method_names.count(prev.value);
}
