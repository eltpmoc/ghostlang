// InterpreterQuantum.cpp - qreg e qcircuit no REPL da GhostLang.
//
//   ghost> qreg q(2);
//   ghost> q.h(0); q.cx(0, 1);
//   ghost> print(q);                      // 0.7071|00> + 0.7071|11>
//   ghost> print(q.histogram(1000));
//   ghost> int r = q.measure(0);
//   ghost> qcircuit c(2); c.ansatz(2); c.observable("ZZ", 1.0);
//   ghost> print(f"E = ${c.minimize(100, 0.1)}");
//
// Os métodos têm os mesmos nomes do compilador (GhostQReg / GhostQCircuit). Argumentos
// numéricos passam por evaluateMath; textos aceitam "literal", f"..." ou variáveis string.
#include "../include/Interpreter.hpp"
#include "../include/GhostQuantumOps.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string precise(double v) {
    std::ostringstream os;
    os << std::setprecision(17) << v;
    return os.str();
}
} // namespace

bool GhostInterpreter::isQuantum(const std::string& name) const {
    // Dentro de um circuit, parâmetros e variáveis locais sombreiam registradores de mesmo nome:
    // circuit bell(a, b) { cx(a, b); } funciona mesmo com um qreg chamado b.
    if (circuit_depth > 0 && numeric_memory.count(name)) return false;
    // Dentro de uma função, parâmetros e variáveis locais sombreiam registradores globais:
    if (!frames.empty() && coll_memory.count(name)) return false;
    return qreg_memory.count(name) > 0 || qcircuit_memory.count(name) > 0 || qtape_memory.count(name) > 0;
}

std::string GhostInterpreter::circuitBase(const std::string& n, bool* inverse, bool* controlled) const {
    auto ends = [](const std::string& s, const std::string& suf) { return s.size() > suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0; };
    bool inv = false, ctl = false;
    std::string b = n;
    if (!circuit_memory.count(b) && ends(b, "_ctrl")) { b = b.substr(0, b.size() - 5); ctl = true; }
    if (!circuit_memory.count(b) && ends(b, "_dg")) { b = b.substr(0, b.size() - 3); inv = true; }
    if (!circuit_memory.count(b)) return "";
    if (inverse) *inverse = inv;
    if (controlled) *controlled = ctl;
    return b;
}

void GhostInterpreter::parseQuantumDecl(Lexer& lex, GTokenType type) {
    if (type == GTokenType::KwQlayer) {
        Token name = lex.getNextToken();
        if (name.type != GTokenType::Identifier) throw std::runtime_error("Esperado o nome da qlayer.");
        if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error("Use qlayer " + name.value + "(qubits, camadas);");
        std::vector<Token> qTokens, lTokens;
        bool inSecond = false;
        int depth = 1;
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Faltando ')' em qlayer " + name.value);
            if (t.type == GTokenType::LParen) depth++;
            else if (t.type == GTokenType::RParen) {
                if (--depth == 0) break;
            } else if (t.type == GTokenType::Comma && depth == 1) {
                inSecond = true;
                continue;
            }
            if (inSecond) lTokens.push_back(t);
            else qTokens.push_back(t);
        }
        Token semi = lex.getNextToken();
        if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos declaracao de qlayer " + name.value);
        int qubits = (int)evaluateMath(qTokens);
        int layers = inSecond ? (int)evaluateMath(lTokens) : 1;
        qlayer_memory[name.value] = std::make_shared<GhostQLayer>(qubits, layers);
        return;
    }
    const std::string kw = type == GTokenType::KwQcircuit ? "qcircuit" : "qreg";
    Token name = lex.getNextToken();
    if (name.type != GTokenType::Identifier) throw std::runtime_error("Esperado o nome do " + kw + ".");
    if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error("Use " + kw + " " + name.value + "(numero_de_qubits);");
    std::vector<Token> expr;
    int depth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Faltando ')' em " + kw + ".");
        if (t.type == GTokenType::LParen) depth++;
        if (t.type == GTokenType::RParen && --depth == 0) break;
        expr.push_back(t);
    }
    if (expr.empty()) throw std::runtime_error(kw + " " + name.value + "() precisa do numero de qubits.");
    const int n = static_cast<int>(evaluateMath(expr));
    if (!frames.empty()) declareLocal(name.value);
    qreg_memory.erase(name.value);
    qcircuit_memory.erase(name.value);
    if (type == GTokenType::KwQcircuit) qcircuit_memory.emplace(name.value, GhostQCircuit(n));
    else qreg_memory.emplace(name.value, GhostQReg(n));
    if (lex.peekNextToken().type == GTokenType::Semicolon) lex.getNextToken();
}

