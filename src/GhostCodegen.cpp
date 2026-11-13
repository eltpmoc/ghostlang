// GhostCodegen.cpp - geração de C++ a partir da AST (c40, etapa 2). Veja include/GhostCodegen.hpp.
//
// Cobre a linguagem compilada inteira:
//   - núcleo: funções (também genéricas, com restrições e sobrecargas para lambdas polimórficas),
//     variáveis locais e globais, print, if/else, while, do/while, for, foreach, break, continue,
//     return, switch, try/catch/finally, throw, spawn, hypothesis/collapse;
//   - expressões: aritmética, comparações, ternário, **, chamadas, membros, índices, fatias, listas,
//     mapas e JSON literais, compreensões, lambdas, f-strings, result com ?, 2.0i / 3.5g e a álgebra
//     ghost com i e theta;
//   - classes (herança, super, static, public/private/protected, genéricas), interfaces (métodos
//     padrão, genéricas) e circuit (com as variantes _dg e _ctrl);
//   - comandos de IA/quântico (state, gate, qreg, qcircuit, qlayer, network, show, activate, apply,
//     load(m, "x.csv"), x => load/save/optimize(...), server, sleep, mutex, lock, unlock, input,
//     pragma, swarm).
// O que ainda é recusado (load "arquivo", formas raras) volta para o Parser antigo.
//
// As regras de tradução são as do Parser antigo (src/Parser.cpp, src/ParserCore.cpp), inclusive os
// detalhes que mudam o C++: o primeiro nome de um comando sai cru (parseAction), uma lista literal
// pega o tipo da declaração (listLiteralType), função usada como valor vira ghost_fn_val(f), classes e
// genéricos valem a partir da declaração, o contador único (ghost_sw_N, ghost_fin_N, ...) avança nos
// mesmos pontos, e os #line saem nos mesmos lugares (antes de cada comando; no nível principal,
// também antes de cada '}'; nada dentro de switch, try, lambda, hypothesis, circuit e interface).
#include "../include/GhostCodegen.hpp"

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "../include/GhostCppEmit.hpp"
#include "../include/GhostDiag.hpp"
#include "../include/GhostFormat.hpp"
#include "../include/GhostQuantumOps.hpp"

namespace {

// Auxiliares de tipos genéricos (cópia dos de src/Parser.cpp, que ficam num namespace anônimo)
std::string trimSpaces(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

void splitCppTemplateSpec(const std::string& spec, std::string& baseName, std::vector<std::string>& args) {
    args.clear();
    const size_t lt = spec.find('<');
    if (lt == std::string::npos || spec.back() != '>') {
        baseName = spec;
        return;
    }
    baseName = spec.substr(0, lt);
    std::string cur;
    int depth = 0;
    for (size_t k = lt + 1; k + 1 < spec.size(); ++k) {
        const char ch = spec[k];
        if (ch == '<' || ch == '(') depth++;
        else if (ch == '>' || ch == ')') depth--;
        else if (ch == ',' && depth == 0) {
            args.push_back(trimSpaces(cur));
            cur.clear();
            continue;
        }
        cur += ch;
    }
    if (!cur.empty() || lt + 1 < spec.size() - 1) args.push_back(trimSpaces(cur));
}

std::string substCppTypeWords(const std::string& src, const std::vector<std::string>& params, const std::vector<std::string>& args) {
    if (params.empty() || params.size() != args.size()) return src;
    auto isId = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    std::string out;
    for (size_t i = 0; i < src.size();) {
        if (std::isalpha(static_cast<unsigned char>(src[i])) || src[i] == '_') {
            size_t j = i;
            while (j < src.size() && isId(src[j])) ++j;
            const std::string w = src.substr(i, j - i);
            const bool afterScope = i >= 2 && src[i - 1] == ':' && src[i - 2] == ':';
            const bool beforeScope = j + 1 < src.size() && src[j] == ':' && src[j + 1] == ':';
            bool replaced = false;
            if (!afterScope && !beforeScope) {
                for (size_t k = 0; k < params.size(); ++k) {
                    if (params[k] == w) {
                        out += args[k];
                        replaced = true;
                        break;
                    }
                }
            }
            if (!replaced) out += w;
            i = j;
        } else {
            out += src[i++];
        }
    }
    return out;
}


struct Unsupported {
    std::string what;
    int line;
};

bool wordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// Junta dois pedaços como o Parser antigo (append): espaço entre nomes e entre sinais (- -x, + +x)
std::string cat(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const char x = a.back(), y = b.front();
    if ((wordChar(x) && wordChar(y)) || ((x == '-' || x == '+') && (y == '-' || y == '+'))) return a + " " + b;
    return a + b;
}

std::string joinList(const std::vector<std::string>& v, const char* sep) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) s += (i ? sep : "") + v[i];
    return s;
}

// Texto de string literal no C++ (Lexer::cppStringLiteral, com aspas duplas)
std::string cppStringLiteral(const std::string& text) {
    std::string out = "std::string(\"";
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\\' && i + 1 < text.size()) {
            out += c;
            out += text[++i];
            continue;
        }
        if (c == '"') { out += "\\\""; continue; }
        if (c == '\n') { out += "\\n"; continue; }
        if (c == '\r') continue;
        out += c;
    }
    return out + "\")";
}

