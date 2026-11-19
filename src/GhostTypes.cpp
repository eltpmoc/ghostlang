// GhostTypes.cpp - tabela de símbolos e verificador de tipos (c41). Veja include/GhostTypes.hpp.
//
// Duas passadas sobre a AST:
//   1. declarações do nível principal (funções com sobrecarga, classes, interfaces, circuits, globais);
//   2. corpos, com escopos de bloco: resolve nomes, calcula o tipo de cada expressão e confere os usos.
// Tipo desconhecido (genéricos T, json, retorno de função do runtime, ...) nunca gera erro.
#include "../include/GhostTypes.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "../include/GhostBuiltins.hpp"
#include "../include/GhostModules.hpp"
#include "../include/GhostQuantumOps.hpp"

namespace {

struct Ty {
    std::string n;           // vazio = desconhecido
    std::vector<Ty> a;       // argumentos de tipo
};
bool known(const Ty& t) { return !t.n.empty(); }
Ty named(const std::string& n) { return Ty{n, {}}; }

std::string show(const Ty& t) {
    if (!known(t)) return "?";
    if (t.a.empty()) return t.n;
    std::string s = t.n + "<";
    for (size_t i = 0; i < t.a.size(); ++i) s += (i ? ", " : "") + show(t.a[i]);
    return s + ">";
}

struct Sig {
    Ty ret;
    std::vector<Ty> params;
    size_t minArgs = 0;
    bool generic = false;
    bool isStatic = false;
    bool isPrivate = false;
    bool hasBody = false;
    int line = 0;
};

struct ClassInfo {
    std::string base;                 // classe base (vazio: nenhuma)
    std::vector<std::string> ifaces;
    std::set<std::string> typeParams;
    std::map<std::string, Ty> fields;
    std::multimap<std::string, Sig> methods;
    std::map<std::string, char> access; // membro -> 'v' private, 'p' protected (ausente: public)
    std::vector<Sig> ctors;
    bool generic = false;
};

struct IfaceInfo {
    std::vector<std::string> bases;
    std::set<std::string> typeParams;
    std::multimap<std::string, Sig> methods;
};

struct VarInfo {
    Ty type;
    int line = 0;
};

struct FnCtx {
    std::string what;      // "a funcao f", "o metodo m", "o construtor de P", "a lambda"
    Ty ret;
    bool isVoid = false;
    bool isResult = false;
    bool checkReturn = true;
};

enum Cat { CUnknown, CNum, CStr, CCx, CGhost, CList, CMap, CSet, CClass };

class Checker {
public:
    std::vector<AstDiagnostic> errors;

    void run(const AstNode& program) {
        declare(program);
        for (const auto& k : program.kids) {
            if (k->kind == AstKind::ClassDecl) checkImplements(*k);
        }
        scopes.emplace_back();  // nível principal (comandos soltos)
        for (const auto& k : program.kids) top(*k);
        scopes.pop_back();
    }

private:
    std::multimap<std::string, Sig> functions;
    std::map<std::string, ClassInfo> classes;
    std::map<std::string, IfaceInfo> ifaces;
    std::set<std::string> circuits;
    std::map<std::string, VarInfo> globals;

    std::vector<std::map<std::string, VarInfo>> scopes;
    std::vector<FnCtx> fns;
    std::vector<std::set<std::string>> typeParams;  // tipos genéricos ativos (T, U, ...)
    std::string curClass, curIface;
    bool staticCtx = false;
    bool matOpOk = false;        // dentro de x = conv(...) / x = transpose(...) (forma de comando)
    bool ghostAlgebra = false;   // dentro do valor de uma variável ghost (i e theta são unidades)
    int loopDepth = 0, switchDepth = 0;
    int finallyDepth = 0;
    int comprehensionDepth = 0;
    int spawnDepth = 0;
    int hypothesisDepth = 0;
    bool inReturnExpr = false;
    std::vector<std::string> classDeclOrder;
    std::vector<std::string> ifaceDeclOrder;

    struct LambdaCtx {
        bool hasCaps = false;
        int defMode = 0; // '=' ou '&' ou 0
        std::set<std::string> captures;
        size_t scopeIndex = 0;
    };
    std::vector<LambdaCtx> lambdaStack;
    std::set<int> errLines;

    // ------------------------------------------------------------ diagnósticos
    void error(const AstNode& at, const std::string& code, const std::string& msg) {
        if (errLines.count(at.line) || errors.size() >= 50) return;
        errLines.insert(at.line);
        AstDiagnostic d;
        d.line = at.line;
        d.col = at.col;
        d.msg = msg;
        d.code = code;
        errors.push_back(d);
    }

    // ------------------------------------------------------------ tipos
    bool isTypeParam(const std::string& n) const {
        for (const auto& s : typeParams)
            if (s.count(n)) return true;
        return false;
    }
    static bool builtinType(const std::string& n) {
        static const std::set<std::string> b = {"int", "double", "float", "bool", "string", "void", "complex", "ghost",
                                                "json", "date", "time", "datetime", "list", "map", "set", "result", "fn",
                                                "qreg", "qcircuit", "qlayer", "textmodel", "llm", "matrix", "GhostMatrix"};
        return b.count(n) > 0;
    }
    Ty fromType(const AstNode& t) const {
        if (t.kind != AstKind::Type) return Ty{};
        const std::string& n = t.text;
        if (n == "var" || isTypeParam(n)) return Ty{};
        if (!builtinType(n) && !classes.count(n)) return Ty{};  // interface ou tipo do runtime: sem conferência
        Ty r = named(n == "float" ? "double" : n);
        for (const auto& k : t.kids) r.a.push_back(fromType(*k));
        return r;
    }
    Cat cat(const Ty& t) const {
        const std::string& n = t.n;
        if (n == "int" || n == "double" || n == "bool") return CNum;
        if (n == "string") return CStr;
        if (n == "complex") return CCx;
        if (n == "ghost") return CGhost;
        if (n == "list") return CList;
        if (n == "map") return CMap;
        if (n == "set") return CSet;
        if (classes.count(n)) return CClass;
        return CUnknown;
    }
    bool derives(std::string from, const std::string& to) const {
        for (int guard = 0; guard < 64 && !from.empty(); ++guard) {
            if (from == to) return true;
            auto it = classes.find(from);
            if (it == classes.end()) return true;  // base desconhecida: não dá para negar
            from = it->second.base;
        }
        return false;
    }
    bool hasOneArgCtor(const std::string& cls) const {
        auto it = classes.find(cls);
        if (it == classes.end()) return true;
        for (const auto& c : it->second.ctors)
            if (c.minArgs <= 1 && c.params.size() >= 1) return true;
        return false;
    }
    // O valor src não cabe em target (o g++ recusaria). Só responde sim quando tem certeza.
    bool incompatible(const Ty& target, const Ty& src) const {
        const Cat t = cat(target), s = cat(src);
        if (t == CUnknown || s == CUnknown) return false;
        auto elems = [&]() {
            if (target.a.size() != src.a.size()) return false;
            for (size_t i = 0; i < target.a.size(); ++i)
                if (incompatible(target.a[i], src.a[i])) return true;
            return false;
        };
        switch (t) {
            case CNum: return s == CStr || s == CList || s == CMap || s == CSet || s == CClass || s == CCx;
            case CStr: return s == CNum || s == CList || s == CMap || s == CSet || s == CClass || s == CCx;
            case CCx: return s == CStr || s == CList || s == CMap || s == CSet || s == CClass;
            case CGhost: return s == CStr || s == CList || s == CMap || s == CSet;
            case CList: return s == CNum || s == CStr || s == CCx || s == CClass || s == CMap || (s == CList && elems());
            case CSet: return s == CNum || s == CStr || s == CCx || s == CClass || s == CMap || (s == CSet && elems());
            case CMap: return s == CNum || s == CStr || s == CCx || s == CClass || s == CList || s == CSet || (s == CMap && elems());
            case CClass:
                if (s == CClass && (target.n == src.n || derives(src.n, target.n))) return false;
                return !hasOneArgCtor(target.n);
            default: return false;
        }
    }
    Ty promote(const Ty& x, const Ty& y) const {
        if (cat(x) != CNum || cat(y) != CNum) return Ty{};
        if (x.n == "double" || y.n == "double") return named("double");
        return named("int");
    }
    Ty elementOf(const Ty& c) const {
        if ((c.n == "list" || c.n == "set") && c.a.size() == 1) return c.a[0];
        if (c.n == "string") return named("string");
        return Ty{};
    }