std::vector<std::vector<Token>> GhostInterpreter::splitArgs(std::vector<Token>& tokens, size_t& i) {
    if (i >= tokens.size() || tokens[i].type != GTokenType::LParen) throw std::runtime_error("Esperado '(' na chamada do metodo.");
    std::vector<std::vector<Token>> args;
    std::vector<Token> cur;
    int depth = 0, nest = 0; // nest: [ ] e { } dentro dos argumentos (f([1, 2], 3))
    int angle = 0;           // < > de tipos genéricos: f(Par<string, int>("a", 1), maior<int, double>(...))
    for (; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        if (t.type == GTokenType::Lt && (angle > 0 || (!cur.empty() && opensTypeAngle(cur.back())))) {
            angle++;
            cur.push_back(t);
            continue;
        }
        if (angle > 0 && (t.type == GTokenType::Gt || t.type == GTokenType::Chain)) {
            angle = std::max(0, angle - (t.type == GTokenType::Chain ? 2 : 1));
            cur.push_back(t);
            continue;
        }
        if (t.type == GTokenType::LParen) { if (depth++ == 0) continue; }
        if (t.type == GTokenType::RParen) {
            if (--depth == 0) {
                if (!cur.empty() || !args.empty()) args.push_back(cur);
                return args;
            }
        }
        if (t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) nest++;
        if (t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) nest--;
        if (depth == 1 && nest == 0 && angle == 0 && t.type == GTokenType::Comma) { args.push_back(cur); cur.clear(); continue; }
        cur.push_back(t);
    }
    throw std::runtime_error("Faltando ')' na chamada do metodo.");
}

GhostInterpreter::QValue GhostInterpreter::evalQuantumAt(std::vector<Token>& tokens, size_t& i) {
    QValue out;
    const std::string obj = tokens[i].value;

    if (obj == "quantum_backend" || obj == "quantum_device") {
        ++i;
        auto args = splitArgs(tokens, i);
        if (obj == "quantum_backend") { out.kind = QValue::Text; out.str = GhostQReg::backend_info(); return out; }
        if (args.size() != 1) throw std::runtime_error("quantum_device(\"cpu\" | \"cuda\" | \"auto\")");
        GhostQReg::set_default_device(evaluateStringExpression(args[0]));
        return out;
    }

    // Objeto sozinho: texto (ket do qreg ou resumo do qcircuit)
    if (i + 1 >= tokens.size() || tokens[i + 1].type != GTokenType::Dot) {
        out.kind = QValue::Text;
        out.str = qreg_memory.count(obj) ? qreg_memory.at(obj).ket() : qcircuit_memory.at(obj).to_string();
        return out;
    }
    if (i + 2 >= tokens.size()) throw std::runtime_error("Esperado o nome do metodo apos '" + obj + ".'.");
    const std::string method = tokens[i + 2].value;
    i += 3;
    auto args = splitArgs(tokens, i);
    return callQuantum(obj, method, args);
}

// circuit nome(a, b, double theta) { h(a); cx(a, b); ry(b, theta); }
// Parâmetros sem tipo são numéricos (double); "int" arredonda o valor recebido.
void GhostInterpreter::parseCircuitDef(Lexer& lex) {
    Token name = lex.getNextToken();
    if (name.type != GTokenType::Identifier) throw std::runtime_error("Esperado o nome do circuit. Ex.: circuit bell(a, b) { h(a); cx(a, b); }");
    if (ghost_is_quantum_op(name.value)) throw std::runtime_error("'" + name.value + "' ja e uma porta/metodo quantico; escolha outro nome para o circuit.");
    if (name.value.size() > 5 && name.value.compare(name.value.size() - 5, 5, "_ctrl") == 0) {
        throw std::runtime_error("nomes terminados em _ctrl sao reservados para a versao controlada (declare circuit " +
                                 name.value.substr(0, name.value.size() - 5) + " e use " + name.value + "(controle, ...)).");
    }
    if (name.value.size() > 3 && name.value.compare(name.value.size() - 3, 3, "_dg") == 0) {
        throw std::runtime_error("nomes terminados em _dg sao reservados para o inverso (declare circuit " + name.value.substr(0, name.value.size() - 3) +
                                 " e use " + name.value + " para o inverso).");
    }
    if (!circuit_target.empty()) throw std::runtime_error("circuit " + name.value + " nao pode ser declarado dentro de outro circuit.");
    if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos circuit " + name.value + ".");
    GhostCircuitDef def;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::RParen) break;
        if (t.type == GTokenType::Comma && !def.params.empty()) continue;
        GTokenType type = GTokenType::Identifier;
        if (t.type == GTokenType::KwInt || t.type == GTokenType::KwDouble) { type = t.type; t = lex.getNextToken(); }
        if (t.type != GTokenType::Identifier) throw std::runtime_error("Parametro invalido em circuit " + name.value + " (use nome, int nome ou double nome).");
        for (const auto& p : def.params) {
            if (p.second == t.value) throw std::runtime_error("Parametro '" + t.value + "' repetido em circuit " + name.value + ".");
        }
        def.params.push_back({type, t.value});
    }
    if (lex.getNextToken().type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' apos circuit " + name.value + "(...).");
    def.body = lex.extractBlock();
    circuit_memory[name.value] = def;
}