bool endsWith(const std::string& s, const std::string& suf) {
    return s.size() > suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

struct MethodSig {
    std::string ret;
    std::vector<std::string> params, paramNames;
    bool hasDefault = false;
    std::string defaultBody;
};

struct IfaceInfo {
    std::vector<std::string> tparams;
    std::vector<std::string> bases;  // com argumentos: Colecao<T>
    std::vector<std::pair<std::string, MethodSig>> methods;
    std::string typeAsserts;
};

struct HypFrame {
    int id;
    std::set<std::string> assigned, declared;
};

class Gen {
public:
    Gen(bool wasm, const std::string& cppFile) : wasm_(wasm), cppFile_(cppFile) {}

    std::string run(const AstNode& program) {
        prescan(program);
        mainCode_ += ghost_cpp_main_prologue();
        for (const auto& k : program.kids) top(*k);
        mainCode_ += ghost_cpp_main_epilogue(hasMain_, mainParams_, mainArray_);
        std::string forward;
        for (const auto& c : classes_) {
            auto g = genericParams_.find(c);
            std::string templ;
            if (g != genericParams_.end()) templ = "template <" + classList(g->second) + "> ";
            forward += templ + "class " + c + ";\n";
        }
        for (const auto& i : ifaces_) {
            std::string templ;
            if (!i.second.tparams.empty()) templ = "template <" + classList(i.second.tparams) + "> ";
            forward += templ + "class " + i.first + ";\n";
        }
        return ghost_cpp_assemble(ghost_cpp_header(wasm_), forward, protos_, classesCode_, globals_, functions_, mainCode_, cppFile_);
    }

private:
    bool wasm_;
    std::string cppFile_;
    std::string protos_, classesCode_, globals_, functions_, mainCode_;
    std::string* buf_ = &mainCode_;
    std::string lastMarker_;
    int uniqueCounter_ = 0;

    // nomes do arquivo (pré-varredura do Parser) e registrados à medida que são declarados
    std::set<std::string> declaredFunctions_, circuits_;
    std::map<std::string, size_t> declaredGenericFns_;
    bool fileUsesPolyFn_ = false;
    std::map<std::string, std::vector<std::string>> genericFunctions_, genericParams_, genericIfaceParams_;
    std::set<std::string> genericMethodNames_, classes_, userFunctions_, globalVars_;
    std::map<std::string, IfaceInfo> ifaces_;
    std::set<std::string> quantumLayers_, deepNetworks_;

    // contexto
    std::vector<std::set<std::string>> locals_{1};  // localVars do Parser, com escopo de bloco
    std::set<std::string> activeTypeParams_;
    bool inFunction_ = false, isInsideClass_ = false, inInstanceMethod_ = false;
    bool parsingField_ = false, staticField_ = false, noMarkers_ = false;
    std::string currentClass_, currentBase_, currentIface_, currentReturnType_;
    std::set<std::string> currentIfaceMethodNames_;
    std::string circuitTarget_, currentCircuit_;
    bool hasMain_ = false, mainArray_ = false;
    int mainParams_ = 0;
    bool ghostAlgebra_ = false;
    std::string listType_;   // listLiteralType: tipo da declaração para a primeira lista literal
    std::string mapType_;    // map<K, V> m = {...}: o primeiro {...} vira o mapa tipado
    std::vector<std::pair<char, std::string>> blocks_;  // 'l' laço, 's' switch (rótulo do fim)
    std::vector<HypFrame> hyp_;

    [[noreturn]] static void no(const AstNode& n, const std::string& what) { throw Unsupported{what, n.line}; }
    static std::string classList(const std::vector<std::string>& ps) {
        std::string t;
        for (size_t k = 0; k < ps.size(); ++k) t += (k ? ", class " : "class ") + ps[k];
        return t;
    }

    // ------------------------------------------------------------ pré-varredura (Parser::Parser)
    void prescan(const AstNode& program) {
        for (const auto& k : program.kids) {
            if (k->kind == AstKind::CircuitDecl) circuits_.insert(k->name);
            if (k->kind != AstKind::FuncDecl || k->name == "main") continue;
            size_t tps = 0;
            for (const auto& c : k->kids)
                if (c->kind == AstKind::TypeParam) ++tps;
            if (tps) declaredGenericFns_[k->name] = tps;
            else declaredFunctions_.insert(k->name);
        }
        // o arquivo usa valores polimórficos de função? (função genérica como valor ou lambda (x) => ...)
        std::function<void(const AstNode&, const AstNode*)> walk = [&](const AstNode& n, const AstNode* parent) {
            if (fileUsesPolyFn_) return;
            if (n.kind == AstKind::Ident && declaredGenericFns_.count(n.text)) {
                const bool callee = parent && parent->kind == AstKind::Call && parent->kids[0].get() == &n && n.parens == 0;
                const bool generic = parent && parent->kind == AstKind::GenericRef;
                if (!callee && !generic) fileUsesPolyFn_ = true;
            }
            if (n.kind == AstKind::Lambda)
                for (const auto& k : n.kids)
                    if (k->kind == AstKind::Param && k->kids[0]->text == "var") fileUsesPolyFn_ = true;
            for (const auto& k : n.kids) walk(*k, &n);
        };
        walk(program, nullptr);
    }

    bool isLocal(const std::string& n) const {
        for (const auto& s : locals_)
            if (s.count(n)) return true;
        return false;
    }
    void declare(const std::string& n) { locals_.back().insert(n); }
    bool isMain() const { return buf_ == &mainCode_; }
    void hypAssigned(const std::string& n) {
        for (auto& f : hyp_) f.assigned.insert(n);
    }
    void hypDeclared(const std::string& n) {
        for (auto& f : hyp_) f.declared.insert(n);
    }

    // ------------------------------------------------------------ #line
    void marker(int line) {
        if (noMarkers_) return;
        if (buf_ != &mainCode_ && buf_ != &functions_ && buf_ != &classesCode_) return;
        if (!buf_->empty() && buf_->back() != '\n') return;
        const auto loc = GhostDiag::get().locate(line);
        if (loc.first.empty() || loc.second <= 0) return;
        std::string file;
        for (char c : loc.first) file += (c == '\\') ? '/' : c;
        const std::string m = "#line " + std::to_string(loc.second) + " \"" + file + "\"\n";
        if (m == lastMarker_ && buf_->size() >= m.size() && buf_->compare(buf_->size() - m.size(), m.size(), m) == 0) return;
        *buf_ += m;
        lastMarker_ = m;
    }
    int depth_ = 0;  // recuo do C++ gerado (o Parser antigo usa sempre 4 espaços; a comparação ignora espaços)
    std::string indent() const { return std::string(4 * static_cast<size_t>(depth_ + 1), ' '); }
    void line(const std::string& code) { *buf_ += indent() + code + "\n"; }

    // ------------------------------------------------------------ tipos (Parser::parseTypeName)
    std::string type(const AstNode& t) {
        const std::string& n = t.text;
        auto args = [&]() {
            std::vector<std::string> a;
            for (const auto& k : t.kids) a.push_back(type(*k));
            return joinList(a, ", ");
        };
        if (n == "list" || n == "set" || n == "result") {
            if (t.kids.size() != 1) no(t, "tipo " + n);
            return std::string(n == "list" ? "GhostList<" : n == "set" ? "GhostSet<" : "GhostResult<") + type(*t.kids[0]) + ">";
        }
        if (n == "map") {
            if (t.kids.size() != 2) no(t, "tipo map");
            return "GhostMap<" + type(*t.kids[0]) + ", " + type(*t.kids[1]) + ">";
        }
        if (n == "fn" && !classes_.count("fn") && !ifaces_.count("fn")) {
            if (t.kids.empty()) no(t, "tipo fn");
            std::vector<std::string> ps;
            for (size_t i = 1; i < t.kids.size(); ++i) {
                std::string p = type(*t.kids[i]);
                if (p == "GhostQReg" || p == "GhostQCircuit") p += "&";
                ps.push_back(p);
            }
            return "GhostFn<" + type(*t.kids[0]) + "(" + joinList(ps, ", ") + ")>";
        }
        static const std::pair<const char*, const char*> simple[] = {
            {"int", "int"}, {"double", "double"}, {"float", "float"}, {"string", "std::string"}, {"bool", "bool"},
            {"void", "void"}, {"var", "auto"}, {"ghost", "GhostNumberG"}, {"complex", "std::complex<double>"},
            {"json", "GhostJson"}, {"date", "GhostDate"}, {"time", "GhostDate"}, {"datetime", "GhostDate"},
            {"textmodel", "GhostTextModel"}, {"llm", "GhostLLM"}, {"qreg", "GhostQReg"}, {"qcircuit", "GhostQCircuit"},
            {"matrix", "GhostMatrix"}, {"GhostMatrix", "GhostMatrix"}};
        for (const auto& s : simple)
            if (n == s.first) {
                if (!t.kids.empty()) no(t, "tipo " + n + "<...>");
                return s.second;
            }
        if (n == "qlayer") no(t, "tipo qlayer");
        // classes, interfaces, tipos genéricos (T) e tipos do runtime: o nome como está
        if (!t.kids.empty()) {
            if (activeTypeParams_.count(n) || (!genericParams_.count(n) && !genericIfaceParams_.count(n))) no(t, "tipo " + n + "<...>");
            return n + "<" + args() + ">";
        }
        if ((genericParams_.count(n) || genericIfaceParams_.count(n)) && !activeTypeParams_.count(n)) no(t, n + " sem <tipo>");
        return n;
    }
    std::string paramType(const AstNode& t) {
        std::string p = type(t);
        if (p == "GhostQReg" || p == "GhostQCircuit") p += "&";
        return p;
    }

    // <T extends Animal & B, U extends number>: static_assert de cada restrição (Parser::parseTypeParams)
    std::string typeParams(const AstNode& owner, const std::string& what, std::vector<std::string>& names) {
        std::string asserts;
        for (const auto& k : owner.kids) {
            if (k->kind != AstKind::TypeParam) continue;
            const std::string& p = k->name;
            names.push_back(p);
            activeTypeParams_.insert(p);
            for (const auto& b : k->kids) {
                if (k->attr == "extends" && b->text == "number" && b->kids.empty()) {
                    asserts += "    static_assert(std::is_arithmetic_v<" + p + "> && !std::is_same_v<" + p +
                               ", bool>, \"" + p + " precisa ser numero (int, double ou float): restricao de " + what + "\");\n";
                } else if (k->attr == "extends") {
                    if (!classes_.count(b->text)) no(*k, "restricao com classe desconhecida");
                    const std::string full = genericParams_.count(b->text) ? type(*b) : b->text;
                    asserts += "    static_assert(std::is_base_of_v<" + full + ", " + p + ">, \"" + p + " precisa ser " + full +
                               " ou subclasse: restricao de " + what + "\");\n";
                } else {
                    if (!ifaces_.count(b->text)) no(*k, "restricao com interface desconhecida");
                    const std::string full = genericIfaceParams_.count(b->text) ? type(*b) : b->text;
                    asserts += "    static_assert(std::is_base_of_v<ghost_iface_" + full + ", " + p + ">, \"" + p +
                               " precisa implementar " + full + ": restricao de " + what + "\");\n";
                }
            }
        }
        return asserts;
    }

    // ------------------------------------------------------------ expressões (Parser::translateExpr)
    std::string number(const std::string& t) const {
        const bool neg = !t.empty() && t[0] == '-';
        const std::string body = neg ? t.substr(1) : t;
        std::string fn, num;
        if (body.size() > 5 && body.compare(body.size() - 5, 5, "theta") == 0) { fn = "ghost_g("; num = body.substr(0, body.size() - 5); }
        else if (!body.empty() && body.back() == 'i') { fn = "ghost_i("; num = body.substr(0, body.size() - 1); }
        else if (!body.empty() && body.back() == 'g') { fn = "ghost_g("; num = body.substr(0, body.size() - 1); }
        else return t;
        return (neg ? "-" : "") + fn + num + ")";
    }

    bool isGenericFn(const std::string& v) const { return genericFunctions_.count(v) || declaredGenericFns_.count(v); }

    std::string ident(const AstNode& e, bool callee, bool target, bool raw) {
        std::string v = e.text;
        if (raw) { // primeiro nome de um comando (parseAction)
            if (callee && !currentIface_.empty() && !isLocal(v) && currentIfaceMethodNames_.count(v)) return "this->" + v;
            return v == "assert" ? "ghost_assert" : v;
        }
        if ((v == "i" || (ghostAlgebra_ && v == "theta")) && !isLocal(v) && !globalVars_.count(v) && !callee)
            return v == "i" ? "ghost_i(1.0)" : "ghost_g(1.0)";
        if (callee && !currentIface_.empty() && !isLocal(v) && currentIfaceMethodNames_.count(v)) return "this->" + v;
        if (!isLocal(v) && isGenericFn(v) && !callee && !target) {
            return "ghost_make_poly_fn([](auto&&... ghost_a_) -> decltype(" + v +
                   "(std::forward<decltype(ghost_a_)>(ghost_a_)...)) { return " + v +
                   "(std::forward<decltype(ghost_a_)>(ghost_a_)...); })";
        }
        if (genericParams_.count(v) && !activeTypeParams_.count(v)) no(e, v + " generica sem <tipo>");
        if (callee && v == "bool") return "ghost_bool";
        if (callee && v == "qreg") return "GhostQReg";
        if (callee && v == "qcircuit") return "GhostQCircuit";
        if (callee && v == "json") return "GhostJson";
        if (callee && v == "ghost") return "GhostNumberG";
        if (callee && (v == "matrix" || v == "GhostMatrix")) return "GhostMatrix";
        if (v == "assert") v = "ghost_assert";
        if (v == "union") v = "union_";
        if (v == "set" && callee) v = "ghost_to_set";
        if (declaredFunctions_.count(v) && !isGenericFn(v) && !isLocal(v) && !callee && !target) return "ghost_fn_val(" + v + ")";
        return v;
    }

    std::string typeArgs(const AstNode& n, size_t from) {
        std::vector<std::string> a;
        for (size_t i = from; i < n.kids.size(); ++i) a.push_back(type(*n.kids[i]));
        return joinList(a, ", ");
    }

    // Nome<tipos>: classe genérica (Pilha<int>), função genérica (f<int>) ou método genérico
    std::string genericRef(const AstNode& g, bool callee, bool scope) {
        const AstNode& id = *g.kids[0];
        const std::string& v = id.text;
        const std::string targs = typeArgs(g, 1);
        if (!isLocal(v) && isGenericFn(v)) {
            if (callee) return v + "<" + targs + ">";
            return "ghost_fn_val(&" + v + "<" + targs + ">)";
        }
        if (genericMethodNames_.count(v) && callee) return v + "<" + targs + ">";
        if (genericParams_.count(v) && !activeTypeParams_.count(v)) return v + "<" + targs + ">" + (scope ? "::" : "");
        no(g, "nome generico " + v);
    }

    std::string fstring(const AstNode& e) {
        const std::string& text = e.text;
        std::vector<std::string> parts;
        std::string literal;
        size_t i = 0;
        while (i < text.size()) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                literal += text[i];
                literal += text[i + 1];
                i += 2;
                continue;
            }
            if (text[i] == '{' && i + 1 < text.size() && text[i + 1] == '{') {
                literal += '{';
                i += 2;
                continue;
            }
            if (text[i] == '}' && i + 1 < text.size() && text[i + 1] == '}') {
                literal += '}';
                i += 2;
                continue;
            }
            const bool hasDollar = (text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{');
            if (hasDollar || text[i] == '{') {
                const size_t offset = hasDollar ? 2 : 1;
                size_t j = i + offset;
                int depth = 1;
                while (j < text.size() && depth > 0) {
                    if (text[j] == '"') {
                        for (++j; j < text.size() && text[j] != '"'; ++j)
                            if (text[j] == '\\') ++j;
                        ++j;
                        continue;
                    }
                    if (text[j] == '{') depth++;
                    else if (text[j] == '}') depth--;
                    if (depth > 0) j++;
                }
                if (depth != 0) {
                    literal += text.substr(i);
                    break;
                }
                if (!literal.empty()) {
                    parts.push_back(cppStringLiteral(literal));
                    literal.clear();
                }
                std::string inner, spec;
                ghost_split_fmt(text.substr(i + offset, j - (i + offset)), inner, spec);
                bool blank = true;
                for (char ch : inner)
                    if (!std::isspace(static_cast<unsigned char>(ch))) blank = false;
                std::string x;
                if (blank) {
                    x = "(\"\")";
                } else {
                    std::set<std::string> gen(genericMethodNames_);
                    for (const auto& g : genericFunctions_) gen.insert(g.first);
                    for (const auto& g : declaredGenericFns_) gen.insert(g.first);
                    for (const auto& g : genericParams_) gen.insert(g.first);
                    for (const auto& g : genericIfaceParams_) gen.insert(g.first);
                    AstPtr sub = ghost_ast_parse_expr(inner, e.line, nullptr, &gen);
                    if (!sub) no(e, "expressao em f-string: " + inner);
                    const std::string savedList = listType_, savedMap = mapType_;
                    listType_.clear();
                    mapType_.clear();
                    std::string s = expr(*sub);
                    listType_ = savedList;
                    mapType_ = savedMap;
                    x = "(" + (s.empty() ? std::string("std::string(\"\")") : s) + ")";
                }
                parts.push_back(spec.empty() ? x : "ghost_fmt(" + x + ", \"" + spec + "\")");
                i = j + 1;
                continue;
            }
            literal += text[i++];
        }
        if (!literal.empty() || parts.empty()) parts.push_back(cppStringLiteral(literal));
        if (parts.size() == 1 && parts[0].rfind("std::string(", 0) == 0) return parts[0];
        return "ghost_fstr(" + joinList(parts, ", ") + ")";
    }

    std::string list(const AstNode& e) {
        if (e.attr == "set") no(e, "conjunto literal {a, b}");
        const std::string lt = listType_;
        listType_.clear();
        std::vector<std::string> el;
        for (const auto& k : e.kids) el.push_back(expr(*k));
        if (!lt.empty()) return lt + "{" + joinList(el, ", ") + "}";
        if (el.empty()) no(e, "lista vazia sem tipo");
        return "ghost_list(" + joinList(el, ", ") + ")";
    }

    // {"a": 1, "b": [1, 2]} fora de map<K, V>: texto JSON montado token a token (Parser::parseJsonLiteral)
    void jsonTokens(const AstNode& e, std::string& r) {
        if (e.parens) no(e, "parenteses no JSON");
        switch (e.kind) {
            case AstKind::MapLit:
                for (size_t i = 0; i < e.kids.size(); ++i) {
                    if (i) r += " + std::string(\",\") ";
                    jsonTokens(*e.kids[i]->kids[0], r);
                    r += " + std::string(\":\") ";
                    jsonTokens(*e.kids[i]->kids[1], r);
                }
                return;
            case AstKind::String:
                r += " + std::string(\"\\\"\" + std::string(\"" + e.text + "\") + \"\\\"\") ";
                return;
            case AstKind::Ident: case AstKind::Number:
                r += " + ghost_to_json_string(" + (e.kind == AstKind::Number ? number(e.text) : e.text) + ") ";
                return;
            case AstKind::ListLit:
                if (e.attr == "set") no(e, "JSON");
                r += " + std::string(\"[\") ";
                for (size_t i = 0; i < e.kids.size(); ++i) {
                    if (i) r += " + std::string(\",\") ";
                    jsonTokens(*e.kids[i], r);
                }
                r += " + std::string(\"]\") ";
                return;
            default: no(e, "valor no JSON");
        }
    }
    std::string mapLiteral(const AstNode& e) {
        if (!mapType_.empty()) { // map<K, V> m = {k: v, ...}
            const std::string t = mapType_;
            mapType_.clear();
            if (t.rfind("GhostSet<", 0) == 0) {
                if (!e.kids.empty()) no(e, "conjunto com {chave: valor}");
                return t + "{}";
            }
            std::vector<std::string> pairs;
            for (const auto& en : e.kids) pairs.push_back("{" + expr(*en->kids[0]) + ", " + expr(*en->kids[1]) + "}");
            return t + "{" + joinList(pairs, ", ") + "}";
        }
        std::string r = "std::string(\"{\") ";
        for (size_t i = 0; i < e.kids.size(); ++i) {
            if (i) r += " + std::string(\",\") ";
            jsonTokens(*e.kids[i]->kids[0], r);
            r += " + std::string(\":\") ";
            jsonTokens(*e.kids[i]->kids[1], r);
        }
        return r + " + std::string(\"}\") ";
    }

    std::string slice(const AstNode& e) {
        const std::string obj = expr(*e.kids[0]);
        auto arg = [&](size_t i, const char* none) {
            if (e.kids[i]->kind == AstKind::Empty) return std::string(none);
            return "true, static_cast<long long>(" + expr(*e.kids[i]) + ")";
        };
        const std::string a = arg(1, "false, 0"), b = arg(2, "false, 0"), c = arg(3, "false, 1");
        return "ghost_slice(" + obj + ", GhostSliceArg{" + a + ", " + b + ", " + c + "})";
    }

    // ------------------------------------------------------------ compreensões
    std::string comprehension(const AstNode& e) {
        std::string declared;
        if (e.attr == "map" || e.attr == "set") { // {..} pega o tipo da declaração map/set (ou o de lista)
            declared = !mapType_.empty() ? mapType_ : listType_;
            mapType_.clear();
            listType_.clear();
        } else {
            declared = listType_;
            listType_.clear();
        }
        size_t fors = 0, ifs = 0;
        bool kv = false;
        for (size_t i = 1; i < e.kids.size(); ++i) {
            if (e.kids[i]->kind == AstKind::CompFor) { ++fors; if (!e.kids[i]->attr.empty()) kv = true; }
            else ++ifs;
        }
        if (e.attr == "list" && fors == 1 && ifs <= 1 && !kv && e.kids[1]->kind == AstKind::CompFor) return simpleComp(e, declared);
        return generalComp(e, declared);
    }

    std::string simpleComp(const AstNode& e, const std::string& declared) {
        const AstNode& f = *e.kids[1];
        const std::string coll = expr(*f.kids[0]);
        locals_.emplace_back();
        declare(f.name);
        const std::string head = expr(*e.kids[0]);
        std::string cond;
        if (e.kids.size() > 2) cond = expr(*e.kids[2]->kids[0]);
        locals_.pop_back();
        const std::string id = std::to_string(++uniqueCounter_);
        const std::string src = "ghost_src_" + id, fx = "ghost_fx_" + id, fc = "ghost_fc_" + id, out = "ghost_out_" + id;
        const std::string& var = f.name;
        std::string c = "([&]() { auto&& " + src + " = ghost_iter(" + coll + "); ";
        c += "auto " + fx + " = [&](auto " + var + ") { return " + head + "; }; ";
        if (!cond.empty()) c += "auto " + fc + " = [&](auto " + var + ") { return static_cast<bool>(" + cond + "); }; ";
        const std::string rt = declared.empty() ? "GhostList<std::decay_t<decltype(" + fx + "(*std::begin(" + src + ")))>>" : declared;
        c += rt + " " + out + "; for (const auto& " + var + " : " + src + ") { ";
        if (!cond.empty()) c += "if (" + fc + "(" + var + ")) ";
        c += "ghost_collect(" + out + ", " + fx + "(" + var + ")); } return " + out + "; }())";
        return c;
    }

    std::string generalComp(const AstNode& e, const std::string& declared) {
        const bool isBrace = e.attr != "list";
        const bool isMapComp = e.attr == "map";
        // as variáveis dos for valem na cabeça (preCompVars)
        locals_.emplace_back();
        for (size_t i = 1; i < e.kids.size(); ++i)
            if (e.kids[i]->kind == AstKind::CompFor) {
                declare(e.kids[i]->name);
                if (!e.kids[i]->attr.empty()) declare(e.kids[i]->attr);
            }
        std::string keyExpr, valExpr;
        if (isMapComp) {
            keyExpr = expr(*e.kids[0]->kids[0]);
            valExpr = expr(*e.kids[0]->kids[1]);
        } else {
            valExpr = expr(*e.kids[0]);
        }
        locals_.pop_back();
        struct Clause { std::string v1, v2, coll; std::vector<std::string> conds; };
        std::vector<Clause> clauses;
        locals_.emplace_back();
        for (size_t i = 1; i < e.kids.size(); ++i) {
            const AstNode& k = *e.kids[i];
            if (k.kind == AstKind::CompFor) {
                Clause cl;
                cl.v1 = k.name;
                cl.v2 = k.attr;
                cl.coll = expr(*k.kids[0]);
                declare(cl.v1);
                if (!cl.v2.empty()) declare(cl.v2);
                clauses.push_back(std::move(cl));
            } else {
                if (clauses.empty()) no(k, "compreensao com if antes do for");
                clauses.back().conds.push_back(expr(*k.kids[0]));
            }
        }
        locals_.pop_back();
        (void)isBrace;
        const std::string id = std::to_string(++uniqueCounter_);
        const std::string out = "ghost_out_" + id;
        std::string c = "([&]() { ";
        std::string rt = declared;
        if (rt.empty()) {
            const std::string tyFn = "ghost_ty_" + id;
            c += "auto " + tyFn + " = [&]() { ";
            for (size_t idx = 0; idx < clauses.size(); ++idx) {
                const auto& cl = clauses[idx];
                const std::string p = "ghost_p_" + id + "_" + std::to_string(idx);
                if (cl.v2.empty()) {
                    c += "auto&& " + p + " = ghost_iter(" + cl.coll + "); ";
                    c += "auto " + cl.v1 + " = *std::begin(" + p + "); (void)" + cl.v1 + "; ";
                } else {
                    const std::string kvn = "ghost_pkv_" + id + "_" + std::to_string(idx);
                    c += "auto&& " + p + " = ghost_iter_kv(" + cl.coll + "); ";
                    c += "auto " + kvn + " = *std::begin(" + p + "); ";
                    c += "auto " + cl.v1 + " = " + kvn + ".first; auto " + cl.v2 + " = " + kvn + ".second; ";
                    c += "(void)" + cl.v1 + "; (void)" + cl.v2 + "; ";
                }
            }
            if (isMapComp) c += "return GhostMap<std::decay_t<decltype(" + keyExpr + ")>, std::decay_t<decltype(" + valExpr + ")>>{}; }; ";
            else if (isBrace) c += "return GhostSet<std::decay_t<decltype(" + valExpr + ")>>{}; }; ";
            else c += "return GhostList<std::decay_t<decltype(" + valExpr + ")>>{}; }; ";
            rt = "ghost_rt_" + id;
            c += "using " + rt + " = decltype(" + tyFn + "()); ";
        }
        c += rt + " " + out + "; ";
        for (size_t idx = 0; idx < clauses.size(); ++idx) {
            const auto& cl = clauses[idx];
            if (cl.v2.empty()) {
                c += "for (auto " + cl.v1 + " : ghost_iter(" + cl.coll + ")) { ";
            } else {
                const std::string kvn = "ghost_kv_" + id + "_" + std::to_string(idx);
                c += "for (const auto& " + kvn + " : ghost_iter_kv(" + cl.coll + ")) { ";
                c += "auto " + cl.v1 + " = " + kvn + ".first; auto " + cl.v2 + " = " + kvn + ".second; ";
            }
            for (const auto& cnd : cl.conds) c += "if (!static_cast<bool>(" + cnd + ")) continue; ";
        }
        if (isMapComp) c += "ghost_collect(" + out + ", " + keyExpr + ", " + valExpr + "); ";
        else c += "ghost_collect(" + out + ", " + valExpr + "); ";
        for (size_t idx = 0; idx < clauses.size(); ++idx) c += "} ";
        c += "return " + out + "; }())";
        return c;
    }

    // ------------------------------------------------------------ lambdas (Parser::parseLambda)
    std::string lambda(const AstNode& e) {
        std::string cap = (parsingField_ || staticField_) ? "[]" : ((inInstanceMethod_ && !parsingField_) ? "[=, *this]" : "[=]");
        const bool hasCaptureList = e.attr == "caps";
        bool allowImplicitOuterLocals = true;
        std::set<std::string> explicitCaptured;
        std::vector<const AstNode*> params;
        const AstNode* retType = nullptr;
        std::vector<const AstNode*> caps;
        for (const auto& k : e.kids) {
            if (k->kind == AstKind::Capture) caps.push_back(k.get());
            else if (k->kind == AstKind::Param) params.push_back(k.get());
            else if (k->kind == AstKind::Type && k->attr == "ret") retType = k.get();
        }
        const AstNode& body = *e.kids.back();
        // [capturas]: começa com '[' (o Parser também limpa o tipo de lista pendente)
        if (hasCaptureList) {
            listType_.clear();
            char defMode = 0;
            std::vector<std::string> items;
            bool sawThis = false;
            for (const AstNode* c : caps) {
                if (c->text == "=") { defMode = '='; continue; }
                if (c->text == "&") { defMode = '&'; continue; }
                const bool byRef = c->attr == "ref";
                if (c->text == "this") {
                    sawThis = true;
                    items.push_back(byRef ? "this" : "*this");
                    continue;
                }
                explicitCaptured.insert(c->text);
                items.push_back(byRef ? "&" + c->text + " = " + c->text : c->text + " = " + c->text);
            }
            allowImplicitOuterLocals = defMode != 0;
            std::vector<std::string> finalItems;
            if (defMode == '=') {
                finalItems.push_back("=");
                if (inInstanceMethod_ && !parsingField_ && !sawThis) finalItems.push_back("*this");
            } else if (defMode == '&') {
                finalItems.push_back("&");
            }
            for (const auto& it : items) finalItems.push_back(it);
            cap = "[" + joinList(finalItems, ", ") + "]";
        }
        std::vector<std::string> pTypes, pDecls, pNames;
        bool hasAutoParam = false;
        for (const AstNode* p : params) {
            if (p->kids[0]->text == "var") {
                hasAutoParam = true;
                pTypes.push_back("auto");
                pDecls.push_back("auto " + p->name);
            } else {
                const std::string t = paramType(*p->kids[0]);
                pTypes.push_back(t);
                pDecls.push_back(t + " " + p->name);
            }
            pNames.push_back(p->name);
        }
        const std::string explicitRet = retType ? type(*retType) : "";

        // corpo: contexto próprio (como o Parser salva e restaura)
        std::string* savedBuf = buf_;
        auto savedBlocks = std::move(blocks_);
        blocks_.clear();
        const bool savedField = parsingField_, savedStatic = staticField_;
        parsingField_ = staticField_ = false;
        auto savedHyp = std::move(hyp_);
        hyp_.clear();
        const std::string savedRet = currentReturnType_;
        currentReturnType_ = explicitRet;
        auto savedLocals = locals_;
        if (!allowImplicitOuterLocals) locals_.assign(1, explicitCaptured);
        locals_.emplace_back(pNames.begin(), pNames.end());
        const int savedDepth = depth_;
        std::string bodyCode;
        bool blockBody = false;
        if (body.kind == AstKind::Block) {
            blockBody = true;
            buf_ = &bodyCode;
            depth_ = savedDepth + 1;
            for (const auto& s : body.kids) stmt(*s);
        } else if (body.kind == AstKind::Print) {
            no(body, "lambda com print sem chaves");
        } else {
            bodyCode = " return " + expr(body) + "; ";
        }
        depth_ = savedDepth;
        locals_ = std::move(savedLocals);
        currentReturnType_ = savedRet;
        hyp_ = std::move(savedHyp);
        parsingField_ = savedField;
        staticField_ = savedStatic;
        blocks_ = std::move(savedBlocks);
        buf_ = savedBuf;

        const std::string openClose = blockBody ? ("{\n" + bodyCode + indent() + "}") : ("{" + bodyCode + "}");
        if (hasAutoParam) {
            if (!explicitRet.empty())
                return "ghost_make_poly_fn(" + cap + "(" + joinList(pDecls, ", ") + ") mutable -> " + explicitRet + " " + openClose + ")";
            return "ghost_make_poly_fn(" + cap + "(" + joinList(pDecls, ", ") + ") mutable " + openClose + ")";
        }
        if (!explicitRet.empty())
            return "GhostFn<" + explicitRet + "(" + joinList(pTypes, ", ") + ")>(" + cap + "(" + joinList(pDecls, ", ") +
                   ") mutable -> " + explicitRet + " " + openClose + ")";
        const std::string targs = pTypes.empty() ? "" : ("<" + joinList(pTypes, ", ") + ">");
        return "ghost_make_fn" + targs + "(" + cap + "(" + joinList(pDecls, ", ") + ") mutable " + openClose + ")";
    }

    // ------------------------------------------------------------ chamadas e membros
    std::string call(const AstNode& e, bool raw) {
        const AstNode& callee = *e.kids[0];
        std::string f;
        if (callee.kind == AstKind::Ident && callee.parens == 0) f = ident(callee, true, false, raw);
        else if (callee.kind == AstKind::GenericRef && callee.parens == 0) f = genericRef(callee, true, false);
        else if (callee.kind == AstKind::Member && callee.parens == 0) f = member(callee, raw, true);
        else if (callee.kind == AstKind::Super) no(e, "super(...) fora do construtor");
        else f = expr(callee, raw);
        std::vector<std::string> args;
        for (size_t i = 1; i < e.kids.size(); ++i) args.push_back(expr(*e.kids[i]));
        return f + "(" + joinList(args, ", ") + ")";
    }

    std::string member(const AstNode& e, bool raw, bool callee) {
        const AstNode& obj = *e.kids[0];
        std::string m = e.text;
        if (e.kids.size() > 1) { // obj.metodo<int>(x)
            if (!genericMethodNames_.count(m) || !callee) no(e, "metodo generico ." + m + "<...>");
            const std::string targs = "template " + m + "<" + typeArgs(e, 1) + ">";
            if (obj.parens == 0 && obj.kind == AstKind::Ident && classes_.count(obj.text)) return obj.text + "::" + targs;
            if (obj.parens == 0 && obj.kind == AstKind::GenericRef) return genericRef(obj, false, true) + targs;
            return expr(obj, raw) + "." + targs;
        }
        if (m == "assert") m = "ghost_assert";
        if (m == "union") m = "union_";
        if (obj.parens == 0 && obj.kind == AstKind::Ident && (classes_.count(obj.text) || obj.text == "json")) {
            if (genericParams_.count(obj.text)) no(e, obj.text + " generica: use " + obj.text + "<tipo>.membro");
            const std::string cls = obj.text == "json" ? "GhostJson" : obj.text;
            return cls + "::" + m;  // Classe.membro_static ou GhostJson::parse
        }
        if (obj.parens == 0 && obj.kind == AstKind::GenericRef) return genericRef(obj, false, true) + m;  // Pilha<int>.total
        if (obj.kind == AstKind::Super && obj.parens == 0) {
            if (currentBase_.empty()) no(e, "super sem extends");
            return currentBase_ + "::" + m;
        }
        return expr(obj, raw) + "." + m;
    }

    // raw: este nó começa o comando e o primeiro nome dele sai cru (Parser::parseAction)
    std::string expr(const AstNode& e, bool raw = false) {
        if (e.parens > 0) raw = false;
        std::string s = exprInner(e, raw);
        for (int k = 0; k < e.parens; ++k) s = "(" + s + ")";
        return s;
    }

    std::string exprInner(const AstNode& e, bool raw) {
        switch (e.kind) {
            case AstKind::Number: return number(e.text);
            case AstKind::String: return "std::string(\"" + e.text + "\")";
            case AstKind::FString: return fstring(e);
            case AstKind::Ident: return ident(e, false, false, raw);
            case AstKind::This: return "(*this)";
            case AstKind::Unary: return cat(e.text, expr(*e.kids[0]));
            case AstKind::Postfix: {
                const AstNode& t = *e.kids[0];
                const std::string v = (t.kind == AstKind::Ident && t.parens == 0) ? ident(t, false, true, raw) : expr(t, raw);
                return v + e.text;
            }
            case AstKind::Binary:
                if (e.text == "**") return "ghost_pow(" + expr(*e.kids[0], raw) + ", " + expr(*e.kids[1]) + ")";
                return expr(*e.kids[0], raw) + " " + e.text + " " + expr(*e.kids[1]);
            case AstKind::Assign: {
                const AstNode& t = *e.kids[0];
                const std::string lhs = (t.kind == AstKind::Ident && t.parens == 0) ? ident(t, false, true, raw) : expr(t, raw);
                return lhs + " " + e.text + " " + expr(*e.kids[1]);
            }
            case AstKind::Ternary:
                return expr(*e.kids[0], raw) + " ? " + expr(*e.kids[1]) + " : " + expr(*e.kids[2]);
            case AstKind::Call: return call(e, raw);
            case AstKind::Member: return member(e, raw, false);
            case AstKind::Index: return expr(*e.kids[0], raw) + "[" + expr(*e.kids[1]) + "]";
            case AstKind::Slice:
                if (raw) no(e, "fatia no inicio de um comando");
                return slice(e);
            case AstKind::ListLit: return list(e);
            case AstKind::MapLit: return mapLiteral(e);
            case AstKind::Cast: return "(" + e.text + ")" + expr(*e.kids[0]);
            case AstKind::TypeExpr: return type(*e.kids[0]);
            case AstKind::New: return expr(*e.kids[0], raw);
            case AstKind::GenericRef: return genericRef(e, false, false);
            case AstKind::Lambda: return lambda(e);
            case AstKind::Comprehension: return comprehension(e);
            case AstKind::TryOp: { // r?: err sai da função com o mesmo erro; ok vira o valor
                const std::string operand = expr(*e.kids[0], raw);
                const std::string v = "ghost_try_" + std::to_string(++uniqueCounter_);
                return "({ auto " + v + " = ghost_try_arg(" + operand + "); if (" + v + ".is_err()) return GhostErr{" + v +
                       ".error()}; " + v + ".value(); })";
            }
            case AstKind::Super: no(e, "super sem .metodo");
            default: no(e, std::string("expressao ") + ghost_ast_kind_name(e.kind));
        }
    }

    // ------------------------------------------------------------ nível principal
    void top(const AstNode& n) {
        switch (n.kind) {
            case AstKind::FuncDecl:
                marker(n.line);  // o Parser marca a linha de "function" no main()
                function(n);
                return;
            case AstKind::ClassDecl: marker(n.line); classDecl(n); return;
            case AstKind::InterfaceDecl: marker(n.line); interfaceDecl(n); return;
            case AstKind::CircuitDecl: marker(n.line); circuitDecl(n); return;
            case AstKind::Import: no(n, "import");
            default:
                marker(n.line);
                stmt(n);
        }
    }

    // ------------------------------------------------------------ funções e métodos (Parser::parseFunction)
    void function(const AstNode& f) {
        const bool isCtor = isInsideClass_ && f.has("ctor");
        const bool isStatic = isInsideClass_ && f.has("static");
        std::string funcName = f.name;
        std::string returnType = type(*f.kids[0]);
        if (isCtor) returnType.clear();
        std::string templ, typeAsserts;
        std::vector<std::string> fnTps;
        const auto savedActive = activeTypeParams_;
        bool generic = false;
        for (const auto& c : f.kids)
            if (c->kind == AstKind::TypeParam) generic = true;
        if (generic) {
            // os tipos de retorno/parâmetros podem citar T: o Parser lê o retorno antes, então recalcula
            typeAsserts = typeParams(f, "function " + funcName, fnTps);
            if (isInsideClass_) genericMethodNames_.insert(funcName);
            else genericFunctions_[funcName] = fnTps;
            templ = "template <" + classList(fnTps) + "> ";
        }
        if (!isInsideClass_) userFunctions_.insert(funcName);
        if (funcName == "main") {
            hasMain_ = true;
            funcName = "ghost_main";
        }
        std::vector<std::string> plain, withDefaults, paramNames, paramCppTypes;
        bool firstIsList = false;
        const AstNode* body = nullptr;
        for (const auto& k : f.kids) {
            if (k->kind == AstKind::Block) { body = k.get(); continue; }
            if (k->kind != AstKind::Param) continue;
            const std::string pt = paramType(*k->kids[0]);
            if (plain.empty()) firstIsList = pt.rfind("GhostList<", 0) == 0;
            std::string def;
            if (k->kids.size() > 1) def = expr(*k->kids[1]);
            paramCppTypes.push_back(pt);
            plain.push_back(pt + " " + k->name);
            withDefaults.push_back(pt + " " + k->name + (def.empty() ? "" : " = " + def));
            paramNames.push_back(k->name);
        }
        if (!body) no(f, "funcao sem corpo");
        if (funcName == "ghost_main") {
            mainParams_ = static_cast<int>(plain.size());
            mainArray_ = firstIsList;
        }
        auto savedLocals = std::move(locals_);
        locals_.assign(1, std::set<std::string>(paramNames.begin(), paramNames.end()));

        // super(...) na primeira linha do construtor
        size_t firstStmt = 0;
        std::string init;
        if (isCtor && !body->kids.empty()) {
            const AstNode& s0 = *body->kids[0];
            if (s0.kind == AstKind::ExprStmt && s0.kids[0]->kind == AstKind::Call && s0.kids[0]->kids[0]->kind == AstKind::Super &&
                s0.kids[0]->parens == 0) {
                if (currentBase_.empty()) no(s0, "super(...) sem extends");
                std::vector<std::string> a;
                const AstNode& c = *s0.kids[0];
                for (size_t i = 1; i < c.kids.size(); ++i) a.push_back(expr(*c.kids[i]));
                init = " : " + currentBase_ + "(" + joinList(a, ", ") + ")";
                firstStmt = 1;
            }
        }
        std::string prefix;
        if (wasm_ && funcName != "ghost_main" && !isInsideClass_) prefix = "extern \"C\" EMSCRIPTEN_KEEPALIVE ";
        if (isInsideClass_ && !isCtor) prefix += isStatic ? "static " : (templ.empty() ? "virtual " : "");
        const std::string head = (returnType.empty() ? "" : returnType + " ") + funcName + "(";
        const bool prototype = !isInsideClass_ && returnType != "auto";
        if (prototype) protos_ += templ + prefix + head + joinList(withDefaults, ", ") + ");\n";
        std::string* saved = buf_;
        if (!isInsideClass_) buf_ = &functions_;
        *buf_ += templ + prefix + head + joinList(prototype ? plain : withDefaults, ", ") + ")" + init + " {\n";
        *buf_ += typeAsserts;
        const std::string savedRet = currentReturnType_;
        const bool savedInInstance = inInstanceMethod_;
        inInstanceMethod_ = isInsideClass_ && !isStatic;
        currentReturnType_ = returnType;
        const bool savedInFunction = inFunction_;
        inFunction_ = true;
        for (size_t i = firstStmt; i < body->kids.size(); ++i) {
            marker(body->kids[i]->line);
            stmt(*body->kids[i]);
        }
        inFunction_ = savedInFunction;
        currentReturnType_ = savedRet;
        inInstanceMethod_ = savedInInstance;
        locals_ = std::move(savedLocals);
        *buf_ += "}\n\n";
        if (!fnTps.empty() && fileUsesPolyFn_ && !isCtor)
            polyOverload(fnTps, paramCppTypes, plain, withDefaults, paramNames, prototype, prefix, head, funcName, isStatic);
        activeTypeParams_ = savedActive;
        buf_ = isInsideClass_ ? saved : &mainCode_;
    }

    // Sobrecarga auxiliar para funções/métodos genéricos com parâmetros fn<...> receberem lambdas (x) => ...
    // ou funções genéricas pelo nome direto (Parser::parseFunction, quando o arquivo usa valores polimórficos).
    void polyOverload(const std::vector<std::string>& fnTps, const std::vector<std::string>& paramCppTypes,
                      const std::vector<std::string>& plain, const std::vector<std::string>& withDefaults,
                      const std::vector<std::string>& paramNamesList, bool prototype, const std::string& prefix,
                      const std::string& head, const std::string& funcName, bool isStatic) {
        auto hasWord = [](const std::string& hay, const std::string& w) {
            for (size_t pos = hay.find(w); pos != std::string::npos; pos = hay.find(w, pos + 1)) {
                const bool l = pos == 0 || !wordChar(hay[pos - 1]);
                const size_t e = pos + w.size();
                const bool r = e >= hay.size() || !wordChar(hay[e]);
                if (l && r) return true;
            }
            return false;
        };
        auto splitGhostFn = [](const std::string& pt, std::string& fRet, std::string& fArgs) -> bool {
            if (pt.rfind("GhostFn<", 0) != 0 || pt.back() != '>') return false;
            const std::string inner = pt.substr(8, pt.size() - 9);
            if (inner.empty() || inner.back() != ')') return false;
            int pd = 0;
            size_t openAt = std::string::npos;
            for (size_t k = inner.size(); k-- > 0;) {
                if (inner[k] == ')') pd++;
                else if (inner[k] == '(' && --pd == 0) { openAt = k; break; }
            }
            if (openAt == std::string::npos || openAt == 0) return false;
            fRet = inner.substr(0, openAt);
            fArgs = inner.substr(openAt + 1, inner.size() - openAt - 2);
            return true;
        };
        bool anyFnParam = false;
        for (const auto& pt : paramCppTypes)
            if (pt.rfind("GhostFn<", 0) == 0) { anyFnParam = true; break; }
        if (!anyFnParam) return;
        std::set<std::string> directTps;
        for (const auto& tp : fnTps)
            for (const auto& pt : paramCppTypes)
                if (pt.rfind("GhostFn<", 0) != 0 && hasWord(pt, tp)) directTps.insert(tp);
        std::vector<std::string> tplProto, tplDef, callParamsProto, callParamsDef, forwardArgs, notGhostFnConds;
        for (const auto& tp : fnTps)
            if (directTps.count(tp)) {
                tplProto.push_back("class " + tp);
                tplDef.push_back("class " + tp);
            }
        for (size_t k = 0; k < paramCppTypes.size(); ++k) {
            if (paramCppTypes[k].rfind("GhostFn<", 0) == 0) {
                const std::string cname = "GhostCallable_" + std::to_string(k);
                tplProto.push_back("class " + cname);
                tplDef.push_back("class " + cname);
                notGhostFnConds.push_back("!ghost_detail::is_ghost_fn<std::decay_t<" + cname + ">>::value");
                const size_t eqPos = withDefaults[k].find('=');
                const std::string defPart = (!prototype && eqPos != std::string::npos) ? withDefaults[k].substr(eqPos - 1) : "";
                const std::string protoDef = (prototype && eqPos != std::string::npos) ? withDefaults[k].substr(eqPos - 1) : "";
                callParamsProto.push_back(cname + "&& " + paramNamesList[k] + protoDef);
                callParamsDef.push_back(cname + "&& " + paramNamesList[k] + defPart);
                forwardArgs.push_back(paramCppTypes[k] + "(std::forward<" + cname + ">(" + paramNamesList[k] + "))");
            } else {
                callParamsProto.push_back(withDefaults[k]);
                callParamsDef.push_back(prototype ? plain[k] : withDefaults[k]);
                forwardArgs.push_back(paramNamesList[k]);
            }
        }
        bool canDeduceAll = true;
        for (const auto& tp : fnTps) {
            if (directTps.count(tp)) continue;
            std::string deducedExpr;
            for (size_t k = 0; k < paramCppTypes.size(); ++k) {
                std::string fRet, fArgs;
                if (splitGhostFn(paramCppTypes[k], fRet, fArgs) && fRet == tp) {
                    const std::string cname = "GhostCallable_" + std::to_string(k);
                    deducedExpr = "std::decay_t<std::invoke_result_t<std::decay_t<" + cname + ">&" + (fArgs.empty() ? "" : ", " + fArgs) + ">>";
                    break;
                }
            }
            if (deducedExpr.empty()) { canDeduceAll = false; break; }
            tplProto.push_back("class " + tp + " = " + deducedExpr);
            tplDef.push_back("class " + tp + (prototype ? "" : " = " + deducedExpr));
            directTps.insert(tp);
        }
        if (!canDeduceAll || notGhostFnConds.empty()) return;
        std::string condOr;
        for (size_t k = 0; k < notGhostFnConds.size(); ++k) condOr += (k ? " || " : "") + notGhostFnConds[k];
        tplProto.push_back("class GhostEn_ = std::enable_if_t<(" + condOr + ")>");
        tplDef.push_back("class GhostEn_" + (prototype ? std::string() : " = std::enable_if_t<(" + condOr + ")>"));
        std::string tpList;
        for (size_t k = 0; k < fnTps.size(); ++k) tpList += (k ? ", " : "") + fnTps[k];
        const std::string secPrefix = isInsideClass_ ? (isStatic ? "static " : "") : prefix;
        if (prototype) protos_ += "template <" + joinList(tplProto, ", ") + "> " + secPrefix + head + joinList(callParamsProto, ", ") + ");\n";
        *buf_ += "template <" + joinList(tplDef, ", ") + "> " + secPrefix + head + joinList(callParamsDef, ", ") + ") {\n";
        *buf_ += "    return " + funcName + "<" + tpList + ">(" + joinList(forwardArgs, ", ") + ");\n}\n\n";
    }

    // ------------------------------------------------------------ classes (Parser::parseClass)
    void classDecl(const AstNode& c) {
        const std::string& className = c.name;
        std::vector<std::string> tparams;
        const auto savedActive = activeTypeParams_;
        std::string classAsserts;
        bool generic = false;
        for (const auto& k : c.kids)
            if (k->kind == AstKind::TypeParam) generic = true;
        if (generic) {
            classAsserts = typeParams(c, "class " + className, tparams);
            genericParams_[className] = tparams;
        }
        std::string base;
        std::vector<std::string> impls;
        for (const auto& k : c.kids) {
            if (k->kind == AstKind::Type && k->attr == "extends") {
                if (!classes_.count(k->text)) no(c, "classe base desconhecida");
                base = genericParams_.count(k->text) ? type(*k) : k->text;
            } else if (k->kind == AstKind::Type && k->attr == "implements") {
                if (!ifaces_.count(k->text)) no(c, "interface desconhecida");
                impls.push_back(genericIfaceParams_.count(k->text) ? type(*k) : k->text);
            }
        }
        classes_.insert(className);
        std::string* saved = buf_;
        buf_ = &classesCode_;
        std::string bases = base.empty() ? "" : "public " + base;
        for (const auto& i : impls) bases += (bases.empty() ? "" : ", ") + std::string("public virtual ghost_iface_") + i;
        std::string templ;
        if (!tparams.empty()) templ = "template <" + classList(tparams) + ">\n";
        *buf_ += templ + "class " + className + (bases.empty() ? "" : " : " + bases) + " {\npublic:\n" + classAsserts;
        const bool savedInside = isInsideClass_;
        isInsideClass_ = true;
        currentClass_ = className;
        currentBase_ = base;
        for (const auto& kp : c.kids) {
            const AstNode& m = *kp;
            if (m.kind == AstKind::Empty && m.text == "section") {
                *buf_ += m.name + ":\n";
            } else if (m.kind == AstKind::FuncDecl) {
                function(m);
            } else if (m.kind == AstKind::Domain) {
                if (m.text == "qlayer") no(m, "campo qlayer");
                if (m.has("static")) no(m, "campo static qreg");
                domain(m);
            } else if (m.kind == AstKind::VarDecl) {
                field(m);
            }
        }
        *buf_ += "};\n\n";
        isInsideClass_ = savedInside;
        currentClass_.clear();
        currentBase_.clear();
        activeTypeParams_ = savedActive;
        (void)saved;
        buf_ = &mainCode_;  // o Parser volta para o main depois da classe
    }

    void field(const AstNode& s) {
        const bool isStatic = s.has("static");
        parsingField_ = true;
        staticField_ = isStatic;
        const std::string t = type(*s.kids[0]);
        std::string val;
        const bool hasInit = s.kids.size() > 1;
        if (hasInit) val = initValue(s, t);
        globalVars_.insert(s.name);
        line(std::string(isStatic ? "static inline " : "") + t + " " + s.name + (hasInit ? " = " + val : std::string("{}")) + ";");
        parsingField_ = staticField_ = false;
    }

    // ------------------------------------------------------------ interfaces (Parser::parseInterface)
    std::vector<std::pair<std::string, MethodSig>> ifaceMethods(const std::string& iface) {
        std::vector<std::pair<std::string, MethodSig>> out;
        std::function<void(const std::string&, int)> walk = [&](const std::string& spec, int depth) {
            if (depth > 64) return;
            std::string baseName;
            std::vector<std::string> targs;
            splitCppTemplateSpec(spec, baseName, targs);
            auto it = ifaces_.find(baseName);
            if (it == ifaces_.end()) return;
            for (const auto& b : it->second.bases) walk(substCppTypeWords(b, it->second.tparams, targs), depth + 1);
            for (const auto& m : it->second.methods) {
                MethodSig sub = m.second;
                sub.ret = substCppTypeWords(sub.ret, it->second.tparams, targs);
                for (auto& p : sub.params) p = substCppTypeWords(p, it->second.tparams, targs);
                bool found = false;
                for (auto& existing : out)
                    if (existing.first == m.first) {
                        existing.second = sub;
                        found = true;
                        break;
                    }
                if (!found) out.push_back({m.first, sub});
            }
        };
        walk(iface, 0);
        return out;
    }

    void interfaceDecl(const AstNode& n) {
        const std::string& name = n.name;
        IfaceInfo info;
        const auto savedActive = activeTypeParams_;
        bool generic = false;
        for (const auto& k : n.kids)
            if (k->kind == AstKind::TypeParam) generic = true;
        if (generic) {
            info.typeAsserts = typeParams(n, "interface " + name, info.tparams);
            genericIfaceParams_[name] = info.tparams;
        }
        for (const auto& k : n.kids)
            if (k->kind == AstKind::Type && k->attr == "extends") {
                if (!ifaces_.count(k->text)) no(n, "interface base desconhecida");
                info.bases.push_back(genericIfaceParams_.count(k->text) ? type(*k) : k->text);
            }
        ifaces_[name] = info;
        std::set<std::string> knownMethods;
        for (const auto& b : info.bases)
            for (const auto& bm : ifaceMethods(b)) knownMethods.insert(bm.first);
        for (const auto& k : n.kids)
            if (k->kind == AstKind::FuncDecl) knownMethods.insert(k->name);
        for (const auto& kp : n.kids) {
            const AstNode& m = *kp;
            if (m.kind != AstKind::FuncDecl) continue;
            for (const auto& c : m.kids)
                if (c->kind == AstKind::TypeParam) no(m, "metodo generico em interface");
            MethodSig sig;
            sig.ret = type(*m.kids[0]);
            const AstNode* body = nullptr;
            for (const auto& c : m.kids) {
                if (c->kind == AstKind::Param) {
                    sig.params.push_back(paramType(*c->kids[0]));
                    sig.paramNames.push_back(c->name);
                } else if (c->kind == AstKind::Block) {
                    body = c.get();
                }
            }
            if (body) {
                sig.hasDefault = true;
                std::string bodyBuf;
                std::string* savedBuf = buf_;
                const std::string savedRet = currentReturnType_;
                const bool savedInInst = inInstanceMethod_;
                const std::string savedIface = currentIface_;
                auto savedIfaceMethods = currentIfaceMethodNames_;
                auto savedLocals = std::move(locals_);
                locals_.assign(1, std::set<std::string>(sig.paramNames.begin(), sig.paramNames.end()));
                buf_ = &bodyBuf;
                currentReturnType_ = sig.ret;
                inInstanceMethod_ = true;
                currentIface_ = name;
                currentIfaceMethodNames_ = knownMethods;
                const bool savedInFunction = inFunction_;
                inFunction_ = true;
                ++depth_;
                for (const auto& s : body->kids) stmt(*s);
                --depth_;
                inFunction_ = savedInFunction;
                currentIfaceMethodNames_ = std::move(savedIfaceMethods);
                currentIface_ = savedIface;
                inInstanceMethod_ = savedInInst;
                currentReturnType_ = savedRet;
                locals_ = std::move(savedLocals);
                buf_ = savedBuf;
                sig.defaultBody = std::move(bodyBuf);
            }
            ifaces_[name].methods.push_back({m.name, sig});
        }
        activeTypeParams_ = savedActive;
        info = ifaces_[name];
        auto params = [](const MethodSig& s, int nameMode) {
            std::string r;
            for (size_t k = 0; k < s.params.size(); ++k) {
                r += (k ? ", " : "") + s.params[k];
                if (nameMode == 1) r += " a" + std::to_string(k);
                else if (nameMode == 2) r += " " + (k < s.paramNames.size() ? s.paramNames[k] : ("a" + std::to_string(k)));
            }
            return r;
        };
        const bool isGen = !info.tparams.empty();
        std::string templDecl, targsStr;
        if (isGen) {
            templDecl = "template <" + classList(info.tparams) + ">\n";
            targsStr = "<" + joinList(info.tparams, ", ") + ">";
        }
        const std::string fullSelf = name + targsStr;
        const std::string I = "ghost_iface_" + fullSelf;
        const std::string objTp = isGen ? "GhostObj_" : "T";
        std::string c = (isGen ? templDecl : std::string()) + "class " + name + ";\n";
        std::string bases;
        for (const auto& b : info.bases) bases += (bases.empty() ? " : " : ", ") + std::string("public virtual ghost_iface_") + b;
        c += (isGen ? templDecl : std::string()) + "class ghost_iface_" + name + bases + " {\npublic:\n" + info.typeAsserts;
        c += "    virtual ~ghost_iface_" + name + "() = default;\n";
        for (const auto& m : info.methods) {
            if (m.second.hasDefault)
                c += "    virtual " + m.second.ret + " " + m.first + "(" + params(m.second, 2) + ") {\n" + m.second.defaultBody + "    }\n";
            else
                c += "    virtual " + m.second.ret + " " + m.first + "(" + params(m.second, 0) + ") = 0;\n";
        }
        c += "};\n";
        c += (isGen ? templDecl : std::string()) + "class " + name + " : public virtual " + I + " {\n" + info.typeAsserts;
        c += "    std::shared_ptr<" + I + "> ghost_p_;\n";
        c += "    std::shared_ptr<" + I + "> (*ghost_clone_)(const " + I + "&) = nullptr;\n";
        c += "    " + I + "& ghost_obj() const {\n";
        c += "        if (!ghost_p_) throw std::runtime_error(\"" + name + " sem objeto: guarde um objeto antes de chamar um metodo\");\n";
        c += "        return *ghost_p_;\n    }\npublic:\n";
        c += "    " + name + "() = default;\n";
        c += "    template <class " + objTp + ", class = std::enable_if_t<std::is_base_of_v<" + I + ", " + objTp + "> && !std::is_same_v<" +
             objTp + ", " + fullSelf + ">>>\n";
        c += "    " + name + "(const " + objTp + "& v) : ghost_p_(std::make_shared<" + objTp + ">(v)),\n";
        c += "        ghost_clone_([](const " + I + "& x) -> std::shared_ptr<" + I + "> { return std::make_shared<" + objTp +
             ">(dynamic_cast<const " + objTp + "&>(x)); }) {}\n";
        c += "    " + name + "(const " + name + "& o) : " + I + "(), ghost_p_(o.ghost_p_ ? o.ghost_clone_(*o.ghost_p_) : nullptr), ghost_clone_(o.ghost_clone_) {}\n";
        c += "    " + name + "& operator=(const " + name + "& o) {\n";
        c += "        if (this != &o) { ghost_p_ = o.ghost_p_ ? o.ghost_clone_(*o.ghost_p_) : nullptr; ghost_clone_ = o.ghost_clone_; }\n";
        c += "        return *this;\n    }\n";
        for (const auto& m : ifaceMethods(fullSelf)) {
            std::string args;
            for (size_t k = 0; k < m.second.params.size(); ++k) args += (k ? ", " : "") + std::string("a") + std::to_string(k);
            c += "    " + m.second.ret + " " + m.first + "(" + params(m.second, 1) + ") override { return ghost_obj()." + m.first + "(" + args + "); }\n";
        }
        c += "};\n\n";
        classesCode_ += c;
    }

    // ------------------------------------------------------------ circuit (Parser::parseCircuit)
    void circuitDecl(const AstNode& n) {
        const std::string& name = n.name;
        std::string templ = "template <class GhostQ_";
        std::string params = "GhostQ_& ghost_q_";
        std::string argNames;
        int untyped = 0;
        const AstNode* body = nullptr;
        for (const auto& p : n.kids) {
            if (p->kind == AstKind::Block) { body = p.get(); continue; }
            if (p->kind != AstKind::Param) continue;
            std::string t = p->kids[0]->text;
            if (t == "var") {
                t = "GhostT" + std::to_string(untyped++) + "_";
                templ += ", class " + t;
            }
            params += ", " + t + " " + p->name;
            argNames += ", " + p->name;
        }
        templ += ">\n";
        const std::string templCtrl = "template <class GhostQ_, class GhostC_" + templ.substr(std::string("template <class GhostQ_").size());
        const std::string paramsCtrl = "GhostQ_& ghost_q_, GhostC_ ghost_ctrl_" + params.substr(std::string("GhostQ_& ghost_q_").size());
        protos_ += templ + "void " + name + "(" + params + ");\n";
        protos_ += templ + "void " + name + "_dg(" + params + ");\n";
        protos_ += templCtrl + "void " + name + "_ctrl(" + paramsCtrl + ");\n";
        protos_ += templCtrl + "void " + name + "_dg_ctrl(" + paramsCtrl + ");\n";
        buf_ = &functions_;
        circuitTarget_ = "ghost_q_";
        currentCircuit_ = name;
        *buf_ += "// circuit " + name + "\n" + templ + "void " + name + "(" + params + ") {\n";
        const bool savedNoMarkers = noMarkers_;
        noMarkers_ = true;  // o Parser não marca linhas dentro de circuit
        const bool savedInFunction = inFunction_;
        inFunction_ = true;
        auto savedLocals = std::move(locals_);
        locals_.assign(1, {});
        for (const auto& s : body->kids) {
            switch (s->kind) {
                case AstKind::ExprStmt: case AstKind::VarDecl: case AstKind::If: case AstKind::For: case AstKind::While:
                case AstKind::Print: case AstKind::Return: case AstKind::Empty:
                    break;
                default: no(*s, "comando dentro de circuit");
            }
            stmt(*s);
        }
        locals_ = std::move(savedLocals);
        inFunction_ = savedInFunction;
        noMarkers_ = savedNoMarkers;
        *buf_ += "}\n\n";
        *buf_ += "// circuit " + name + "_dg: inverso (portas inversas em ordem reversa)\n" + templ + "void " + name + "_dg(" + params + ") {\n"
                 "    auto ghost_rec_ = ghost_begin_inverse(ghost_q_);\n"
                 "    " + name + "(ghost_rec_" + argNames + ");\n"
                 "    ghost_end_inverse(ghost_q_, ghost_rec_);\n}\n\n";
        for (const std::string& variant : {std::string(""), std::string("_dg")}) {
            *buf_ += "// circuit " + name + variant + "_ctrl: versao controlada\n" + templCtrl + "void " + name + variant + "_ctrl(" + paramsCtrl + ") {\n"
                     "    auto ghost_rec_ = ghost_begin_record(ghost_q_);\n"
                     "    " + name + variant + "(ghost_rec_" + argNames + ");\n"
                     "    ghost_end_controlled(ghost_q_, ghost_rec_, ghost_ctrl_);\n}\n\n";
        }
        circuitTarget_.clear();
        currentCircuit_.clear();
        buf_ = &mainCode_;
    }

    std::string circuitOf(const std::string& n) const {
        std::string b = n;
        if (endsWith(b, "_ctrl")) b = b.substr(0, b.size() - 5);
        if (circuits_.count(b)) return b;
        if (endsWith(b, "_dg") && circuits_.count(b.substr(0, b.size() - 3))) return b.substr(0, b.size() - 3);
        return "";
    }

    // ------------------------------------------------------------ comandos de IA/quântico
    // Tokens copiados como estão (parseQreg, parseState): sem as trocas de translateExpr.
    // quoteStrings: textos viram std::string("...") (depois de '='); sem isso, texto é recusado.
    std::string raw(const AstNode& e, bool quoteStrings) {
        std::string s;
        switch (e.kind) {
            case AstKind::Ident: s = e.text; break;
            case AstKind::This: s = "(*this)"; break;
            case AstKind::Number: s = number(e.text); break;
            case AstKind::String:
                if (!quoteStrings) no(e, "texto nos argumentos de um comando");
                s = "std::string(\"" + e.text + "\")";
                break;
            case AstKind::Unary: s = e.text + raw(*e.kids[0], quoteStrings); break;
            case AstKind::Binary: s = raw(*e.kids[0], quoteStrings) + e.text + raw(*e.kids[1], quoteStrings); break;
            case AstKind::Member: s = raw(*e.kids[0], quoteStrings) + "." + e.text; break;
            case AstKind::Index: s = raw(*e.kids[0], quoteStrings) + "[" + raw(*e.kids[1], quoteStrings) + "]"; break;
            case AstKind::Call: {
                std::vector<std::string> a;
                for (size_t i = 1; i < e.kids.size(); ++i) a.push_back(raw(*e.kids[i], quoteStrings));
                s = raw(*e.kids[0], quoteStrings) + "(" + joinList(a, ",") + ")";
                break;
            }
            default: no(e, "expressao no comando");
        }
        for (int k = 0; k < e.parens; ++k) s = "(" + s + ")";
        return s;
    }
    static const std::string& nameArg(const AstNode& d, size_t i) {
        if (i >= d.kids.size() || d.kids[i]->kind != AstKind::Ident || d.kids[i]->parens) no(d, "comando " + d.text);
        return d.kids[i]->text;
    }
    static const std::string& stringArg(const AstNode& d, size_t i) {
        if (i >= d.kids.size() || d.kids[i]->kind != AstKind::String) no(d, "comando " + d.text);
        return d.kids[i]->text;
    }
    void flattenChain(const AstNode& e, std::vector<std::string>& out) {
        if (e.kind == AstKind::Binary && e.text == ">>" && e.parens == 0) {
            flattenChain(*e.kids[0], out);
            flattenChain(*e.kids[1], out);
        } else if (e.kind == AstKind::Ident && e.parens == 0) {
            out.push_back(e.text);
        } else {
            no(e, "network (use camada >> camada)");
        }
    }

    void domain(const AstNode& d) {
        const std::string& k = d.text;
        const bool hasInit = d.attr == "=";
        if (k == "state") {
            if (!hasInit || d.kids.size() != 1) no(d, "state");
            const AstNode& v = *d.kids[0];
            if (v.kind == AstKind::ListLit && v.parens == 0) {
                std::vector<std::string> vals;
                for (const auto& x : v.kids) {
                    if (x->kind != AstKind::Number || x->parens) no(d, "state com valores que nao sao numeros");
                    vals.push_back(number(x->text));
                }
                const std::string n = std::to_string(vals.size());
                if (isMain()) {
                    globals_ += "GhostMatrix " + d.name + ";\n";
                    line(d.name + " = GhostMatrix(" + n + ", 1);");
                } else {
                    line("GhostMatrix " + d.name + "(" + n + ", 1);");
                }
                for (size_t i = 0; i < vals.size(); ++i)
                    line(d.name + "(" + std::to_string(i) + ", 0) = GhostNumberG(" + vals[i] + ", 0.0, 0.0);");
                *buf_ += "\n";
                return;
            }
            const AstNode* first = &v;
            while (first->kind == AstKind::Call || first->kind == AstKind::Member || first->kind == AstKind::Binary ||
                   first->kind == AstKind::Index)
                first = first->kids[0].get();
            if (first->kind != AstKind::Ident || first->parens || v.parens) no(d, "state");
            const std::string rhs = raw(v, true);
            if (isMain()) {
                globals_ += "GhostMatrix " + d.name + ";\n";
                line(d.name + " = " + rhs + ";");
            } else {
                line("GhostMatrix " + d.name + " = " + rhs + ";");
            }
            return;
        }
        if (k == "gate") {
            if (hasInit) {
                if (d.kids.size() != 1 || d.kids[0]->kind != AstKind::Ident) no(d, "gate");
                static const std::pair<const char*, const char*> gates[] = {
                    {"hadamard", "GhostMatrix::Hadamard()"}, {"paulix", "GhostMatrix::PauliX()"},
                    {"pauliy", "GhostMatrix::PauliY()"}, {"pauliz", "GhostMatrix::PauliZ()"}};
                std::string m;
                for (const auto& g : gates)
                    if (d.kids[0]->text == g.first) m = g.second;
                if (m.empty()) no(d, "gate");
                if (isMain()) {
                    globals_ += "GhostMatrix " + d.name + ";\n";
                    line(d.name + " = " + m + ";");
                } else {
                    line("GhostMatrix " + d.name + " = " + m + ";");
                }
                return;
            }
            if (d.kids.size() != 2 || d.kids[0]->kind != AstKind::Number || d.kids[1]->kind != AstKind::Number) no(d, "gate");
            const std::string r = d.kids[0]->text, c = d.kids[1]->text, n = d.name;
            if (isMain()) {
                globals_ += "GhostMatrix " + n + ";\n";
                line(n + " = GhostMatrix(" + r + ", " + c + ");");
            } else {
                line("GhostMatrix " + n + "(" + r + ", " + c + ");");
            }
            line("double limit_" + n + " = std::sqrt(6.0 / (" + r + " + " + c + "));");
            // nomes com prefixo ghost_: uma matriz chamada r, c ou val não pode ficar escondida pelos laços
            line("for (size_t ghost_r = 0; ghost_r < " + n + ".getRows(); ++ghost_r) {");
            line("    for (size_t ghost_c = 0; ghost_c < " + n + ".getCols(); ++ghost_c) {");
            line("        double ghost_val = (((double)rand() / RAND_MAX) * 2.0 - 1.0) * limit_" + n + ";");
            line("        " + n + "(ghost_r, ghost_c) = GhostNumberG(ghost_val, 0.0, 0.0);");
            line("    }");
            line("}");
            *buf_ += "\n";
            return;
        }
        if (k == "matrix" || k == "GhostMatrix") {
            const std::string t = "GhostMatrix";
            std::string init;
            if (hasInit) {
                if (d.kids.size() != 1) no(d, k);
                init = raw(*d.kids[0], true);
            } else {
                std::vector<std::string> a;
                for (const auto& x : d.kids) a.push_back(raw(*x, false));
                if (a.empty()) init = t + "()";
                else init = t + "(" + joinList(a, ", ") + ")";
            }
            if (isMain()) {
                globals_ += t + " " + d.name + ";\n";
                line(d.name + " = " + init + ";");
            } else {
                line(t + " " + d.name + " = " + init + ";");
            }
            return;
        }
        if (k == "qreg" || k == "qcircuit" || k == "qlayer") {
            const std::string t = k == "qcircuit" ? "GhostQCircuit" : k == "qlayer" ? "GhostQLayer" : "GhostQReg";
            if (k == "qlayer") quantumLayers_.insert(d.name);
            std::string init;
            if (hasInit) {
                if (d.kids.size() != 1) no(d, k);
                init = raw(*d.kids[0], true);
            } else {
                if (d.kids.empty()) no(d, k);
                std::vector<std::string> a;
                for (const auto& x : d.kids) a.push_back(raw(*x, false));
                init = t + "(" + joinList(a, ",") + ")";
            }
            if (isMain()) {
                globals_ += t + " " + d.name + ";\n";
                line(d.name + " = " + init + ";");
            } else {
                line(t + " " + d.name + " = " + init + ";");
            }
            return;
        }
        if (k == "network") {
            if (!hasInit || d.kids.size() != 1) no(d, "network");
            std::vector<std::string> layers;
            flattenChain(*d.kids[0], layers);
            bool hybrid = false;
            for (const auto& l : layers)
                if (quantumLayers_.count(l)) hybrid = true;
            std::string refs = "{";
            for (size_t i = 0; i < layers.size(); ++i) refs += (i ? ", &" : "&") + layers[i];
            refs += "}";
            const std::string& n = d.name;
            if (hybrid) {
                *buf_ += "    // Rede hibrida classico-quantica: " + n + "\n";
                if (isMain()) {
                    globals_ += "GhostHybridNetwork " + n + ";\n";
                    line(n + " = GhostHybridNetwork(\"" + n + "\", " + refs + ");");
                } else {
                    line("GhostHybridNetwork " + n + "(\"" + n + "\", " + refs + ");");
                }
                line("std::cout << \"[Ghost OS] Rede hibrida '" + n + "' montada: \" << " + n + ".summary() << std::endl;");
                *buf_ += "\n";
                return;
            }
            deepNetworks_.insert(n);
            *buf_ += "    // Estrutura Deep Ghost: " + n + "\n";
            if (isMain()) {
                globals_ += "GhostNetwork* " + n + ";\n";
                line(n + " = new GhostNetwork(\"" + n + "\", " + refs + ");");
            } else {
                line("GhostNetwork* " + n + " = new GhostNetwork(\"" + n + "\", " + refs + ");");
            }
            line("std::cout << \"[Ghost OS] Rede Profunda '" + n + "' montada com " + std::to_string(layers.size()) +
                 " camadas.\" << std::endl;");
            *buf_ += "\n";
            return;
        }
        if (k == "show") {
            const std::string& n = nameArg(d, 0);
            line("std::cout << \"\\nEstado da estrutura '\" << \"" + n + "\" << \"':\" << std::endl;");
            line(n + ".display();");
            *buf_ += "\n";
            return;
        }
        if (k == "activate") {
            const std::string& n = nameArg(d, 0);
            const std::string& f = nameArg(d, 1);
            static const std::pair<const char*, const char*> acts[] = {
                {"relu", "activateReLU"}, {"sigmoid", "activateSigmoid"}, {"sin", "activateSin"},
                {"cos", "activateCos"}, {"tan", "activateTan"},
                {"tanh", "activateTanh"}, {"leaky_relu", "activateLeakyReLU"},
                {"gelu", "activateGELU"}, {"softmax", "activateSoftmax"}};
            std::string m;
            for (const auto& a : acts)
                if (f == a.first) m = a.second;
            if (m.empty()) no(d, "activate");
            line(n + "." + m + "();");
            line("std::cout << \"[Ghost OS] Colapso Quântico/Matemático (" + f + ") aplicado em '" + n + "'.\" << std::endl;");
            *buf_ += "\n";
            return;
        }
        if (k == "apply") {
            const std::string& g = nameArg(d, 0);
            const std::string& st = nameArg(d, 1);
            line(st + " = " + g + " * " + st + ";");
            line("std::cout << \"[Colapso Quantico] Porta '" + g + "' aplicada sobre '" + st + "'.\" << std::endl;");
            *buf_ += "\n";
            return;
        }
        if (k == "load") {
            if (d.kids.size() != 2) no(d, "load \"arquivo\" (import em tempo de compilacao)");
            const std::string& n = nameArg(d, 0);
            const std::string& f = stringArg(d, 1);
            line(n + ".loadCSV(\"" + f + "\");");
            line("std::cout << \"[Ghost OS] Dados carregados de " + f + "\" << std::endl;");
            *buf_ += "\n";
            return;
        }
        if (k == "server") {
            if (!hasInit || d.kids.size() != 1 || d.kids[0]->kind != AstKind::Number) no(d, "server");
            const std::string decl = "GhostServer " + d.name + "(" + d.kids[0]->text + ");";
            if (isMain()) globals_ += decl + "\n";
            else line(decl);
            return;
        }
        if (k == "sleep") {
            if (d.kids.size() != 1) no(d, "sleep");
            line("std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(" + expr(*d.kids[0]) + ")));");
            return;
        }
        if (k == "mutex") { line("std::mutex " + d.name + ";"); return; }
        if (k == "lock") { line(nameArg(d, 0) + ".lock();"); return; }
        if (k == "unlock") { line(nameArg(d, 0) + ".unlock();"); return; }
        if (k == "input") {
            line("ghost_print(\"" + stringArg(d, 0) + "\");");
            line("ghost_input(" + nameArg(d, 1) + ");");
            return;
        }
        if (k == "pragma") {
            std::string dev = nameArg(d, 0);
            if (dev == "gpu") dev = "cuda";
            if (dev != "cuda" && dev != "cpu" && dev != "auto") no(d, "pragma");
            line("use_device(\"" + dev + "\");");
            return;
        }
        if (k == "swarm") {
            const std::string& ds = stringArg(d, 0);
            line("std::cout << \"[Ghost Swarm] Iniciando orquestracao federada...\" << std::endl;");
            *buf_ += "    // Integracao C++ injetada para rodar o Ghost Swarm\n";
            line("std::string cmd = \"start .\\\\ghost_swarm.exe \" + std::string(\"" + ds + "\");");
            line("system(cmd.c_str());");
            line("std::cout << \"[Ghost Swarm] Modelo global recebido e convergido!\" << std::endl;");
            *buf_ += "\n";
            return;
        }
        if (k == "=>") { arrow(d); return; }
        no(d, "comando " + k);
    }

    // rede => load("x"); rede => save("x"); m => optimize(ini, alvo, epochs=N, lr=X[, opt="adam"]);
    void arrow(const AstNode& d) {
        const AstNode& c = *d.kids[0];
        if (c.kind != AstKind::Call || c.kids[0]->kind != AstKind::Ident) no(d, "=>");
        const std::string& f = c.kids[0]->text;
        const std::string op = deepNetworks_.count(d.name) ? "->" : ".";
        if (f == "load" || f == "save") {
            if (c.kids.size() != 2 || c.kids[1]->kind != AstKind::String) no(d, "=> " + f);
            *buf_ += f == "load" ? "    // Carregando modelo\n" : "    // Salvando modelo\n";
            line(d.name + op + f + "(\"" + c.kids[1]->text + "\");");
            return;
        }
        if (f != "optimize" || c.kids.size() < 5 || c.kids.size() > 6) no(d, "=> " + f);
        auto named = [&](size_t i, const char* key, bool str) {
            const AstNode& a = *c.kids[i];
            if (a.kind != AstKind::Assign || a.text != "=" || a.kids[0]->kind != AstKind::Ident || a.kids[0]->text != key) no(d, "=> optimize");
            const AstNode& v = *a.kids[1];
            if (str ? v.kind != AstKind::String : (v.kind != AstKind::Number && v.kind != AstKind::Ident)) no(d, "=> optimize");
            return v.text;
        };
        const std::string ini = nameArg(c, 1), alvo = nameArg(c, 2);
        const std::string epochs = named(3, "epochs", false), lr = named(4, "lr", false);
        const std::string opt = c.kids.size() == 6 ? named(5, "opt", true) : "gd";
        *buf_ += deepNetworks_.count(d.name) ? "    // Chamando otimizador Backpropagation na Rede Profunda\n"
                                             : "    // Chamando otimizador nativo na GhostMatrix\n";
        line(d.name + op + "optimize(" + ini + ", " + alvo + ", " + epochs + ", " + lr + ", \"" + opt + "\");");
    }

    // ------------------------------------------------------------ comandos
    void blockBody(const AstNode& b) {
        locals_.emplace_back();
        ++depth_;
        for (const auto& s : b.kids) {
            marker(s->line);
            stmt(*s);
        }
        --depth_;
        locals_.pop_back();
    }
    // no nível principal o Parser marca também a linha do '}' que fecha
    void closeMarker(const AstNode& b) {
        if (!inFunction_) marker(b.endLine);
    }

    // valor inicial de uma variável/campo (Parser::parseTypedVar)
    std::string initValue(const AstNode& s, const std::string& t) {
        const AstNode& init = *s.kids[1];
        const bool isSet = t.rfind("GhostSet<", 0) == 0;
        const bool isList = t.rfind("GhostList<", 0) == 0 || isSet;
        const bool isMap = t.rfind("GhostMap<", 0) == 0;
        // primeiro token '{' numa declaração map/set: mapa tipado, compreensão tipada ou set vazio
        const AstNode* first = &init;
        while (first->parens == 0 && (first->kind == AstKind::Call || first->kind == AstKind::Member || first->kind == AstKind::Index ||
                                      first->kind == AstKind::Binary || first->kind == AstKind::Ternary || first->kind == AstKind::Assign ||
                                      first->kind == AstKind::Postfix || first->kind == AstKind::Slice))
            first = first->kids[0].get();
        const bool braceFirst = first->parens == 0 &&
                                (first->kind == AstKind::MapLit || (first->kind == AstKind::Comprehension && first->attr != "list") ||
                                 (first->kind == AstKind::ListLit && first->attr == "set"));
        std::string val;
        if ((isMap || isSet) && braceFirst) {
            if (first->kind == AstKind::ListLit) no(s, "conjunto literal {a, b}");
            mapType_ = t;
            val = expr(init);
            mapType_.clear();
            return val;
        }
        if (isList) listType_ = t;
        ghostAlgebra_ = t == "GhostNumberG";
        val = expr(init);
        ghostAlgebra_ = false;
        listType_.clear();
        mapType_.clear();
        return val;
    }

    void varDecl(const AstNode& s) {
        const std::string t = type(*s.kids[0]);
        if (t == "void") no(s, "variavel void");
        hypDeclared(s.name);
        std::string val;
        const bool hasInit = s.kids.size() > 1;
        if (hasInit) val = initValue(s, t);
        else if (t == "auto") no(s, "var sem valor");
        if (isMain() && t != "auto") { // nível principal: global
            globalVars_.insert(s.name);
            globals_ += t + " " + s.name + ";\n";
            if (hasInit) line(s.name + " = " + val + ";");
            return;
        }
        declare(s.name);
        line(hasInit ? t + " " + s.name + " = " + val + ";" : t + " " + s.name + "{};");
    }

    void ifStmt(const AstNode& s, bool chained) {
        const std::string c = expr(*s.kids[0]);
        if (chained) *buf_ += " if (" + c + ") {\n";
        else line("if (" + c + ") {");
        blockBody(*s.kids[1]);
        if (s.kids.size() > 2) {
            closeMarker(*s.kids[1]);
            const AstNode& e = *s.kids[2];
            if (e.kind == AstKind::If) {
                *buf_ += indent() + "} else";
                ifStmt(e, true);
                return;
            }
            line("} else {");
            blockBody(e);
            closeMarker(e);
            line("}");
            return;
        }
        closeMarker(*s.kids[1]);
        line("}");
    }

    std::string forInit(const AstNode& n) {
        if (n.kind == AstKind::Empty) return "";
        if (n.kind == AstKind::VarDecl) {
            const std::string ct = type(*n.kids[0]);
            hypDeclared(n.name);
            declare(n.name);
            std::string s = ct + " " + n.name;
            if (n.kids.size() > 1) s += " = " + expr(*n.kids[1]);
            return s;
        }
        return expr(*n.kids[0], false);
    }

    // O comando começa com um nome (Identifier no Parser: vai para parseAction)?
    static const AstNode* leadingName(const AstNode& e, const AstNode** parent) {
        const AstNode* n = &e;
        *parent = nullptr;
        for (;;) {
            if (n->parens > 0) return nullptr;
            switch (n->kind) {
                case AstKind::Ident: return n;
                case AstKind::Call: case AstKind::Member: case AstKind::Index: case AstKind::Slice: case AstKind::Assign:
                case AstKind::Postfix: case AstKind::Binary: case AstKind::Ternary: case AstKind::TryOp:
                    *parent = n;
                    n = n->kids[0].get();
                    continue;
                default: return nullptr;
            }
        }
    }

    void exprStmt(const AstNode& s) {
        const AstNode& e = *s.kids[0];
        // x = transpose(a);  x = conv(imagem, kernel[, passo]);  (Parser::parseTransposeAssignment / parseConvAssignment)
        if (e.kind == AstKind::Assign && e.text == "=" && e.parens == 0 && e.kids[1]->kind == AstKind::Call && e.kids[1]->parens == 0 &&
            e.kids[1]->kids[0]->kind == AstKind::Ident && (e.kids[1]->kids[0]->text == "transpose" || e.kids[1]->kids[0]->text == "conv")) {
            const AstNode& c = *e.kids[1];
            if (e.kids[0]->kind != AstKind::Ident) no(s, "x = transpose/conv com destino que nao e um nome");
            const std::string& alvo = e.kids[0]->text;
            auto nome = [&](size_t i) -> const std::string& {
                if (i >= c.kids.size() || c.kids[i]->kind != AstKind::Ident || c.kids[i]->parens) no(s, "argumentos de " + c.kids[0]->text);
                return c.kids[i]->text;
            };
            if (c.kids[0]->text == "transpose") {
                if (c.kids.size() != 2) no(s, "transpose(a)");
                const std::string& a = nome(1);
                line(alvo + " = " + a + ".transpose();");
                line("std::cout << \"[Ghost OS] Transposicao de '\" << \"" + a + "\" << \"' calculada com sucesso.\" << std::endl;");
            } else {
                if (c.kids.size() != 3 && c.kids.size() != 4) no(s, "conv(imagem, kernel[, passo])");
                const std::string& img = nome(1);
                const std::string& ker = nome(2);
                std::string passo = "1";
                if (c.kids.size() == 4) {
                    if (c.kids[3]->kind != AstKind::Number) no(s, "passo de conv precisa ser um numero");
                    passo = c.kids[3]->text;
                }
                line(alvo + " = " + img + ".conv2d(" + ker + ", " + passo + ");");
                line("std::cout << \"[Ghost OS] Convolucao ('" + ker + "') aplicada sobre '" + img + "'.\" << std::endl;");
            }
            *buf_ += "\n";
            return;
        }
        const AstNode* parent = nullptr;
        const AstNode* lead = leadingName(e, &parent);
        if (!lead) {
            line(expr(e) + ";");
            return;
        }
        const std::string& objName = lead->text;
        // hypothesis: o comando altera objName? (=, +=, ++, [..], .metodo)
        if (parent) {
            const bool viaMember = parent->kind == AstKind::Member && !classes_.count(objName);
            if (parent->kind == AstKind::Assign || parent->kind == AstKind::Postfix || parent->kind == AstKind::Index ||
                parent->kind == AstKind::Slice || viaMember)
                hypAssigned(objName);
        }
        // chamada solta: circuit e portas dentro de circuit; q.circuito(...) em qualquer lugar
        if (e.kind == AstKind::Call && e.parens == 0) {
            const AstNode& callee = *e.kids[0];
            std::vector<std::string> args;
            if (callee.kind == AstKind::Ident && callee.parens == 0) {
                if (callee.text == "inputln" || callee.text == "input") {
                    if (e.kids.size() == 2) {
                        const AstNode& arg = *e.kids[1];
                        if (arg.kind == AstKind::FString) {
                            const std::string& rawText = arg.text;
                            size_t braceOpen = rawText.rfind('{');
                            size_t braceClose = rawText.rfind('}');
                            if (braceOpen != std::string::npos && braceClose != std::string::npos && braceClose > braceOpen) {
                                size_t varStart = braceOpen + 1;
                                std::string varName = rawText.substr(varStart, braceClose - varStart);
                                while (!varName.empty() && std::isspace((unsigned char)varName.front())) varName.erase(varName.begin());
                                while (!varName.empty() && std::isspace((unsigned char)varName.back())) varName.pop_back();

                                size_t promptEnd = braceOpen;
                                if (braceOpen > 0 && rawText[braceOpen - 1] == '$') promptEnd = braceOpen - 1;
                                std::string promptPart = rawText.substr(0, promptEnd);
                                if (!promptPart.empty()) {
                                    line("ghost_print(\"" + promptPart + "\");");
                                }
                                line("ghost_input(" + varName + ");");
                                return;
                            }
                        } else if (arg.kind == AstKind::Ident && (isLocal(arg.text) || globalVars_.count(arg.text))) {
                            line("ghost_input(" + arg.text + ");");
                            return;
                        }
                    } else if (e.kids.size() == 3) {
                        const AstNode& p = *e.kids[1];
                        if (p.kind == AstKind::String) {
                            line("ghost_print(\"" + p.text + "\");");
                        } else {
                            line("ghost_print(" + expr(p) + ");");
                        }
                        line("ghost_input(" + expr(*e.kids[2]) + ");");
                        return;
                    }
                }
                for (size_t i = 1; i < e.kids.size(); ++i) args.push_back(expr(*e.kids[i]));
                const std::string rest = joinList(args, ", ");
                if (!circuitTarget_.empty()) {
                    const std::string fn = ident(callee, true, false, true);
                    if (!circuitOf(objName).empty()) {
                        line(objName + "(" + circuitTarget_ + (rest.empty() ? "" : ", ") + rest + ");");
                        return;
                    }
                    if (ghost_is_quantum_op(objName)) {
                        line(circuitTarget_ + "." + fn + "(" + rest + ");");
                        return;
                    }
                    if (userFunctions_.count(objName)) {
                        line(fn + "(" + rest + ");");
                        return;
                    }
                    no(s, "'" + objName + "' nao e porta nem circuit");
                }
                line(ident(callee, true, false, true) + "(" + rest + ");");
                return;
            }
            if (callee.kind == AstKind::Member && callee.parens == 0 && callee.kids.size() == 1 &&
                callee.kids[0]->kind == AstKind::Ident && callee.kids[0]->parens == 0 && !circuitOf(callee.text).empty() &&
                !classes_.count(objName)) {
                for (size_t i = 1; i < e.kids.size(); ++i) args.push_back(expr(*e.kids[i]));
                const std::string rest = joinList(args, ", ");
                line(callee.text + "(" + objName + (rest.empty() ? "" : ", ") + rest + ");");
                return;
            }
        }
        line(expr(e, true) + ";");
    }

    void switchStmt(const AstNode& s) {
        const std::string subject = expr(*s.kids[0]);
        const std::string id = std::to_string(++uniqueCounter_);
        const std::string label = "ghost_sw_end_" + id;
        struct Section { bool isDefault = false; std::vector<std::string> values; std::string code; };
        std::vector<Section> sections;
        std::string* saved = buf_;
        locals_.emplace_back();
        blocks_.push_back({'s', label});
        for (size_t i = 1; i < s.kids.size(); ++i) {
            const AstNode& c = *s.kids[i];
            Section sec;
            sec.isDefault = c.attr == "default";
            for (size_t k = 0; k + 1 < c.kids.size(); ++k) sec.values.push_back(expr(*c.kids[k]));
            buf_ = &sec.code;
            ++depth_;
            for (const auto& st : c.kids.back()->kids) stmt(*st);
            --depth_;
            buf_ = saved;
            sections.push_back(std::move(sec));
        }
        blocks_.pop_back();
        locals_.pop_back();
        const std::string sw = "ghost_sw_" + id, fall = "ghost_fall_" + id;
        std::vector<std::string> all;
        for (const auto& sec : sections)
            for (const auto& v : sec.values) all.push_back(sw + " == (" + v + ")");
        line("{");
        line("const auto " + sw + " = (" + subject + ");");
        line("bool " + fall + " = false;");
        for (const auto& sec : sections) {
            std::vector<std::string> conds;
            for (const auto& v : sec.values) conds.push_back(sw + " == (" + v + ")");
            const std::string cond = sec.isDefault ? "!(" + (all.empty() ? std::string("false") : joinList(all, " || ")) + ")" : joinList(conds, " || ");
            line("if (" + fall + " || " + cond + ") {");
            line(fall + " = true;");
            *buf_ += sec.code;
            line("}");
        }
        line("(void)" + fall + ";");
        line("}");
        line(label + ":;");
    }

    void tryStmt(const AstNode& s) {
        const std::string id = std::to_string(++uniqueCounter_);
        std::string* saved = buf_;
        std::string tryCode, catchCode, finCode, catchVar;
        bool hasCatch = false, hasFinally = false;
        auto body = [&](const AstNode& b, std::string& out) {
            buf_ = &out;
            locals_.emplace_back();
            ++depth_;
            for (const auto& st : b.kids) stmt(*st);
            --depth_;
            locals_.pop_back();
            buf_ = saved;
        };
        body(*s.kids[0], tryCode);
        for (size_t i = 1; i < s.kids.size(); ++i) {
            const AstNode& c = *s.kids[i];
            if (c.kind == AstKind::Catch) {
                hasCatch = true;
                catchVar = c.name;
                locals_.emplace_back();
                if (!catchVar.empty()) {
                    hypDeclared(catchVar);
                    declare(catchVar);
                }
                body(*c.kids[0], catchCode);
                locals_.pop_back();
            } else {
                hasFinally = true;
                body(*c.kids[0], finCode);
            }
        }
        if (!hasCatch && !hasFinally) no(s, "try sem catch nem finally");
        line("{");
        if (hasFinally) {
            line("GhostDefer ghost_fin_" + id + "([&]() {");
            *buf_ += finCode;
            line("});");
        }
        if (hasCatch) {
            line("try {");
            *buf_ += tryCode;
            line("} catch (...) {");
            if (!catchVar.empty()) {
                line("std::string " + catchVar + " = ghost_exception_message(std::current_exception());");
                line("(void)" + catchVar + ";");
            }
            *buf_ += catchCode;
            line("}");
        } else {
            line("{");
            *buf_ += tryCode;
            line("}");
        }
        line("}");
    }

    void hypothesis(const AstNode& s) {
        hypDeclared(s.name);
        const std::string coll = expr(*s.kids[1]);
        const int idn = ++uniqueCounter_;
        hyp_.push_back({idn, {}, {}});
        std::string body;
        std::string* saved = buf_;
        buf_ = &body;
        locals_.emplace_back();
        ++depth_;
        for (const auto& st : s.kids[2]->kids) stmt(*st);
        --depth_;
        locals_.pop_back();
        buf_ = saved;
        HypFrame frame = hyp_.back();
        hyp_.pop_back();
        const std::string& var = s.name;
        std::vector<std::string> vars;
        for (const auto& n : frame.assigned) {
            if (frame.declared.count(n) || n == var || userFunctions_.count(n) || classes_.count(n) || circuits_.count(n)) continue;
            vars.push_back(n);
        }
        const std::string id = std::to_string(idn);
        const std::string items = "ghost_hyp_items_" + id, has = "ghost_hyp_has_" + id, best = "ghost_hyp_best_" + id;
        const std::string scored = "ghost_hyp_scored_" + id, score = "ghost_hyp_score_" + id, kind = "ghost_hyp_kind_" + id;
        std::string c = "    { // hypothesis\n    auto " + items + " = " + coll + ";\n    bool " + has + " = false;\n    double " + best + " = 0.0;\n";
        for (const auto& v : vars) c += "    auto ghost_hyp_best_" + v + "_" + id + " = " + v + ";\n";
        c += "    for (auto& " + var + " : " + items + ") {\n";
        for (const auto& v : vars) c += "    auto ghost_hyp_o_" + v + "_" + id + " = " + v + ";\n";
        c += "    {\n";
        for (const auto& v : vars) c += "    auto " + v + " = ghost_hyp_o_" + v + "_" + id + ";\n";
        c += "    bool " + scored + " = false;\n    double " + score + " = 0.0;\n    int " + kind + " = 1;\n";
        c += body;
        c += "    if (" + scored + " && (!" + has + " || (" + kind + " > 0 ? " + score + " > " + best + " : " + score + " < " + best + "))) {\n";
        c += "    " + has + " = true;\n    " + best + " = " + score + ";\n";
        for (const auto& v : vars) c += "    ghost_hyp_best_" + v + "_" + id + " = " + v + ";\n";
        c += "    }\n    (void)" + var + ";\n    }\n    }\n";
        c += "    if (" + has + ") {\n";
        for (const auto& v : vars) c += "    " + v + " = ghost_hyp_best_" + v + "_" + id + ";\n";
        c += "    std::cout << \"[Multiverso] Colapso temporal resolvido. Linha do tempo de maior score (\" << " + best +
             " << \") assumida.\" << std::endl;\n";
        c += "    }\n    }\n";
        *buf_ += c;
    }

    void stmt(const AstNode& s) {
        switch (s.kind) {
            case AstKind::Empty: return;
            case AstKind::VarDecl: varDecl(s); return;
            case AstKind::ExprStmt: exprStmt(s); return;
            case AstKind::Print: {
                std::string code = "std::cout";
                for (const auto& a : s.kids) code += " << ghost_show(" + expr(*a) + ")";
                line(code + " << std::endl;");
                return;
            }
            case AstKind::If: ifStmt(s, false); return;
            case AstKind::While: {
                line("while (" + expr(*s.kids[0]) + ") {");
                blocks_.push_back({'l', ""});
                blockBody(*s.kids[1]);
                blocks_.pop_back();
                closeMarker(*s.kids[1]);
                line("}");
                return;
            }
            case AstKind::DoWhile: {
                line("do {");
                blocks_.push_back({'l', "do"});
                blockBody(*s.kids[0]);
                blocks_.pop_back();
                closeMarker(*s.kids[0]);
                line("} while (" + expr(*s.kids[1]) + ");");
                return;
            }
            case AstKind::For: {
                locals_.emplace_back();
                const std::string init = forInit(*s.kids[0]);
                const std::string cond = s.kids[1]->kind == AstKind::Empty ? "" : expr(*s.kids[1]);
                const std::string step = s.kids[2]->kind == AstKind::Empty ? "" : expr(*s.kids[2]);
                line("for (" + init + "; " + cond + "; " + step + ") {");
                blocks_.push_back({'l', ""});
                blockBody(*s.kids[3]);
                blocks_.pop_back();
                closeMarker(*s.kids[3]);
                line("}");
                locals_.pop_back();
                return;
            }
            case AstKind::Foreach: {
                const AstNode& t = *s.kids[0];
                std::string declType = "auto&";
                if (t.text != "var") {
                    const std::string ct = type(t);
                    if (ct != "auto") declType = ct;
                }
                const std::string coll = expr(*s.kids[1]);
                locals_.emplace_back();
                hypDeclared(s.name);
                declare(s.name);
                if (!s.attr.empty()) {
                    hypDeclared(s.attr);
                    declare(s.attr);
                }
                if (s.attr.empty()) line("for (" + declType + " " + s.name + " : " + coll + ") {");
                else line("for (auto& [" + s.name + ", " + s.attr + "] : " + coll + ") {");
                blocks_.push_back({'l', ""});
                blockBody(*s.kids[2]);
                blocks_.pop_back();
                closeMarker(*s.kids[2]);
                line("}");
                locals_.pop_back();
                return;
            }
            case AstKind::Break: case AstKind::Continue: {
                const bool isBreak = s.kind == AstKind::Break;
                for (auto it = blocks_.rbegin(); it != blocks_.rend(); ++it) {
                    if (it->first == 'l') { line(isBreak ? "break;" : "continue;"); return; }
                    if (it->first == 's' && isBreak) { line("goto " + it->second + ";"); return; }
                }
                no(s, "break/continue fora de laco");
            }
            case AstKind::Return:
                line(s.kids.empty() ? "return;" : "return " + expr(*s.kids[0]) + ";");
                return;
            case AstKind::Throw:
                line(s.kids.empty() ? "throw;" : "throw ghost_error(" + expr(*s.kids[0]) + ");");
                return;
            case AstKind::Switch: switchStmt(s); return;
            case AstKind::Try: tryStmt(s); return;
            case AstKind::Spawn:
                line("std::thread([&]() {");
                blocks_.push_back({'o', ""});
                blockBody(*s.kids[0]);
                blocks_.pop_back();
                closeMarker(*s.kids[0]);
                line("}).detach();");
                return;
            case AstKind::Hypothesis: hypothesis(s); return;
            case AstKind::Collapse: {
                if (hyp_.empty()) no(s, "collapse fora de hypothesis");
                const std::string e = expr(*s.kids[0]);
                const std::string id = std::to_string(hyp_.back().id);
                line("ghost_hyp_scored_" + id + " = true;");
                line("ghost_hyp_score_" + id + " = static_cast<double>(" + e + ");");
                line("ghost_hyp_kind_" + id + " = " + (s.text == "max" ? "1" : "-1") + ";");
                return;
            }
            case AstKind::Domain: domain(s); return;
            case AstKind::FuncDecl: no(s, "function dentro de bloco");
            default: no(s, std::string("comando ") + ghost_ast_kind_name(s.kind));
        }
    }
};

// Texto sem espaços fora de strings, e a lista de #line (sem os de volta ao .cpp)
void normalize(const std::string& code, std::string& tokens, std::vector<std::string>& markers) {
    std::istringstream in(code);
    std::string text;
    for (std::string ln; std::getline(in, ln);) {
        if (ln.rfind("#line ", 0) == 0) {
            const size_t q = ln.find('"');
            const std::string file = q == std::string::npos ? "" : ln.substr(q);
            if (file.size() < 6 || file.compare(file.size() - 5, 5, ".cpp\"") != 0) markers.push_back(ln);
            continue;
        }
        text += ln;
        text += '\n';
    }
    char quote = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (quote) {
            tokens += c;
            if (c == '\\' && i + 1 < text.size()) tokens += text[++i];
            else if (c == quote) quote = 0;
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            tokens += c;
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) tokens += c;
    }
}

} // namespace