    // ------------------------------------------------------------ nomes
    const VarInfo* lookupLocal(const std::string& n) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(n);
            if (f != it->end()) return &f->second;
        }
        return nullptr;
    }
    // membro (campo ou método) da classe ou das bases; unknownBase = alguma base não é conhecida
    bool classMember(const std::string& cls, const std::string& name, bool& unknownBase, const Ty** fieldType,
                     std::vector<const Sig*>* methods) const {
        std::string c = cls;
        bool found = false;
        for (int guard = 0; guard < 64 && !c.empty(); ++guard) {
            auto it = classes.find(c);
            if (it == classes.end()) {
                unknownBase = true;
                return found;
            }
            const ClassInfo& ci = it->second;
            auto f = ci.fields.find(name);
            if (f != ci.fields.end()) {
                if (fieldType && !*fieldType) *fieldType = &f->second;
                found = true;
            }
            auto r = ci.methods.equal_range(name);
            for (auto m = r.first; m != r.second; ++m) {
                found = true;
                if (methods) methods->push_back(&m->second);
            }
            for (const auto& i : ci.ifaces)
                if (ifaceMember(i, name, methods)) found = true;
            if (!ci.ifaces.empty() && !found) {
                for (const auto& i : ci.ifaces)
                    if (!ifaces.count(i)) unknownBase = true;
            }
            c = ci.base;
        }
        return found;
    }
    // private: so dentro da classe dona; protected: na dona e nas derivadas (como em C++/Java)
    void checkAccess(const AstNode& at, const std::string& cls, const std::string& name) {
        std::string owner, c = cls;
        char acc = 0;
        for (int guard = 0; guard < 64 && !c.empty(); ++guard) {
            auto it = classes.find(c);
            if (it == classes.end()) return;
            const ClassInfo& ci = it->second;
            if (ci.fields.count(name) || ci.methods.count(name)) {
                owner = c;
                auto a = ci.access.find(name);
                acc = a == ci.access.end() ? 0 : a->second;
                break;
            }
            c = ci.base;
        }
        if (!acc) return;
        if (curClass == owner) return;
        if (acc == 'p') {
            std::string d = curClass;
            for (int guard = 0; guard < 64 && !d.empty(); ++guard) {
                if (d == owner) return;
                auto it = classes.find(d);
                if (it == classes.end()) break;
                d = it->second.base;
            }
        }
        error(at, "GHOST-E0303", "'" + name + "' e " + (acc == 'v' ? "private" : "protected") + " na classe " + owner +
              (acc == 'v' ? " (so a propria classe acessa)" : " (so a classe e as derivadas acessam)"));
    }
    bool ifaceMember(const std::string& iface, const std::string& name, std::vector<const Sig*>* methods) const {
        auto it = ifaces.find(iface);
        if (it == ifaces.end()) return false;
        bool found = false;
        auto r = it->second.methods.equal_range(name);
        for (auto m = r.first; m != r.second; ++m) {
            found = true;
            if (methods) methods->push_back(&m->second);
        }
        for (const auto& b : it->second.bases)
            if (ifaceMember(b, name, methods)) found = true;
        return found;
    }
    bool isBuiltin(const std::string& n, bool asCall) const {
        if (ghost_builtin_values().count(n)) return true;
        if (asCall && ghost_builtin_functions().count(n)) return true;
        if (asCall && ghost_is_quantum_op(n)) return true;     // h(q), x(q), cx(a, b)
        if (!asCall && (n == "i" || (ghostAlgebra && n == "theta"))) return true; // constante imaginaria i / theta
        if (n.rfind("ghost_", 0) == 0) return true;
        auto stripped = [&](const char* suf) {
            const size_t k = std::string(suf).size();
            return n.size() > k && n.compare(n.size() - k, k, suf) == 0 && circuits.count(n.substr(0, n.size() - k));
        };
        return stripped("_dg") || stripped("_ctrl");
    }
    // nome conhecido como valor ou função (fora as variáveis locais)
    bool knownName(const std::string& n, bool asCall = false) {
        if (globals.count(n) || functions.count(n) || classes.count(n) || ifaces.count(n) || circuits.count(n)) return true;
        if (isTypeParam(n) || isBuiltin(n, asCall) || builtinType(n)) return true; // qreg(4), bool(x), int(x)
        if (!curClass.empty()) {
            bool unknownBase = false;
            if (classMember(curClass, n, unknownBase, nullptr, nullptr) || unknownBase) return true;
        }
        if (!curIface.empty() && ifaceMember(curIface, n, nullptr)) return true;
        return false;
    }
    Ty varType(const std::string& n) {
        if (const VarInfo* v = lookupLocal(n)) return v->type;
        if (!curClass.empty()) {
            bool unknownBase = false;
            const Ty* ft = nullptr;
            if (classMember(curClass, n, unknownBase, &ft, nullptr) && ft) return *ft;
        }
        auto g = globals.find(n);
        if (g != globals.end()) return g->second.type;
        return Ty{};
    }

    void declareLocal(const AstNode& at, const std::string& n, const Ty& t) {
        if (scopes.empty()) return;
        auto& s = scopes.back();
        auto it = s.find(n);
        if (it != s.end()) {
            error(at, "GHOST-E0204", "'" + n + "' ja foi declarado neste bloco (linha " + std::to_string(it->second.line) + ")");
            return;
        }
        s[n] = VarInfo{t, at.line};
    }

    static bool sameParams(const std::vector<Ty>& a, const std::vector<Ty>& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (known(a[i]) && known(b[i]) && show(a[i]) != show(b[i])) return false;
        }
        return true;
    }

    void validateTypeParams(const AstNode& owner, const std::string& prefix) {
        std::set<std::string> seenTP;
        for (const auto& k : owner.kids) {
            if (k->kind != AstKind::TypeParam) continue;
            if (!seenTP.insert(k->name).second) {
                error(*k, "GHOST-E0501", prefix + ": tipo " + k->name + " repetido entre < >");
            }
            if (classes.count(k->name)) {
                error(*k, "GHOST-E0501", prefix + ": '" + k->name + "' ja e uma classe; use outro nome para o tipo");
            }
            for (const auto& restr : k->kids) {
                if (k->attr == "extends") {
                    if (restr->text != "number" && !classes.count(restr->text)) {
                        error(*restr, "GHOST-E0501", prefix + ": " + k->name + " extends " + restr->text + ": '" + restr->text + "' nao e uma classe declarada (use uma classe ou number)");
                    }
                } else if (k->attr == "implements") {
                    if (!ifaces.count(restr->text)) {
                        error(*restr, "GHOST-E0501", prefix + ": " + k->name + " implements " + restr->text + ": '" + restr->text + "' nao e uma interface declarada");
                    }
                }
            }
        }
    }

    struct IfaceReq {
        std::string ifaceOrigin;
        Sig sig;
    };
    std::map<std::string, IfaceReq> allIfaceMethods(const std::string& iface) const {
        std::map<std::string, IfaceReq> res;
        auto it = ifaces.find(iface);
        if (it == ifaces.end()) return res;
        for (const auto& b : it->second.bases) {
            auto sub = allIfaceMethods(b);
            for (auto& p : sub) res[p.first] = p.second;
        }
        for (const auto& m : it->second.methods) {
            res[m.first] = IfaceReq{iface, m.second};
        }
        return res;
    }

    void classAndBaseMethods(const std::string& cls, const std::string& name, std::vector<const Sig*>& methods) const {
        std::string c = cls;
        for (int guard = 0; guard < 64 && !c.empty(); ++guard) {
            auto it = classes.find(c);
            if (it == classes.end()) return;
            const ClassInfo& ci = it->second;
            auto r = ci.methods.equal_range(name);
            for (auto m = r.first; m != r.second; ++m) {
                methods.push_back(&m->second);
            }
            c = ci.base;
        }
    }

    void checkImplements(const AstNode& k) {
        ClassInfo& ci = classes[k.name];
        for (const auto& ifaceName : ci.ifaces) {
            auto itIface = ifaces.find(ifaceName);
            if (itIface == ifaces.end()) continue;
            auto reqs = allIfaceMethods(ifaceName);
            for (const auto& req : reqs) {
                const std::string& mName = req.first;
                const Sig& reqSig = req.second.sig;
                std::vector<const Sig*> foundMethods;
                classAndBaseMethods(k.name, mName, foundMethods);
                const Sig* exact = nullptr;
                for (const Sig* s : foundMethods) {
                    if (sameParams(s->params, reqSig.params)) { exact = s; break; }
                }
                if (!exact) {
                    if (!foundMethods.empty() && !reqSig.hasBody) {
                        const Sig* anyM = foundMethods[0];
                        if (anyM->generic) error(k, "GHOST-E0401", k.name + "." + mName + "() e generico; a interface " + ifaceName + " precisa de um metodo normal");
                        else if (anyM->isStatic) error(k, "GHOST-E0401", k.name + "." + mName + "() e static; a interface " + ifaceName + " precisa de um metodo normal");
                        else if (anyM->isPrivate) error(k, "GHOST-E0401", k.name + "." + mName + "() e private; metodos da interface " + ifaceName + " precisam ser publicos");
                        else if (anyM->params.size() == reqSig.params.size()) {
                            bool diff = false;
                            for (size_t pi = 0; pi < anyM->params.size(); ++pi) {
                                if (known(anyM->params[pi]) && known(reqSig.params[pi]) && show(anyM->params[pi]) != show(reqSig.params[pi])) {
                                    diff = true; break;
                                }
                            }
                            if (diff) {
                                error(k, "GHOST-E0401", k.name + "." + mName + "() tem tipos diferentes dos da interface " + ifaceName + " (esperado " + show(reqSig.ret) + " " + mName + "(...))");
                            }
                        }
                    } else if (!reqSig.hasBody) {
                        error(k, "GHOST-E0401", "a classe " + k.name + " implementa " + ifaceName + " mas nao tem o metodo " + mName + "()");
                    }
                    continue;
                }
                if (exact->generic) error(k, "GHOST-E0401", k.name + "." + mName + "() e generico; a interface " + ifaceName + " precisa de um metodo normal");
                else if (exact->isStatic) error(k, "GHOST-E0401", k.name + "." + mName + "() e static; a interface " + ifaceName + " precisa de um metodo normal");
                else if (exact->isPrivate) error(k, "GHOST-E0401", k.name + "." + mName + "() e private; metodos da interface " + ifaceName + " precisam ser publicos");
                else if (known(exact->ret) && known(reqSig.ret) && show(exact->ret) != show(reqSig.ret))
                    error(k, "GHOST-E0401", k.name + "." + mName + "() tem tipos diferentes dos da interface " + ifaceName + " (esperado " + show(reqSig.ret) + " " + mName + "(...))");
            }
        }
    }

    // ------------------------------------------------------------ passada 1: declarações
    Sig signature(const AstNode& f) {
        Sig s;
        s.line = f.line;
        std::set<std::string> tps;
        for (const auto& k : f.kids)
            if (k->kind == AstKind::TypeParam) tps.insert(k->name);
        s.generic = !tps.empty();
        typeParams.push_back(tps);
        if (!f.kids.empty()) s.ret = fromType(*f.kids[0]);
        if (!f.kids.empty() && f.kids[0]->text == "void") s.ret = named("void");
        for (const auto& k : f.kids) {
            if (k->kind != AstKind::Param) continue;
            s.params.push_back(fromType(*k->kids[0]));
            if (k->kids.size() < 2) s.minArgs = s.params.size();
        }
        typeParams.pop_back();
        return s;
    }

    void declare(const AstNode& program) {
        // classes e interfaces primeiro (os tipos dos campos e parâmetros podem citar qualquer uma)
        for (const auto& k : program.kids) {
            if (k->kind == AstKind::ClassDecl) classes[k->name];
            if (k->kind == AstKind::InterfaceDecl) ifaces[k->name];
        }
        for (const auto& kp : program.kids) {
            const AstNode& k = *kp;
            switch (k.kind) {
                case AstKind::FuncDecl: {
                    Sig s = signature(k);
                    if (k.name == "main" && s.generic) {
                        error(k, "GHOST-E0501", "main nao pode ser generica");
                    }
                    validateTypeParams(k, "function " + k.name);
                    functions.insert({k.name, s});
                    break;
                }
                case AstKind::CircuitDecl: {
                    circuits.insert(k.name);
                    if (ghost_is_quantum_op(k.name)) {
                        error(k, "GHOST-E0901", "'" + k.name + "' ja e uma porta/metodo quantico; escolha outro nome para o circuit.");
                    }
                    if (k.name.size() > 3 && k.name.compare(k.name.size() - 3, 3, "_dg") == 0) {
                        error(k, "GHOST-E0901", "nomes terminados em _dg sao reservados para o inverso");
                    }
                    if (k.name.size() > 5 && k.name.compare(k.name.size() - 5, 5, "_ctrl") == 0) {
                        error(k, "GHOST-E0901", "nomes terminados em _ctrl sao reservados para a versao controlada");
                    }
                    std::set<std::string> seenCircuitParams;
                    for (const auto& p : k.kids) {
                        if (p->kind == AstKind::Param) {
                            if (!seenCircuitParams.insert(p->name).second) {
                                error(*p, "GHOST-E0901", "parametro '" + p->name + "' repetido em circuit " + k.name + ".");
                            }
                        }
                    }
                    break;
                }
                case AstKind::VarDecl: {
                    if (k.kids[0]->text == "void") {
                        error(k, "GHOST-E0201", "variavel '" + k.name + "' nao pode ser void.");
                    }
                    if (k.kids[0]->text == "var" && k.kids.size() < 2) {
                        error(k, "GHOST-E0201", "'var " + k.name + "' precisa de valor inicial (ou declare o tipo).");
                    }
                    auto it = globals.find(k.name);
                    if (it != globals.end()) {
                        error(k, "GHOST-E0204", "'" + k.name + "' ja foi declarado (linha " + std::to_string(it->second.line) + ")");
                        break;
                    }
                    globals[k.name] = VarInfo{fromType(*k.kids[0]), k.line};
                    break;
                }
                case AstKind::Domain:
                    if (!k.name.empty() && k.text != "=>") globals[k.name] = VarInfo{domainType(k), k.line};
                    break;
                case AstKind::ClassDecl: declareClass(k); break;
                case AstKind::InterfaceDecl: {
                    ifaceDeclOrder.push_back(k.name);
                    validateTypeParams(k, "interface " + k.name);
                    IfaceInfo& ii = ifaces[k.name];
                    for (const auto& c : k.kids) {
                        if (c->kind == AstKind::TypeParam) ii.typeParams.insert(c->name);
                        if (c->kind == AstKind::Type && c->attr == "extends") {
                            ii.bases.push_back(c->text);
                            if (std::find(ifaceDeclOrder.begin(), ifaceDeclOrder.end(), c->text) == ifaceDeclOrder.end()) {
                                error(k, "GHOST-E0401", "interface " + k.name + ": '" + c->text + "' nao e uma interface declarada antes");
                            }
                        }
                    }
                    typeParams.push_back(ii.typeParams);
                    std::set<std::string> seenIfaceMethods;
                    for (const auto& c : k.kids) {
                        if (c->kind == AstKind::FuncDecl) {
                            if (!seenIfaceMethods.insert(c->name).second) {
                                error(*c, "GHOST-E0401", "interface " + k.name + ": metodo " + c->name + " declarado duas vezes");
                            }
                            Sig s = signature(*c);
                            s.hasBody = !c->kids.empty() && c->kids.back()->kind == AstKind::Block;
                            if (s.generic) {
                                error(*c, "GHOST-E0401", "interface " + k.name + ": metodos de interface nao podem ser genericos (use interface " + k.name + "<T>)");
                            }
                            for (const auto& p : c->kids) {
                                if (p->kind == AstKind::Param && p->kids.size() > 1) {
                                    error(*p, "GHOST-E0401", "interface " + k.name + ": parametros de interface nao tem valor padrao");
                                }
                            }
                            ii.methods.insert({c->name, s});
                        }
                    }
                    typeParams.pop_back();
                    break;
                }
                default: break;
            }
        }
    }

    Ty domainType(const AstNode& d) const {
        if (d.text == "qreg" || d.text == "qcircuit" || d.text == "qlayer") return named(d.text);
        return Ty{};
    }

    void declareClass(const AstNode& k) {
        classDeclOrder.push_back(k.name);
        validateTypeParams(k, "class " + k.name);
        ClassInfo& ci = classes[k.name];
        for (const auto& c : k.kids) {
            if (c->kind == AstKind::TypeParam) ci.typeParams.insert(c->name);
            if (c->kind == AstKind::Type && c->attr == "extends") ci.base = c->text;
            if (c->kind == AstKind::Type && c->attr == "implements") ci.ifaces.push_back(c->text);
        }
        if (!ci.base.empty()) {
            if (ci.typeParams.count(ci.base)) {
                error(k, "GHOST-E0301", "class " + k.name + ": a classe base nao pode depender de " + ci.base + " (use um tipo fixo, ex.: extends Base<int>)");
            } else if (std::find(classDeclOrder.begin(), classDeclOrder.end(), ci.base) == classDeclOrder.end()) {
                error(k, "GHOST-E0301", "a classe base '" + ci.base + "' precisa ser declarada antes de " + k.name + ".");
            }
        }
        for (const auto& iface : ci.ifaces) {
            if (!ifaces.count(iface)) {
                error(k, "GHOST-E0401", "'" + iface + "' nao e uma interface declarada (implements de " + k.name + ").");
            }
        }
        ci.generic = !ci.typeParams.empty();
        typeParams.push_back(ci.typeParams);
        std::map<std::string, int> fieldLine;
        for (const auto& c : k.kids) {
            if (c->kind == AstKind::VarDecl || (c->kind == AstKind::Domain && !c->name.empty())) {
                if (c->kind == AstKind::VarDecl) {
                    if (c->kids[0]->text == "void") {
                        error(*c, "GHOST-E0201", "variavel '" + c->name + "' nao pode ser void.");
                    }
                    if (c->kids[0]->text == "var") {
                        error(*c, "GHOST-E0301", "campo '" + c->name + "' da classe precisa de tipo (var nao vale em campos).");
                    }
                    if (c->has("static") && c->kids[0]->text == k.name) {
                        error(*c, "GHOST-E0301", "campo static '" + c->name + "' nao pode ser do tipo da propria classe.");
                    }
                } else if (c->kind == AstKind::Domain) {
                    if (c->text == "qlayer") {
                        error(*c, "GHOST-E0301", "campos qlayer nao existem em classes (use qreg ou qcircuit).");
                    }
                    if (c->has("static") && (c->text == "qreg" || c->text == "qcircuit")) {
                        error(*c, "GHOST-E0301", "campos static " + c->text + " nao existem (use um campo normal)");
                    }
                }
                auto it = fieldLine.find(c->name);
                if (it != fieldLine.end()) {
                    error(*c, "GHOST-E0204", "campo '" + c->name + "' ja foi declarado na classe " + k.name + " (linha " +
                                                 std::to_string(it->second) + ")");
                    continue;
                }
                fieldLine[c->name] = c->line;
                ci.fields[c->name] = c->kind == AstKind::VarDecl ? fromType(*c->kids[0]) : domainType(*c);
                if (c->has("private")) ci.access[c->name] = 'v';
                else if (c->has("protected")) ci.access[c->name] = 'p';
            } else if (c->kind == AstKind::FuncDecl) {
                Sig s = signature(*c);
                s.isStatic = c->has("static");
                s.isPrivate = c->has("private");
                if (c->name != k.name) {
                    if (c->has("private")) ci.access[c->name] = 'v';
                    else if (c->has("protected")) ci.access[c->name] = 'p';
                }
                s.hasBody = !c->kids.empty() && c->kids.back()->kind == AstKind::Block;
                if (c->has("ctor")) {
                    if (s.generic) error(*c, "GHOST-E0301", "o construtor de " + k.name + " nao pode ser generico");
                    if (s.isStatic) error(*c, "GHOST-E0301", "o construtor de " + k.name + " nao pode ser static");
                    ci.ctors.push_back(s);
                } else {
                    validateTypeParams(*c, k.name + "." + c->name);
                    if (!ci.base.empty()) {
                        auto itBase = classes.find(ci.base);
                        if (itBase != classes.end()) {
                            auto br = itBase->second.methods.equal_range(c->name);
                            for (auto bit = br.first; bit != br.second; ++bit) {
                                if (sameParams(bit->second.params, s.params)) {
                                    if (bit->second.generic) {
                                        error(*c, "GHOST-E0301", k.name + "." + c->name + "(): o metodo generico de " + ci.base + " nao pode ser redefinido (metodos genericos nao sao virtuais)");
                                    } else if (s.generic) {
                                        error(*c, "GHOST-E0301", k.name + "." + c->name + "(): um metodo generico nao pode redefinir o metodo " + c->name + " de " + ci.base);
                                    }
                                }
                            }
                        }
                    }
                    auto range = ci.methods.equal_range(c->name);
                    for (auto it = range.first; it != range.second; ++it) {
                        if (sameParams(it->second.params, s.params)) {
                            error(*c, "GHOST-E0301", "class " + k.name + ": metodo " + c->name + " com os mesmos parametros declarado duas vezes");
                            break;
                        }
                    }
                    ci.methods.insert({c->name, s});
                }
            }
        }
        typeParams.pop_back();
    }

    // ------------------------------------------------------------ passada 2: corpos
    void top(const AstNode& n) {
        switch (n.kind) {
            case AstKind::FuncDecl: function(n, "a funcao " + n.name); return;
            case AstKind::ClassDecl: classBody(n); return;
            case AstKind::InterfaceDecl: {
                curIface = n.name;
                typeParams.push_back(ifaces[n.name].typeParams);
                for (const auto& c : n.kids)
                    if (c->kind == AstKind::FuncDecl && !c->has("abstract")) function(*c, "o metodo " + c->name);
                typeParams.pop_back();
                curIface.clear();
                return;
            }
            case AstKind::CircuitDecl: {
                scopes.emplace_back();
                for (const auto& p : n.kids)
                    if (p->kind == AstKind::Param) declareLocal(*p, p->name, fromType(*p->kids[0]));
                fns.push_back(FnCtx{"o circuit " + n.name, Ty{}, false, false});
                for (const auto& s : n.kids.back()->kids) stmt(*s);
                fns.pop_back();
                scopes.pop_back();
                return;
            }
            case AstKind::VarDecl: { // global: confere o valor; var ganha o tipo do valor
                Ty t = fromType(*n.kids[0]);
                if (n.kids.size() > 1) {
                    ghostAlgebra = n.kids[0]->text == "ghost";
                    const Ty v = expr(*n.kids[1]);
                    ghostAlgebra = false;
                    checkInit(n, t, v);
                    if (n.kids[0]->text == "var") globals[n.name].type = v;
                }
                return;
            }
            case AstKind::Domain:
                domain(n, false);
                return;
            case AstKind::Import:
                return;
            default:
                stmt(n);
        }
    }

    void classBody(const AstNode& n) {
        curClass = n.name;
        typeParams.push_back(classes[n.name].typeParams);
        for (const auto& c : n.kids) {
            if (c->kind == AstKind::VarDecl && c->kids.size() > 1) {
                staticCtx = c->has("static");
                checkInit(*c, fromType(*c->kids[0]), expr(*c->kids[1]));
                staticCtx = false;
            } else if (c->kind == AstKind::FuncDecl) {
                staticCtx = c->has("static");
                function(*c, c->has("ctor") ? "o construtor de " + n.name : "o metodo " + c->name);
                staticCtx = false;
            }
        }
        typeParams.pop_back();
        curClass.clear();
    }

    void function(const AstNode& f, const std::string& what) {
        std::set<std::string> tps;
        for (const auto& k : f.kids)
            if (k->kind == AstKind::TypeParam) tps.insert(k->name);
        typeParams.push_back(tps);
        FnCtx ctx;
        ctx.what = what;
        ctx.ret = fromType(*f.kids[0]);
        ctx.isVoid = f.kids[0]->text == "void";
        ctx.isResult = (f.kids[0]->kind == AstKind::Type && f.kids[0]->text == "result");
        scopes.emplace_back();
        const AstNode* body = nullptr;
        for (const auto& k : f.kids) {
            if (k->kind == AstKind::Param) {
                const Ty pt = fromType(*k->kids[0]);
                if (k->kids.size() > 1) checkInit(*k->kids[1], pt, expr(*k->kids[1]), "o valor padrao de '" + k->name + "'");
                declareLocal(*k, k->name, pt);
            } else if (k->kind == AstKind::Block) {
                body = k.get();
            }
        }
        if (body) {
            fns.push_back(ctx);
            const int savedLoop = loopDepth, savedSwitch = switchDepth;
            loopDepth = switchDepth = 0;
            for (const auto& s : body->kids) stmt(*s); // o corpo divide o escopo com os parâmetros (como no C++)
            loopDepth = savedLoop;
            switchDepth = savedSwitch;
            fns.pop_back();
        }
        scopes.pop_back();
        typeParams.pop_back();
    }

    void checkInit(const AstNode& at, const Ty& target, const Ty& value, const std::string& who = "") {
        if (!incompatible(target, value)) return;
        const std::string w = who.empty() ? "a variavel '" + at.name + "'" : who;
        error(at, "GHOST-E0202", "tipo incompativel: " + w + " e " + show(target) + ", mas o valor e " + show(value));
    }

    void block(const AstNode& b) {
        scopes.emplace_back();
        for (const auto& s : b.kids) stmt(*s);
        scopes.pop_back();
    }

    void condition(const AstNode& c, const char* where) {
        const Ty t = expr(c);
        const Cat k = cat(t);
        if (k == CStr || k == CClass || k == CList || k == CMap || k == CSet)
            error(c, "GHOST-E0202", std::string("tipo incompativel: a condicao do ") + where + " precisa ser bool (ou numero), mas e " + show(t));
    }

    void domain(const AstNode& d, bool local) {
        if (d.text == "qreg" || d.text == "qcircuit" || d.text == "qlayer" || d.text == "sleep")
            for (const auto& k : d.kids) expr(*k);
        if (local && !d.name.empty() && d.text != "=>") declareLocal(d, d.name, domainType(d)); // rede => ... não declara
    }

    void stmt(const AstNode& s) {
        switch (s.kind) {
            case AstKind::VarDecl: {
                if (s.kids[0]->text == "void") {
                    error(s, "GHOST-E0201", "variavel '" + s.name + "' nao pode ser void.");
                }
                if (s.kids[0]->text == "var" && s.kids.size() < 2) {
                    error(s, "GHOST-E0201", "'var " + s.name + "' precisa de valor inicial (ou declare o tipo).");
                }
                Ty t = fromType(*s.kids[0]);
                if (s.kids.size() > 1) {
                    ghostAlgebra = s.kids[0]->text == "ghost";
                    const Ty v = expr(*s.kids[1]);
                    ghostAlgebra = false;
                    checkInit(s, t, v);
                    if (s.kids[0]->text == "var") t = v;
                }
                declareLocal(s, s.name, t);
                return;
            }
            case AstKind::ExprStmt: {
                const AstNode& e = *s.kids[0];
                // x = conv(imagem, kernel[, passo]);  x = transpose(a);  (forma de comando, como no Parser antigo)
                if (e.kind == AstKind::Assign && e.text == "=" && e.parens == 0 && e.kids[1]->kind == AstKind::Call &&
                    e.kids[1]->parens == 0 && e.kids[1]->kids[0]->kind == AstKind::Ident &&
                    (e.kids[1]->kids[0]->text == "conv" || e.kids[1]->kids[0]->text == "transpose")) {
                    const AstNode& c = *e.kids[1];
                    const bool isConv = c.kids[0]->text == "conv";
                    auto nome = [&](size_t i) { return i < c.kids.size() && c.kids[i]->kind == AstKind::Ident && c.kids[i]->parens == 0; };
                    const bool ok = e.kids[0]->kind == AstKind::Ident &&
                                    (isConv ? (c.kids.size() == 3 || (c.kids.size() == 4 && c.kids[3]->kind == AstKind::Number)) && nome(1) && nome(2)
                                            : c.kids.size() == 2 && nome(1));
                    if (!ok) {
                        error(c, "GHOST-E0601", isConv ? "use x = conv(imagem, kernel); ou x = conv(imagem, kernel, passo); com nomes de matriz e o passo como numero"
                                                       : "use x = transpose(a); com o nome de uma matriz");
                        return;
                    }
                    matOpOk = true;
                    expr(e);
                    matOpOk = false;
                    return;
                }
                expr(e);
                return;
            }
            case AstKind::Block: block(s); return;
            case AstKind::If:
                condition(*s.kids[0], "if");
                block(*s.kids[1]);
                if (s.kids.size() > 2) stmt(*s.kids[2]);
                return;
            case AstKind::While:
                condition(*s.kids[0], "while");
                ++loopDepth;
                block(*s.kids[1]);
                --loopDepth;
                return;
            case AstKind::DoWhile:
                ++loopDepth;
                block(*s.kids[0]);
                --loopDepth;
                condition(*s.kids[1], "while");
                return;
            case AstKind::For:
                scopes.emplace_back();
                if (s.kids[0]->kind != AstKind::Empty) stmt(*s.kids[0]);
                if (s.kids[1]->kind != AstKind::Empty) condition(*s.kids[1], "for");
                if (s.kids[2]->kind != AstKind::Empty) expr(*s.kids[2]);
                ++loopDepth;
                block(*s.kids[3]);
                --loopDepth;
                scopes.pop_back();
                return;
            case AstKind::Foreach: {
                const Ty coll = expr(*s.kids[1]);
                scopes.emplace_back();
                Ty decl = fromType(*s.kids[0]);
                if (!s.attr.empty()) { // chave, valor
                    Ty k, v;
                    if (coll.n == "map" && coll.a.size() == 2) { k = coll.a[0]; v = coll.a[1]; }
                    declareLocal(s, s.name, k);
                    declareLocal(s, s.attr, v);
                } else {
                    declareLocal(s, s.name, known(decl) ? decl : elementOf(coll));
                }
                ++loopDepth;
                block(*s.kids[2]);
                --loopDepth;
                scopes.pop_back();
                return;
            }
            case AstKind::Switch:
                expr(*s.kids[0]);
                ++switchDepth;
                for (size_t i = 1; i < s.kids.size(); ++i) {
                    const AstNode& c = *s.kids[i];
                    for (size_t k = 0; k + 1 < c.kids.size(); ++k) expr(*c.kids[k]);
                    block(*c.kids.back());
                }
                --switchDepth;
                return;
            case AstKind::Try:
                block(*s.kids[0]);
                for (size_t i = 1; i < s.kids.size(); ++i) {
                    const AstNode& c = *s.kids[i];
                    scopes.emplace_back();
                    if (c.kind == AstKind::Catch && !c.name.empty()) declareLocal(c, c.name, named("string"));
                    if (c.kind == AstKind::Finally) {
                        ++finallyDepth;
                        block(*c.kids[0]);
                        --finallyDepth;
                    } else {
                        block(*c.kids[0]);
                    }
                    scopes.pop_back();
                }
                return;
            case AstKind::Throw:
                if (!s.kids.empty()) expr(*s.kids[0]);
                return;
            case AstKind::Return: returnStmt(s); return;
            case AstKind::Break:
                if (loopDepth == 0 && switchDepth == 0) error(s, "GHOST-E0104", "break fora de laco ou switch");
                return;
            case AstKind::Continue:
                if (loopDepth == 0) error(s, "GHOST-E0104", "continue fora de laco");
                return;
            case AstKind::Print:
                for (const auto& k : s.kids) expr(*k);
                return;
            case AstKind::Spawn:
                ++spawnDepth;
                block(*s.kids[0]);
                --spawnDepth;
                return;
            case AstKind::Collapse:
                if (hypothesisDepth == 0) error(s, "GHOST-E0100", "collapse_" + s.text + " so pode ser usado dentro de hypothesis");
                expr(*s.kids[0]);
                return;
            case AstKind::Hypothesis: {
                const Ty coll = expr(*s.kids[1]);
                scopes.emplace_back();
                const Ty decl = fromType(*s.kids[0]);
                declareLocal(s, s.name, known(decl) ? decl : elementOf(coll));
                ++hypothesisDepth;
                block(*s.kids[2]);
                --hypothesisDepth;
                scopes.pop_back();
                return;
            }
            case AstKind::Domain: domain(s, true); return;
            default: return;
        }
    }

    void returnStmt(const AstNode& s) {
        inReturnExpr = true;
        const Ty v = s.kids.empty() ? Ty{} : expr(*s.kids[0]);
        inReturnExpr = false;
        if (fns.empty() || !fns.back().checkReturn) return;
        const FnCtx& f = fns.back();
        if (f.isVoid && !s.kids.empty()) {
            error(s, "GHOST-E0603", f.what + " nao devolve valor (e void), mas este return tem um valor");
        } else if (!f.isVoid && s.kids.empty()) {
            error(s, "GHOST-E0603", f.what + " devolve " + (known(f.ret) ? show(f.ret) : std::string("um valor")) +
                                        ": o return precisa de um valor");
        } else if (!s.kids.empty() && incompatible(f.ret, v)) {
            error(*s.kids[0], "GHOST-E0202", "tipo incompativel: " + f.what + " devolve " + show(f.ret) + ", mas o return e " + show(v));
        }
    }

    // ------------------------------------------------------------ expressões
    Ty numberType(const std::string& t) const {
        if (!t.empty() && t.back() == 'i') return named("complex");
        if ((!t.empty() && t.back() == 'g') || (t.size() > 5 && t.compare(t.size() - 5, 5, "theta") == 0)) return named("ghost");
        if (t.find_first_of(".eE") != std::string::npos) return named("double");
        return named("int");
    }

    // Confere a quantidade (e, com uma assinatura só, os tipos) dos argumentos
    void checkArgs(const AstNode& call, const std::string& what, const std::vector<const Sig*>& sigs, const std::vector<Ty>& args) {
        if (sigs.empty()) return;
        const size_t n = args.size();
        bool any = false;
        for (const Sig* s : sigs)
            if (n >= s->minArgs && n <= s->params.size()) any = true;
        if (!any) {
            std::string want;
            if (sigs.size() == 1) {
                const Sig& s = *sigs[0];
                want = s.minArgs == s.params.size() ? std::to_string(s.params.size())
                                                    : std::to_string(s.minArgs) + " a " + std::to_string(s.params.size());
                want += s.params.size() == 1 && s.minArgs == 1 ? " argumento" : " argumentos";
            } else {
                want = "outra quantidade de argumentos (" + std::to_string(sigs.size()) + " versoes)";
            }
            error(call, "GHOST-E0602", what + " espera " + want + ", mas recebeu " + std::to_string(n));
            return;
        }
        if (sigs.size() != 1) return;
        const Sig& s = *sigs[0];
        for (size_t i = 0; i < n && i < s.params.size(); ++i)
            if (incompatible(s.params[i], args[i])) {
                error(*call.kids[i + 1], "GHOST-E0202", "tipo incompativel: o argumento " + std::to_string(i + 1) + " " + contract(what) +
                                                           " e " + show(s.params[i]) + ", mas recebeu " + show(args[i]));
                return;
            }
    }
    // "a funcao f" -> "da funcao f", "o metodo m" -> "do metodo m"
    static std::string contract(const std::string& w) {
        if (w.rfind("a ", 0) == 0) return "da " + w.substr(2);
        if (w.rfind("o ", 0) == 0) return "do " + w.substr(2);
        return "de " + w;
    }
    static Ty uniqueRet(const std::vector<const Sig*>& sigs) {
        Ty r;
        for (size_t i = 0; i < sigs.size(); ++i) {
            if (sigs[i]->generic) return Ty{};
            if (i == 0) r = sigs[i]->ret;
            else if (show(r) != show(sigs[i]->ret)) return Ty{};
        }
        return r.n == "void" ? Ty{} : r;
    }
    std::vector<Ty> callArgs(const AstNode& c) {
        std::vector<Ty> out;
        for (size_t i = 1; i < c.kids.size(); ++i) {
            const AstNode& a = *c.kids[i];
            if (a.kind == AstKind::Assign && a.text == "=" && a.kids[0]->kind == AstKind::Ident) { // porta=8080, epochs=10
                out.push_back(expr(*a.kids[1]));
            } else {
                out.push_back(expr(a));
            }
        }
        return out;
    }
    Ty constructorCall(const AstNode& call, const std::string& cls, const std::vector<Ty>& args, std::vector<Ty> typeArgs) {
        const ClassInfo& ci = classes[cls];
        std::vector<const Sig*> sigs;
        for (const auto& c : ci.ctors) sigs.push_back(&c);
        if (!sigs.empty()) checkArgs(call, "o construtor de " + cls, sigs, args);
        if (ci.generic && typeArgs.empty()) return Ty{};
        Ty t = named(cls);
        t.a = std::move(typeArgs);
        return t;
    }

    Ty call(const AstNode& c) {
        const AstNode& callee = *c.kids[0];
        if (callee.kind == AstKind::Super) {
            if (curClass.empty() || classes[curClass].base.empty()) {
                error(c, "GHOST-E0301", "super(...) so vale em classe declarada com extends.");
                return Ty{};
            }
        }
        if (callee.kind == AstKind::Ident) {
            const std::string& n = callee.text;
            if (const VarInfo* v = lookupLocal(n)) { // variável fn<...>
                callArgs(c);
                return v->type.n == "fn" && !v->type.a.empty() ? v->type.a[0] : Ty{};
            }
            const std::vector<Ty> args = callArgs(c);
            if (!curClass.empty()) { // método da própria classe (ou da base), chamado sem objeto
                bool unknownBase = false;
                std::vector<const Sig*> ms;
                const Ty* ft = nullptr;
                if (classMember(curClass, n, unknownBase, &ft, &ms)) {
                    checkAccess(callee, curClass, n); // metodo private da base chamado pela derivada
                    if (!ft && !unknownBase) checkArgs(c, "o metodo " + n, ms, args);
                    return ft ? Ty{} : uniqueRet(ms);
                }
                if (unknownBase) return Ty{};
            }
            if (!curIface.empty()) {
                std::vector<const Sig*> ms;
                if (ifaceMember(curIface, n, &ms)) return uniqueRet(ms);
            }
            auto range = functions.equal_range(n);
            if (range.first != range.second) {
                std::vector<const Sig*> sigs;
                for (auto it = range.first; it != range.second; ++it) sigs.push_back(&it->second);
                checkArgs(c, "a funcao " + n, sigs, args);
                return uniqueRet(sigs);
            }
            if (ifaces.count(n)) {
                error(callee, "GHOST-E0401", n + " e interface: crie um objeto de uma classe que implementa " + n);
                return Ty{};
            }
            if (classes.count(n)) return constructorCall(c, n, args, {});
            if (globals.count(n)) return Ty{};
            if (n == "range") return Ty{"list", {named("int")}};
            if (n == "len") return named("int");
            if (n == "str" || n == "to_string") return named("string");
            if (n == "input" || n == "inputln") return Ty{};
            if (n == "ghost") return named("ghost");
            if (n == "complex") return named("complex");
            if (n == "json") return named("json");
            if (n == "matrix" || n == "GhostMatrix") return named("matrix");
            if (n == "to_4d" || n == "to_math" || n == "to_algebraic") return named("string");
            if (n == "conv" || n == "transpose") { // só como comando: x = conv(imagem, kernel[, passo]); x = transpose(a);
                if (!matOpOk)
                    error(callee, "GHOST-E0601", n + "(...) so vale como comando: x = conv(imagem, kernel, passo); ou x = transpose(a); "
                                                     "(dentro de expressoes use imagem.conv2d(kernel, passo) ou a.transpose())");
                return Ty{};
            }
            if (!knownName(n, true)) error(callee, "GHOST-E0203", "funcao '" + n + "' nao foi declarada");
            return Ty{};
        }
        if (callee.kind == AstKind::GenericRef) {
            const std::string& n = callee.kids[0]->text;
            const std::vector<Ty> args = callArgs(c);
            std::vector<Ty> targs;
            for (size_t i = 1; i < callee.kids.size(); ++i) targs.push_back(fromType(*callee.kids[i]));
            if (classes.count(n)) return constructorCall(c, n, args, targs);
            auto range = functions.equal_range(n);
            if (range.first != range.second) {
                std::vector<const Sig*> sigs;
                for (auto it = range.first; it != range.second; ++it) sigs.push_back(&it->second);
                checkArgs(c, "a funcao " + n, sigs, args);
            }
            return Ty{};
        }
        if (callee.kind == AstKind::Member) {
            const AstNode& obj = *callee.kids[0];
            const std::string& m = callee.text;
            if (obj.kind == AstKind::Ident && ghost_module_aliases().count(obj.text)) {
                error(obj, "GHOST-E1001", "o modulo " + obj.text + " nao tem '" + m + "' (nomes do modulo: funcoes, classes, interfaces e variaveis globais do arquivo importado)");
                return Ty{};
            }
            std::string cls;
            if (obj.kind == AstKind::Ident && classes.count(obj.text) && !lookupLocal(obj.text) && !globals.count(obj.text)) {
                cls = obj.text; // Classe.metodo_static()
            } else {
                const Ty ot = expr(obj);
                if (cat(ot) == CClass) cls = ot.n;
            }
            const std::vector<Ty> args = callArgs(c);
            if (cls.empty()) return Ty{};
            bool unknownBase = false;
            std::vector<const Sig*> ms;
            const Ty* ft = nullptr;
            if (!classMember(cls, m, unknownBase, &ft, &ms)) {
                if (!unknownBase) error(callee, "GHOST-E0302", "a classe " + cls + " nao tem o metodo '" + m + "'");
                return Ty{};
            }
            checkAccess(callee, cls, m);
            if (ft) return (ft->n == "fn" && !ft->a.empty()) ? ft->a[0] : Ty{}; // campo fn<...> chamado
            if (!unknownBase) checkArgs(c, "o metodo " + cls + "." + m, ms, args);
            return classes[cls].generic ? Ty{} : uniqueRet(ms);
        }
        if (callee.kind == AstKind::TypeExpr) { // list<int>()
            callArgs(c);
            return fromType(*callee.kids[0]);
        }
        expr(callee);
        callArgs(c);
        return Ty{};
    }

    Ty member(const AstNode& e) {
        const AstNode& obj = *e.kids[0];
        if (obj.kind == AstKind::Ident && ghost_module_aliases().count(obj.text)) {
            error(obj, "GHOST-E1001", "o modulo " + obj.text + " nao tem '" + e.text + "' (nomes do modulo: funcoes, classes, interfaces e variaveis globais do arquivo importado)");
            return Ty{};
        }
        if (obj.kind == AstKind::Super) {
            if (curClass.empty() || classes[curClass].base.empty()) {
                error(e, "GHOST-E0301", "super so pode ser usado em uma classe declarada com extends");
                return Ty{};
            }
        }
        if (obj.kind == AstKind::Ident && ifaces.count(obj.text)) {
            error(e, "GHOST-E0401", obj.text + " e interface: interfaces nao tem membros static");
            return Ty{};
        }
        std::string cls;
        if (obj.kind == AstKind::Ident && classes.count(obj.text) && !lookupLocal(obj.text) && !globals.count(obj.text)) {
            cls = obj.text; // Classe.campo_static
        } else {
            const Ty ot = expr(obj);
            if (cat(ot) == CClass) cls = ot.n;
            if (cls.empty()) return Ty{};
            if (classes[cls].generic) {
                bool ub = false;
                if (!classMember(cls, e.text, ub, nullptr, nullptr) && !ub)
                    error(e, "GHOST-E0302", "a classe " + cls + " nao tem o campo '" + e.text + "'");
                return Ty{};
            }
        }
        if (cls.empty()) return Ty{};
        bool unknownBase = false;
        const Ty* ft = nullptr;
        if (!classMember(cls, e.text, unknownBase, &ft, nullptr)) {
            if (!unknownBase) error(e, "GHOST-E0302", "a classe " + cls + " nao tem o campo '" + e.text + "'");
            return Ty{};
        }
        checkAccess(e, cls, e.text);
        return ft ? *ft : Ty{};
    }

    Ty ident(const AstNode& e) {
        const std::string& n = e.text;
        if (ghost_module_aliases().count(n)) return named("module");
        if (!lambdaStack.empty()) {
            const LambdaCtx& lc = lambdaStack.back();
            if (lc.hasCaps && lc.defMode == 0) {
                bool isOuterLocal = false;
                for (size_t i = 0; i < lc.scopeIndex && i < scopes.size(); ++i) {
                    if (scopes[i].count(n)) { isOuterLocal = true; break; }
                }
                if (isOuterLocal && !lc.captures.count(n)) {
                    error(e, "GHOST-E0601", "variavel local '" + n + "' nao foi capturada na lambda (adicione " + n + " ou &" + n + " entre [ ])");
                }
            }
        }
        if (n == "true" || n == "false") return named("bool");
        if (lookupLocal(n)) return varType(n);
        if (!curClass.empty()) { // campo usado pelo nome (this implicito): private da base nao vale na derivada
            bool unknownBase = false;
            if (classMember(curClass, n, unknownBase, nullptr, nullptr)) checkAccess(e, curClass, n);
        }
        if (!knownName(n)) {
            error(e, "GHOST-E0203", "'" + n + "' nao foi declarado (declare antes de usar, ex.: int " + n + " = 0; ou var " + n + " = ...;)");
            return Ty{};
        }
        return varType(n);
    }

    Ty assign(const AstNode& e) {
        const AstNode& target = *e.kids[0];
        const Ty v = expr(*e.kids[1]);
        Ty t;
        if (target.kind == AstKind::Ident) {
            const std::string& n = target.text;
            if (!lookupLocal(n) && !knownName(n)) {
                error(target, "GHOST-E0203", "variavel '" + n + "' nao foi declarada (use int " + n + " = ...; ou var " + n + " = ...;)");
                return Ty{};
            }
            t = varType(n);
        } else {
            t = expr(target);
        }
        if (e.text == "=" && incompatible(t, v)) {
            std::string who = target.kind == AstKind::Ident ? "'" + target.text + "'"
                              : target.kind == AstKind::Member ? "o campo '" + target.text + "'" : "o destino";
            error(e, "GHOST-E0202", "tipo incompativel: " + who + " e " + show(t) + ", mas o valor e " + show(v));
        }
        return t;
    }

    Ty binary(const AstNode& e) {
        const Ty l = expr(*e.kids[0]);
        const Ty r = expr(*e.kids[1]);
        const std::string& op = e.text;
        if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            const Cat a = cat(l), b = cat(r);
            if ((a == CStr && b == CNum) || (a == CNum && b == CStr))
                error(e, "GHOST-E0202", "tipo incompativel: nao da para comparar " + show(l) + " com " + show(r) + " usando " + op +
                                            " (converta antes, ex.: str(x) ou int(texto))");
            return named("bool");
        }
        if (op == "&&" || op == "||") return named("bool");
        if (op == "+") {
            if (l.n == "string" || r.n == "string") return named("string");
            if (l.n == "complex" || r.n == "complex") return named("complex");
            return promote(l, r);
        }
        if (op == "-" || op == "*" || op == "/" || op == "%" || op == "**") {
            if ((l.n == "complex" || r.n == "complex") && cat(l) != CStr && cat(r) != CStr && op != "%") {
                if ((cat(l) == CNum || l.n == "complex") && (cat(r) == CNum || r.n == "complex")) return named("complex");
            }
            return promote(l, r);
        }
        if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
            if (l.n == "int" && r.n == "int") return named("int");
            return Ty{};
        }
        return Ty{};
    }

    Ty lambda(const AstNode& e) {
        LambdaCtx lctx;
        lctx.hasCaps = e.has("caps");
        lctx.scopeIndex = scopes.size();
        std::set<std::string> seenCaps;
        bool capturesLocalByRef = false;
        if (lctx.hasCaps) {
            for (const auto& c : e.kids) {
                if (c->kind != AstKind::Capture) continue;
                if (c->text == "=") {
                    lctx.defMode = '=';
                } else if (c->text == "&") {
                    lctx.defMode = '&';
                } else if (c->text == "this") {
                    if (curClass.empty() || staticCtx) {
                        error(*c, "GHOST-E0601", "this na captura da lambda so vale em metodo de instancia");
                    } else if (!seenCaps.insert("this").second) {
                        error(*c, "GHOST-E0601", "'this' repetido na captura da lambda");
                    } else if (lctx.defMode == '=' && !c->has("ref")) {
                        error(*c, "GHOST-E0601", "com '=' o objeto this ja e copiado; use [&this] para capturar por referencia");
                    } else if (lctx.defMode == '&' && c->has("ref")) {
                        error(*c, "GHOST-E0601", "com '&' o objeto this ja e capturado por referencia; use [this] para copiar");
                    }
                    lctx.captures.insert("this");
                } else {
                    const std::string& v = c->text;
                    if (!seenCaps.insert(v).second) {
                        error(*c, "GHOST-E0601", "variavel '" + v + "' repetida na captura da lambda");
                    } else if (lctx.defMode == '=' && !c->has("ref")) {
                        error(*c, "GHOST-E0601", "com '=' as variaveis ja sao capturadas por valor; liste apenas &variavel (ex.: [=, &" + v + "])");
                    } else if (lctx.defMode == '&' && c->has("ref")) {
                        error(*c, "GHOST-E0601", "com '&' as variaveis ja sao capturadas por referencia; liste apenas variaveis por valor (ex.: [&, " + v + "])");
                    } else if (!lookupLocal(v) && !globals.count(v)) {
                        error(*c, "GHOST-E0203", "variavel '" + v + "' nao declarada para capturar na lambda");
                    }
                    if (c->has("ref") && lookupLocal(v)) capturesLocalByRef = true;
                    lctx.captures.insert(v);
                }
            }
        }
        if (inReturnExpr && capturesLocalByRef) {
            error(e, "GHOST-E0601", "nao pode retornar lambda que captura variavel local por referencia (a variavel deixa de existir ao sair da funcao)");
        }
        lambdaStack.push_back(lctx);
        scopes.emplace_back();
        FnCtx ctx{"a lambda", Ty{}, false, false, false};
        std::set<std::string> seenLambdaParams;
        for (const auto& k : e.kids) {
            if (k->kind == AstKind::Param) {
                if (!seenLambdaParams.insert(k->name).second) {
                    error(*k, "GHOST-E0601", "parametro '" + k->name + "' repetido na lambda");
                }
                if (k->kids[0]->text == "void") {
                    error(*k, "GHOST-E0201", "parametro da lambda nao pode ser void");
                }
                if (k->kids.size() > 1) {
                    error(*k, "GHOST-E0601", "parametros de lambda nao tem valor padrao");
                }
                declareLocal(*k, k->name, fromType(*k->kids[0]));
            }
            else if (k->kind == AstKind::Type && k->attr == "ret") {
                ctx.ret = fromType(*k);
                ctx.isVoid = k->text == "void";
                ctx.isResult = (k->text == "result");
                ctx.checkReturn = true;
            }
        }
        const AstNode& body = *e.kids.back();
        fns.push_back(ctx);
        const int savedLoop = loopDepth, savedSwitch = switchDepth;
        loopDepth = switchDepth = 0;
        if (body.kind == AstKind::Block) {
            for (const auto& s : body.kids) stmt(*s);
        } else if (body.kind == AstKind::Print) {
            stmt(body);
        } else {
            expr(body);
        }
        loopDepth = savedLoop;
        switchDepth = savedSwitch;
        fns.pop_back();
        scopes.pop_back();
        lambdaStack.pop_back();
        return named("fn");
    }

    Ty comprehension(const AstNode& e) {
        ++comprehensionDepth;
        scopes.emplace_back();
        for (size_t i = 1; i < e.kids.size(); ++i) {
            const AstNode& c = *e.kids[i];
            if (c.kind == AstKind::CompFor) {
                const Ty coll = expr(*c.kids[0]);
                if (!c.attr.empty()) {
                    Ty k, v;
                    if (coll.n == "map" && coll.a.size() == 2) { k = coll.a[0]; v = coll.a[1]; }
                    scopes.back()[c.name] = VarInfo{k, c.line};
                    scopes.back()[c.attr] = VarInfo{v, c.line};
                } else {
                    scopes.back()[c.name] = VarInfo{elementOf(coll), c.line};
                }
            } else if (c.kind == AstKind::CompIf) {
                expr(*c.kids[0]);
            }
        }
        const AstNode& head = *e.kids[0];
        Ty h;
        if (head.kind == AstKind::MapEntry) {
            expr(*head.kids[0]);
            expr(*head.kids[1]);
        } else {
            h = expr(head);
        }
        scopes.pop_back();
        --comprehensionDepth;
        if (e.attr == "list") return Ty{"list", {h}};
        if (e.attr == "set") return Ty{"set", {h}};
        return Ty{};
    }

    Ty expr(const AstNode& e) {
        switch (e.kind) {
            case AstKind::Number: return numberType(e.text);
            case AstKind::String: return named("string");
            case AstKind::FString:
                for (const auto& k : e.kids) expr(*k);
                return named("string");
            case AstKind::Ident: return ident(e);
            case AstKind::This:
                if (!curClass.empty() && !staticCtx && !classes[curClass].generic) return named(curClass);
                return Ty{};
            case AstKind::Super: return Ty{};
            case AstKind::Unary: {
                const Ty t = expr(*e.kids[0]);
                if (e.text == "!") return named("bool");
                if (cat(t) == CNum) return e.text == "~" ? named("int") : (t.n == "bool" ? named("int") : t);
                return e.text == "-" && t.n == "complex" ? t : Ty{};
            }
            case AstKind::Postfix: return expr(*e.kids[0]);
            case AstKind::Binary: return binary(e);
            case AstKind::Assign: return assign(e);
            case AstKind::Ternary: {
                condition(*e.kids[0], "operador ?:");
                const Ty a = expr(*e.kids[1]);
                const Ty b = expr(*e.kids[2]);
                if (known(a) && show(a) == show(b)) return a;
                return promote(a, b);
            }
            case AstKind::Call: return call(e);
            case AstKind::GenericRef: return Ty{};
            case AstKind::Member: return member(e);
            case AstKind::Index: {
                const Ty o = expr(*e.kids[0]);
                expr(*e.kids[1]);
                if (o.n == "list" && o.a.size() == 1) return o.a[0];
                if (o.n == "map" && o.a.size() == 2) return o.a[1];
                return Ty{};
            }
            case AstKind::Slice: {
                const Ty o = expr(*e.kids[0]);
                for (size_t i = 1; i < e.kids.size(); ++i) expr(*e.kids[i]);
                return (o.n == "list" || o.n == "string") ? o : Ty{};
            }
            case AstKind::ListLit: {
                Ty el;
                bool first = true, same = true;
                for (const auto& k : e.kids) {
                    const Ty t = expr(*k);
                    if (first) { el = t; first = false; }
                    else if (show(t) != show(el)) {
                        const Ty p = promote(el, t);
                        if (known(p)) el = p;
                        else same = false;
                    }
                }
                if (!same || first) el = Ty{};
                return Ty{e.attr == "set" ? "set" : "list", {el}};
            }
            case AstKind::MapLit:
                for (const auto& en : e.kids) {
                    if (en->kids[0]->kind != AstKind::Ident) expr(*en->kids[0]); // {nome: ...} em JSON: chave solta
                    expr(*en->kids[1]);
                }
                return Ty{};
            case AstKind::Lambda: return lambda(e);
            case AstKind::Comprehension: return comprehension(e);
            case AstKind::TryOp: {
                if (fns.empty() || !fns.back().isResult) {
                    error(e, "GHOST-E0701", "o operador ? so vale dentro de function result<T> (ex.: function result<double> f() { double v = g()?; ... })");
                } else if (finallyDepth > 0 || comprehensionDepth > 0) {
                    error(e, "GHOST-E0701", "o operador ? nao vale dentro de finally nem de compreensao de lista");
                } else if (spawnDepth > 0) {
                    error(e, "GHOST-E0701", "o operador ? nao vale dentro de spawn");
                }
                const Ty t = expr(*e.kids[0]);
                return t.n == "result" && t.a.size() == 1 ? t.a[0] : Ty{};
            }
            case AstKind::New: return expr(*e.kids[0]);
            case AstKind::TypeExpr: return fromType(*e.kids[0]);
            case AstKind::Cast: expr(*e.kids[0]); return named(e.text == "float" ? "double" : e.text);
            default:
                for (const auto& k : e.kids) expr(*k);
                return Ty{};
        }
    }
};

} // namespace

std::vector<AstDiagnostic> ghost_typecheck(const AstNode& program) {
    Checker c;
    c.run(program);
    return c.errors;
}