void GhostInterpreter::runCircuit(const std::string& obj, const std::string& name, std::vector<std::vector<Token>>& args, bool inverse, bool controlled) {
    // Consulta direta (sem sombreamento): o alvo pode ter o mesmo nome de um parâmetro.
    const bool isReg = qreg_memory.count(obj) > 0, isCirc = qcircuit_memory.count(obj) > 0, isTape = qtape_memory.count(obj) > 0;
    if (!isReg && !isCirc && !isTape) throw std::runtime_error("circuit " + name + ": '" + obj + "' nao e um qreg nem um qcircuit.");

    if (controlled) {
        // nome_ctrl(controle, ...): grava o corpo (ou o inverso) e aplica só quando o controle está em 1.
        const std::string called = name + (inverse ? "_dg" : "") + "_ctrl";
        if (isCirc) throw std::runtime_error("circuit controlado (" + called + ") ainda nao e suportado em qcircuit; use em um qreg");
        if (args.empty()) throw std::runtime_error(called + ": o primeiro argumento e o qubit de controle");
        std::vector<Token> ctlExpr = args[0];
        const int control = static_cast<int>(std::llround(evaluateMath(ctlExpr)));
        std::vector<std::vector<Token>> rest(args.begin() + 1, args.end());
        const int n = isReg ? qreg_memory.at(obj).size() : qtape_memory.at(obj).size();
        const std::string rec = "__ctl" + std::to_string(++inverse_counter) + "__";
        qtape_memory.emplace(rec, GhostQTape(n));
        try {
            runCircuit(rec, name, rest, inverse, false);
            if (isReg) qtape_memory.at(rec).apply_controlled(qreg_memory.at(obj), control);
            else qtape_memory.at(rec).apply_controlled(qtape_memory.at(obj), control);
        } catch (...) {
            qtape_memory.erase(rec);
            throw;
        }
        qtape_memory.erase(rec);
        return;
    }

    if (inverse) {
        // nome_dg: grava o corpo num registrador interno e aplica as portas inversas em ordem reversa.
        // qreg (ou outro gravador) -> GhostQTape;  qcircuit -> cópia leve do próprio circuito.
        const std::string rec = "__inv" + std::to_string(++inverse_counter) + "__";
        if (isCirc) qcircuit_memory.emplace(rec, qcircuit_memory.at(obj).inverse_recorder());
        else qtape_memory.emplace(rec, GhostQTape(isReg ? qreg_memory.at(obj).size() : qtape_memory.at(obj).size()));
        try {
            runCircuit(rec, name, args, false);
            if (isCirc) {
                GhostQCircuit& c = qcircuit_memory.at(obj);
                c.append_inverse(qcircuit_memory.at(rec), c.num_gates(), c.num_params());
            } else if (isReg) {
                qtape_memory.at(rec).apply_inverse(qreg_memory.at(obj));
            } else {
                qtape_memory.at(rec).apply_inverse(qtape_memory.at(obj));
            }
        } catch (...) {
            qcircuit_memory.erase(rec);
            qtape_memory.erase(rec);
            throw;
        }
        qcircuit_memory.erase(rec);
        qtape_memory.erase(rec);
        return;
    }

    const GhostCircuitDef def = circuit_memory.at(name); // cópia: o corpo pode redefinir circuits
    if (args.size() != def.params.size()) {
        throw std::runtime_error("circuit " + name + ": esperado " + std::to_string(def.params.size()) + " argumento(s), recebido " + std::to_string(args.size()));
    }
    if (circuit_depth >= 64) throw std::runtime_error("circuit " + name + ": recursao profunda demais (limite 64).");

    // Argumentos avaliados no escopo de quem chamou, antes de ligar os parâmetros.
    std::vector<double> vals;
    for (auto& a : args) {
        std::vector<Token> copy = a;
        vals.push_back(evaluateMath(copy));
    }

    struct Scope {
        GhostInterpreter& in;
        std::map<std::string, double> num;
        std::map<std::string, std::string> str;
        std::map<std::string, GTokenType> type;
        std::string target;
        explicit Scope(GhostInterpreter& i)
            : in(i), num(i.numeric_memory), str(i.string_memory), type(i.type_memory), target(i.circuit_target) { ++in.circuit_depth; }
        ~Scope() {
            in.numeric_memory = std::move(num);
            in.string_memory = std::move(str);
            in.type_memory = std::move(type);
            in.circuit_target = std::move(target);
            --in.circuit_depth;
        }
    } scope(*this);

    for (size_t k = 0; k < def.params.size(); ++k) {
        const bool isInt = def.params[k].first == GTokenType::KwInt;
        const std::string& p = def.params[k].second;
        numeric_memory[p] = isInt ? static_cast<double>(std::llround(vals[k])) : vals[k];
        type_memory[p] = isInt ? GTokenType::KwInt : GTokenType::KwDouble;
        string_memory.erase(p);
    }
    circuit_target = obj;
    executeLine(def.body);
}