GhostCodegenResult ghost_codegen(const AstNode& program, bool wasm, const std::string& cppFile) {
    GhostCodegenResult r;
    try {
        Gen g(wasm, cppFile);
        r.cpp = g.run(program);
        r.ok = true;
    } catch (const Unsupported& u) {
        const auto loc = GhostDiag::get().locate(u.line);
        r.reason = u.what + " (linha " + std::to_string(loc.second) + (loc.first.empty() ? "" : " de " + loc.first) + ")";
    }
    return r;
}

std::string ghost_cpp_normalized(const std::string& code) {
    std::string tokens;
    std::vector<std::string> markers;
    normalize(code, tokens, markers);
    std::string out;
    for (const auto& m : markers) out += m + "\n";
    out += "----\n";
    // Quebra depois de ; { } fora de textos (os tokens não têm quebras de linha fora de textos)
    char quote = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const char c = tokens[i];
        out += c;
        if (quote) {
            if (c == '\\' && i + 1 < tokens.size()) out += tokens[++i];
            else if (c == quote) quote = 0;
            continue;
        }
        if (c == '"' || c == '\'') quote = c;
        else if (c == ';' || c == '{' || c == '}') out += '\n';
    }
    if (out.empty() || out.back() != '\n') out += '\n';
    return out;
}

bool ghost_cpp_equivalent(const std::string& a, const std::string& b, std::string* diff) {
    std::string ta, tb;
    std::vector<std::string> ma, mb;
    normalize(a, ta, ma);
    normalize(b, tb, mb);
    if (ma != mb) {
        if (diff) {
            size_t k = 0;
            while (k < ma.size() && k < mb.size() && ma[k] == mb[k]) ++k;
            *diff = "#line diferente (posicao " + std::to_string(k + 1) + "): " + (k < ma.size() ? ma[k] : "(fim)") + "  x  " +
                    (k < mb.size() ? mb[k] : "(fim)");
        }
        return false;
    }
    if (ta != tb) {
        if (diff) {
            size_t k = 0;
            while (k < ta.size() && k < tb.size() && ta[k] == tb[k]) ++k;
            const size_t from = k > 60 ? k - 60 : 0;
            *diff = "tokens diferentes:\n  Parser: ..." + ta.substr(from, 140) + "\n  AST:    ..." + tb.substr(from, 140);
        }
        return false;
    }
    return true;
}