// ---------------------------------------------------------------- qreg/qcircuit dentro de valores
GhostValue GhostInterpreter::makeQuantum(GTokenType type, double qubits) {
    const int n = static_cast<int>(qubits);
    if (n < 1 || n > 30) throw std::runtime_error(std::string(type == GTokenType::KwQreg ? "qreg" : "qcircuit") + "(n): use de 1 a 30 qubits");
    GhostValue v;
    v.kind = GhostValue::Quantum;
    if (type == GTokenType::KwQreg) v.qreg = std::make_shared<GhostQReg>(n);
    else v.qcirc = std::make_shared<GhostQCircuit>(n);
    return v;
}

std::string GhostInterpreter::bindQuantum(const GhostValue& v) {
    const std::string tmp = "__qv" + std::to_string(++qvalue_counter) + "__";
    if (v.qreg) qreg_memory.emplace(tmp, *v.qreg);
    else if (v.qcirc) qcircuit_memory.emplace(tmp, *v.qcirc);
    else throw std::runtime_error("registrador vazio");
    return tmp;
}

void GhostInterpreter::unbindQuantum(const std::string& tmp, GhostValue* back) {
    auto r = qreg_memory.find(tmp);
    if (r != qreg_memory.end()) {
        if (back) { back->kind = GhostValue::Quantum; back->qreg = std::make_shared<GhostQReg>(std::move(r->second)); back->qcirc.reset(); }
        qreg_memory.erase(r);
    }
    auto c = qcircuit_memory.find(tmp);
    if (c != qcircuit_memory.end()) {
        if (back) { back->kind = GhostValue::Quantum; back->qcirc = std::make_shared<GhostQCircuit>(std::move(c->second)); back->qreg.reset(); }
        qcircuit_memory.erase(c);
    }
}

GhostValue GhostInterpreter::quantumCall(const LPath* path, const GhostValue& v, const std::string& m, std::vector<Token>& t, size_t& i, bool eval) {
    // t[i] == '(' : acha o ')' da chamada
    size_t close = i;
    int depth = 0;
    for (; close < t.size(); ++close) {
        if (t[close].type == GTokenType::LParen) depth++;
        else if (t[close].type == GTokenType::RParen && --depth == 0) break;
    }
    if (close >= t.size()) throw std::runtime_error("faltou ')' em ." + m + "(...)");
    std::vector<Token> call(t.begin() + static_cast<long>(i), t.begin() + static_cast<long>(close) + 1);
    i = close + 1;
    if (!eval) return GhostValue::number(0, true);
    const std::string tmp = bindQuantum(v);
    Token objTok = t[i - 1];
    objTok.type = GTokenType::Identifier;
    objTok.value = tmp;
    Token dot = objTok, meth = objTok;
    dot.type = GTokenType::Dot; dot.value = ".";
    meth.value = m;
    call.insert(call.begin(), {objTok, dot, meth});
    QValue q;
    try {
        size_t pos = 0;
        q = evalQuantumAt(call, pos);
    } catch (...) {
        unbindQuantum(tmp, nullptr);
        throw;
    }
    GhostValue* target = path ? resolvePath(*path, false, true) : nullptr;
    unbindQuantum(tmp, target && target->kind == GhostValue::Quantum ? target : nullptr);
    if (q.kind == QValue::Text) return GhostValue::text(q.str);
    if (q.kind == QValue::Number) return GhostValue::number(q.num);
    return GhostValue::number(0, true);
}

bool GhostInterpreter::tryCircuitStatement(const Token& name, const Token& next, Lexer& lex) {
    if (next.type != GTokenType::LParen) return false;
    const std::string& n = name.value;
    bool inverse = false, controlled = false; // nome_dg / nome_ctrl / nome_dg_ctrl
    const std::string base = circuitBase(n, &inverse, &controlled);
    const bool isCircuit = !base.empty();
    const bool insideGate = !circuit_target.empty() && !functions_memory.count(n) && n != "quantum_device" && n != "quantum_backend";
    if (!isCircuit && !insideGate) return false;

    std::vector<Token> call{next};
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
        call.push_back(t);
    }
    size_t pos = 0;
    auto args = splitArgs(call, pos);
    if (pos + 1 < call.size()) throw std::runtime_error("Esperado ';' apos a chamada de " + n + ".");

    if (!isCircuit) {
        if (!ghost_is_quantum_op(n)) {
            throw std::runtime_error("'" + n + "' nao e uma porta quantica nem um circuit (dentro de um circuit, use h(a); cx(a, b); ...)");
        }
        callQuantum(circuit_target, n, args);
    } else if (!circuit_target.empty()) {
        runCircuit(circuit_target, base, args, inverse, controlled); // outro circuit dentro de um circuit: mesmo registrador
    } else {
        // bell(q, 0, 1) com q campo da classe (ou variável com qreg(n)): roda no registrador do valor
        if (!args.empty() && args[0].size() == 1 && args[0][0].type == GTokenType::Identifier) {
            const std::string root = args[0][0].value;
            GhostValue* qv = rootRef(root);
            if (qv && qv->kind == GhostValue::Quantum && (fieldRef(root) || !isQuantum(root))) {
                const std::string tmp = bindQuantum(*qv);
                args.erase(args.begin());
                try { runCircuit(tmp, base, args, inverse, controlled); }
                catch (...) { unbindQuantum(tmp, nullptr); throw; }
                GhostValue* back = rootRef(root);
                unbindQuantum(tmp, back && back->kind == GhostValue::Quantum ? back : nullptr);
                return true;
            }
        }
        if (args.empty() || args[0].size() != 1 || args[0][0].type != GTokenType::Identifier || !isQuantum(args[0][0].value)) {
            throw std::runtime_error("Use q." + n + "(...) ou " + n + "(q, ...), com q declarado por qreg/qcircuit.");
        }
        const std::string target = args[0][0].value;
        args.erase(args.begin());
        runCircuit(target, base, args, inverse, controlled);
    }
    return true;
}

GhostInterpreter::QValue GhostInterpreter::callQuantum(const std::string& obj, const std::string& m, std::vector<std::vector<Token>>& a) {
    QValue out;
    {
        bool inverse = false, controlled = false; // q.bell(0, 1) / q.bell_dg(0, 1) / q.bell_ctrl(2, 0, 1)
        const std::string base = circuitBase(m, &inverse, &controlled);
        if (!base.empty()) { runCircuit(obj, base, a, inverse, controlled); return out; }
    }
    auto num = [&](size_t k) -> double {
        if (k >= a.size()) throw std::runtime_error(obj + "." + m + ": faltam argumentos");
        std::vector<Token> copy = a[k];
        return evaluateMath(copy);
    };
    auto inum = [&](size_t k) -> int { return static_cast<int>(std::llround(num(k))); };
    auto str = [&](size_t k) -> std::string {
        if (k >= a.size()) throw std::runtime_error(obj + "." + m + ": faltam argumentos");
        std::vector<Token> copy = a[k];
        return evaluateStringExpression(copy);
    };
    auto nums = [&](size_t from) {
        std::vector<double> v;
        for (size_t k = from; k < a.size(); ++k) v.push_back(num(k));
        return v;
    };
    auto expect_args = [&](size_t lo, size_t hi) {
        if (a.size() < lo || a.size() > hi) {
            throw std::runtime_error(obj + "." + m + ": esperado " + (lo == hi ? std::to_string(lo) : std::to_string(lo) + " a " + std::to_string(hi)) +
                                     " argumento(s), recebido " + std::to_string(a.size()));
        }
    };
    auto number = [&](double v) { out.kind = QValue::Number; out.num = v; return out; };
    auto text = [&](const std::string& s) { out.kind = QValue::Text; out.str = s; return out; };

    // Portas de qreg: valem para GhostQReg e para o gravador GhostQTape (circuit nome_dg).
    auto gates = [&](auto& q) -> bool {
        // portas de 1 qubit
        if (m == "h" || m == "x" || m == "y" || m == "z" || m == "s" || m == "sdg" || m == "t" || m == "tdg" || m == "sx") {
            expect_args(1, 1);
            const int k = inum(0);
            if (m == "h") q.h(k); else if (m == "x") q.x(k); else if (m == "y") q.y(k); else if (m == "z") q.z(k);
            else if (m == "s") q.s(k); else if (m == "sdg") q.sdg(k); else if (m == "t") q.t(k); else if (m == "tdg") q.tdg(k); else q.sx(k);
            return true;
        }
        if (m == "rx" || m == "ry" || m == "rz" || m == "p") {
            expect_args(2, 2);
            const int k = inum(0); const double th = num(1);
            if (m == "rx") q.rx(k, th); else if (m == "ry") q.ry(k, th); else if (m == "rz") q.rz(k, th); else q.p(k, th);
            return true;
        }
        if (m == "u") { expect_args(4, 4); q.u(inum(0), num(1), num(2), num(3)); return true; }
        if (m == "h_all") { expect_args(0, 0); q.h_all(); return true; }
        if (m == "x_all") { expect_args(0, 0); q.x_all(); return true; }
        if (m == "cx" || m == "cnot" || m == "cy" || m == "cz" || m == "ch" || m == "swap") {
            expect_args(2, 2);
            const int c = inum(0), t = inum(1);
            if (m == "cx" || m == "cnot") q.cx(c, t); else if (m == "cy") q.cy(c, t); else if (m == "cz") q.cz(c, t);
            else if (m == "ch") q.ch(c, t); else q.swap(c, t);
            return true;
        }
        if (m == "cp" || m == "crx" || m == "cry" || m == "crz" || m == "rzz") {
            expect_args(3, 3);
            const int c = inum(0), t = inum(1); const double th = num(2);
            if (m == "cp") q.cp(c, t, th); else if (m == "crx") q.crx(c, t, th); else if (m == "cry") q.cry(c, t, th);
            else if (m == "crz") q.crz(c, t, th); else q.rzz(c, t, th);
            return true;
        }
        if (m == "ccx" || m == "toffoli") { expect_args(3, 3); q.ccx(inum(0), inum(1), inum(2)); return true; }
        if (m == "cswap") { expect_args(3, 3); q.cswap(inum(0), inum(1), inum(2)); return true; }
        if (m == "mcx") {
            if (a.size() < 2) throw std::runtime_error("qreg.mcx: esperado pelo menos 1 controle e 1 alvo");
            std::vector<int> ctls;
            int tgt = -1;
            if (a.size() == 2 && !a[0].empty() && a[0][0].type == GTokenType::LBracket) {
                GhostValue gv = evalValue(a[0]);
                if (gv.kind != GhostValue::Kind::List) throw std::runtime_error("qreg.mcx: primeiro argumento deve ser lista de controles");
                for (const auto& el : gv.list) {
                    double v = el.kind == GhostValue::Kind::Num ? el.num : (el.kind == GhostValue::Kind::Bool ? (el.num != 0 ? 1 : 0) : std::stod(el.str));
                    ctls.push_back(static_cast<int>(std::llround(v)));
                }
                tgt = inum(1);
            } else {
                for (size_t k = 0; k + 1 < a.size(); ++k) ctls.push_back(inum(k));
                tgt = inum(a.size() - 1);
            }
            q.mcx(ctls, tgt);
            return true;
        }
        if (m == "qft") { expect_args(0, 0); q.qft(); return true; }
        if (m == "iqft") { expect_args(0, 0); q.iqft(); return true; }
        if (m == "qft_range") { expect_args(2, 2); q.qft_range(inum(0), inum(1)); return true; }
        if (m == "iqft_range") { expect_args(2, 2); q.iqft_range(inum(0), inum(1)); return true; }
        if (m == "phase_flip") { expect_args(1, 1); q.phase_flip(static_cast<long long>(num(0))); return true; }
        if (m == "diffuser") { expect_args(0, 0); q.diffuser(); return true; }
        return false;
    };

    if (qtape_memory.count(obj)) {
        GhostQTape& tape = qtape_memory.at(obj);
        if (gates(tape)) return out;
        if (m == "measure" || m == "measure_all" || m == "reset") {
            throw std::runtime_error("circuit inverso: " + m + " nao e reversivel (tire-o do circuit para usar nome_dg)");
        }
        if (m == "size") { expect_args(0, 0); return number(tape.size()); }
        throw std::runtime_error("'" + m + "' nao pode ser usado dentro de um circuit invertido (nome_dg)");
    }

    if (qreg_memory.count(obj)) {
        GhostQReg& q = qreg_memory.at(obj);
        if (gates(q)) return out;
        if (m == "reset") { expect_args(0, 0); q.reset(); return out; }
        if (m == "seed") { expect_args(1, 1); q.seed(static_cast<unsigned long long>(num(0))); return out; }
        if (m == "set_basis") { expect_args(1, 1); q.set_basis(static_cast<long long>(num(0))); return out; }
        if (m == "device") { expect_args(1, 1); q.device(str(0)); return out; }
        if (m == "display" || m == "show") { expect_args(0, 0); q.display(); return out; }
        if (m == "measure") { expect_args(1, 1); return number(q.measure(inum(0))); }
        if (m == "measure_all") { expect_args(0, 0); return number(static_cast<double>(q.measure_all())); }
        if (m == "prob") { expect_args(1, 1); return number(q.prob(inum(0))); }
        if (m == "prob_state") { expect_args(1, 1); return number(q.prob_state(static_cast<long long>(num(0)))); }
        if (m == "expect_z") { expect_args(1, 1); return number(q.expect_z(inum(0))); }
        if (m == "expect") { expect_args(1, 1); return number(q.expect(str(0))); }
        if (m == "norm") { expect_args(0, 0); return number(q.norm()); }
        if (m == "size") { expect_args(0, 0); return number(q.size()); }
        if (m == "histogram") { expect_args(1, 1); return text(q.histogram(inum(0))); }
        if (m == "counts") {
            expect_args(1, 1);
            auto c = q.counts(inum(0));
            out.kind = QValue::Map;
            out.map.clear();
            for (const auto& kv : c) {
                out.map[kv.first] = GhostValue::number(static_cast<double>(kv.second), true);
            }
            return out;
        }
        if (m == "ket" || m == "to_string") { expect_args(0, 1); return text(a.empty() ? q.ket() : q.ket(inum(0))); }
        if (m == "probabilities") { expect_args(0, 1); return text(a.empty() ? q.probabilities() : q.probabilities(inum(0))); }
        if (m == "draw") { expect_args(0, 0); return text(q.draw()); }
        if (m == "bits") { expect_args(1, 1); return text(q.bits(static_cast<long long>(num(0)))); }
        if (m == "device_name") { expect_args(0, 0); return text(q.device_name()); }
        if (m == "to_qasm" || m == "qasm") { expect_args(0, 0); return text(q.to_qasm()); }
        throw std::runtime_error("qreg." + m + " nao existe (veja docs/QUANTUM.md)");
    }

    GhostQCircuit& c = qcircuit_memory.at(obj);
    if (m == "h" || m == "x" || m == "y" || m == "z" || m == "s" || m == "t") {
        expect_args(1, 1);
        const int k = inum(0);
        if (m == "h") c.h(k); else if (m == "x") c.x(k); else if (m == "y") c.y(k); else if (m == "z") c.z(k); else if (m == "s") c.s(k); else c.t(k);
        return out;
    }
    if (m == "cx" || m == "cz" || m == "swap") {
        expect_args(2, 2);
        if (m == "cx") c.cx(inum(0), inum(1)); else if (m == "cz") c.cz(inum(0), inum(1)); else c.swap(inum(0), inum(1));
        return out;
    }
    if (m == "rx" || m == "ry" || m == "rz") {
        expect_args(2, 2);
        if (m == "rx") c.rx(inum(0), num(1)); else if (m == "ry") c.ry(inum(0), num(1)); else c.rz(inum(0), num(1));
        return out;
    }
    if (m == "rx_train") { expect_args(1, 1); return number(c.rx_train(inum(0))); }
    if (m == "ry_train") { expect_args(1, 1); return number(c.ry_train(inum(0))); }
    if (m == "rz_train") { expect_args(1, 1); return number(c.rz_train(inum(0))); }
    if (m == "rx_param") { expect_args(2, 2); c.rx_param(inum(0), inum(1)); return out; }
    if (m == "ry_param") { expect_args(2, 2); c.ry_param(inum(0), inum(1)); return out; }
    if (m == "rz_param") { expect_args(2, 2); c.rz_param(inum(0), inum(1)); return out; }
    if (m == "rx_param_scaled") { expect_args(3, 3); c.rx_param_scaled(inum(0), inum(1), num(2)); return out; }
    if (m == "rzz") { expect_args(3, 3); c.rzz(inum(0), inum(1), num(2)); return out; }
    if (m == "rzz_train") { expect_args(2, 2); return number(c.rzz_train(inum(0), inum(1))); }
    if (m == "rzz_param") { expect_args(3, 4); c.rzz_param(inum(0), inum(1), inum(2), a.size() > 3 ? num(3) : 1.0); return out; }
    if (m == "add_param") { expect_args(1, 1); return number(c.add_param(num(0))); }
    if (m == "ansatz") { expect_args(1, 1); c.ansatz(inum(0)); return out; }
    if (m == "encode_rx" || m == "encode_ry" || m == "encode_rz") {
        expect_args(2, 3);
        const double sc = a.size() > 2 ? num(2) : 1.0;
        if (m == "encode_rx") c.encode_rx(inum(0), inum(1), sc); else if (m == "encode_ry") c.encode_ry(inum(0), inum(1), sc); else c.encode_rz(inum(0), inum(1), sc);
        return out;
    }
    if (m == "encode_all") { expect_args(0, 1); c.encode_all(a.empty() ? 1.0 : num(0)); return out; }
    if (m == "encode_rzz") { expect_args(4, 5); c.encode_rzz(inum(0), inum(1), inum(2), inum(3), a.size() > 4 ? num(4) : 1.0); return out; }
    if (m == "zz_feature_map") { expect_args(0, 2); c.zz_feature_map(a.size() > 0 ? inum(0) : 1, a.size() > 1 ? num(1) : 1.0); return out; }
    if (m == "observable") { expect_args(1, 2); c.observable(str(0), a.size() > 1 ? num(1) : 1.0); return out; }
    if (m == "clear_observable") { expect_args(0, 0); c.clear_observable(); return out; }
    if (m == "observable_string") { expect_args(0, 0); return text(c.observable_string()); }
    if (m == "seed") { expect_args(1, 1); c.seed(static_cast<unsigned long long>(num(0))); return out; }
    if (m == "init_params") { expect_args(0, 0); c.init_params(); return out; }
    if (m == "expect") return number(c.expect_vec(nums(0)));
    if (m == "state") return text(c.state_vec(nums(0)).ket());
    if (m == "energy") { expect_args(0, 0); return number(c.energy()); }
    if (m == "minimize") { expect_args(2, 2); return number(c.minimize(inum(0), num(1))); }
    if (m == "add_sample") { if (a.empty()) throw std::runtime_error("add_sample(rotulo, x0, x1, ...)"); c.add_sample_vec(nums(1), num(0)); return out; }
    if (m == "add_test") { if (a.empty()) throw std::runtime_error("add_test(rotulo, x0, x1, ...)"); c.add_test_vec(nums(1), num(0)); return out; }
    if (m == "load_csv") { expect_args(1, 1); return number(c.load_csv(str(0))); }
    if (m == "clear_data") { expect_args(0, 0); c.clear_data(); return out; }
    if (m == "fit") { expect_args(2, 2); return number(c.fit(inum(0), num(1))); }
    if (m == "score") return number(c.score_vec(nums(0)));
    if (m == "predict") return number(c.predict_vec(nums(0)));
    if (m == "accuracy") { expect_args(0, 0); return number(c.accuracy()); }
    if (m == "test_accuracy") { expect_args(0, 0); return number(c.test_accuracy()); }
    if (m == "loss") { expect_args(0, 0); return number(c.loss()); }
    if (m == "set_verbose") { expect_args(1, 1); c.set_verbose(inum(0)); return out; }
    if (m == "param") { expect_args(1, 1); return number(c.param(inum(0))); }
    if (m == "set_param") { expect_args(2, 2); c.set_param(inum(0), num(1)); return out; }
    if (m == "params") { expect_args(0, 0); return text(c.params()); }
    if (m == "save") { expect_args(1, 1); c.save(str(0)); return out; }
    if (m == "load") { expect_args(1, 1); c.load(str(0)); return out; }
    if (m == "kernel") {
        const auto v = nums(0);
        if (v.size() % 2) throw std::runtime_error("kernel(x0, x1, ..., y0, y1, ...): x e y com o mesmo tamanho");
        return number(c.kernel_vec(std::vector<double>(v.begin(), v.begin() + v.size() / 2), std::vector<double>(v.begin() + v.size() / 2, v.end())));
    }
    if (m == "kernel_fit") { expect_args(0, 1); return number(c.kernel_fit(a.empty() ? 1e-3 : num(0))); }
    if (m == "kernel_predict") return number(c.kernel_predict_vec(nums(0)));
    if (m == "kernel_score") return number(c.kernel_score_vec(nums(0)));
    if (m == "kernel_accuracy") { expect_args(0, 0); return number(c.kernel_accuracy()); }
    if (m == "kernel_test_accuracy") { expect_args(0, 0); return number(c.kernel_test_accuracy()); }
    if (m == "kernel_alignment") { expect_args(0, 0); return number(c.kernel_alignment()); }
    if (m == "kernel_matrix") { expect_args(0, 1); return text(c.kernel_matrix(a.empty() ? 8 : inum(0))); }
    if (m == "edge") { expect_args(2, 3); c.edge(inum(0), inum(1), a.size() > 2 ? num(2) : 1.0); return out; }
    if (m == "clear_edges") { expect_args(0, 0); c.clear_edges(); return out; }
    if (m == "qaoa") { expect_args(1, 1); c.qaoa(inum(0)); return out; }
    if (m == "expected_cut") { expect_args(0, 0); return number(c.expected_cut()); }
    if (m == "maxcut_exact") { expect_args(0, 0); return number(c.maxcut_exact()); }
    if (m == "cut_of") { expect_args(1, 1); return number(c.cut_of(str(0))); }
    if (m == "maxcut_solution") { expect_args(1, 1); return text(c.maxcut_solution(inum(0))); }
    if (m == "size") { expect_args(0, 0); return number(c.size()); }
    if (m == "num_params") { expect_args(0, 0); return number(c.num_params()); }
    if (m == "num_features") { expect_args(0, 0); return number(c.num_features()); }
    if (m == "samples") { expect_args(0, 0); return number(c.samples()); }
    if (m == "test_samples") { expect_args(0, 0); return number(c.test_samples()); }
    if (m == "weight") { expect_args(0, 0); return number(c.weight()); }
    if (m == "bias") { expect_args(0, 0); return number(c.bias()); }
    if (m == "draw") { expect_args(0, 0); return text(c.draw()); }
    if (m == "to_qasm" || m == "qasm") { expect_args(0, 0); return text(c.to_qasm()); }
    if (m == "load_qasm") { expect_args(1, 1); c = GhostQCircuit::from_qasm(str(0)); return out; }
    if (m == "to_string") { expect_args(0, 0); return text(c.to_string()); }
    if (m == "display" || m == "show") { expect_args(0, 0); c.display(); return out; }
    throw std::runtime_error("qcircuit." + m + " nao existe (veja docs/QUANTUM.md)");
}

// Usado por evaluateMath/evaluateStringExpression para converter números com precisão total.
std::string ghost_repl_precise(double v) { return precise(v); }
