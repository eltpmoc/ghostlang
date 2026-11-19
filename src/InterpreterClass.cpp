// InterpreterClass.cpp - classes no REPL da GhostLang.
//
// Mesma semântica do programa compilado (C++17):
//   - objetos têm semântica de valor: b = a copia; new X(...) == X(...);
//   - construtor = function com o nome da classe; super(args) na 1ª linha chama o construtor da base;
//     sem super(...), a base é construída sem argumentos. Ordem: base, campos da classe, corpo;
//   - campos e métodos são públicos até o primeiro private: (public: volta a liberar);
//     private só é acessível dentro de métodos da própria classe (nem as filhas acessam);
//   - métodos são virtuais: a versão usada é a da classe real do objeto; super.m(...) chama a da base;
//   - dentro de métodos, nomes sem objeto procuram: variável local/parâmetro, campo, variável global;
//     metodo(...) sem objeto chama o método do próprio objeto antes de funções globais;
//   - guardar um objeto derivado em variável, parâmetro, retorno ou campo do tipo base mantém só a
//     parte da base (fatiamento, como no C++).
#include "../include/Interpreter.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <stdexcept>

namespace {

bool isFieldType(GTokenType t) {
    return t == GTokenType::KwInt || t == GTokenType::KwDouble || t == GTokenType::KwFloat || t == GTokenType::KwBool ||
           t == GTokenType::KwString || t == GTokenType::KwComplex || t == GTokenType::KwVar || t == GTokenType::KwList || t == GTokenType::KwMap;
}
} // namespace

bool GhostInterpreter::inMethod() const {
    return !this_stack.empty() && this_stack.back().frameDepth == frames.size();
}

const GhostInterpreter::GhostFieldDef* GhostInterpreter::findField(const std::string& cls, const std::string& name, std::string* declaredIn) const {
    std::string c = cls;
    int guard = 0;
    while (!c.empty() && guard++ < 256) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return nullptr;
        for (const auto& f : it->second.fields) {
            if (f.name == name) {
                if (declaredIn) *declaredIn = c;
                return &f;
            }
        }
        c = it->second.base;
    }
    return nullptr;
}

const GhostFunction* GhostInterpreter::findMethod(const std::string& cls, const std::string& name, std::string* implClass, bool* priv, bool* prot) const {
    std::string c = cls;
    int guard = 0;
    while (!c.empty() && guard++ < 256) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return nullptr;
        auto m = it->second.methods.find(name);
        if (m != it->second.methods.end()) {
            if (implClass) *implClass = c;
            if (priv) {
                auto p = it->second.privateMethod.find(name);
                *priv = p != it->second.privateMethod.end() && p->second;
            }
            if (prot) {
                auto pr = it->second.protectedMethod.find(name);
                *prot = pr != it->second.protectedMethod.end() && pr->second;
            }
            return &m->second;
        }
        c = it->second.base;
    }
    return nullptr;
}

std::vector<GhostFunction> GhostInterpreter::findMethodOverloads(const std::string& cls, const std::string& name, std::string* implClass, bool* priv, bool* prot) const {
    std::vector<GhostFunction> res;
    std::string c = cls;
    int guard = 0;
    while (!c.empty() && guard++ < 256) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return res;
        auto mo = it->second.method_overloads.find(name);
        if (mo != it->second.method_overloads.end() && !mo->second.empty()) {
            if (implClass && res.empty()) *implClass = c;
            if (priv && res.empty()) {
                auto p = it->second.privateMethod.find(name);
                *priv = p != it->second.privateMethod.end() && p->second;
            }
            if (prot && res.empty()) {
                auto pr = it->second.protectedMethod.find(name);
                *prot = pr != it->second.protectedMethod.end() && pr->second;
            }
            for (const auto& f : mo->second) res.push_back(f);
        } else {
            auto m = it->second.methods.find(name);
            if (m != it->second.methods.end()) {
                if (implClass && res.empty()) *implClass = c;
                if (priv && res.empty()) {
                    auto p = it->second.privateMethod.find(name);
                    *priv = p != it->second.privateMethod.end() && p->second;
                }
                if (prot && res.empty()) {
                    auto pr = it->second.protectedMethod.find(name);
                    *prot = pr != it->second.protectedMethod.end() && pr->second;
                }
                res.push_back(m->second);
            }
        }
        if (!res.empty()) return res;
        c = it->second.base;
    }
    return res;
}

bool GhostInterpreter::isSubclass(const std::string& cls, const std::string& base) const {
    std::string c = cls;
    int guard = 0;
    while (!c.empty() && guard++ < 256) {
        if (c == base) return true;
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return false;
        c = it->second.base;
    }
    return false;
}

void GhostInterpreter::checkAccess(const std::string& declaredIn, bool priv, bool prot, const std::string& what) {
    if (!priv && !prot) return;
    const std::string ctx = currentClassCtx();
    if (priv) {
        if (ctx != declaredIn) throw std::runtime_error(what + " e private em " + declaredIn + " (so os metodos de " + declaredIn + " acessam)");
    } else if (prot) {
        if (ctx.empty() || (ctx != declaredIn && !isSubclass(ctx, declaredIn))) {
            throw std::runtime_error(what + " e protected em " + declaredIn + " (so " + declaredIn + " e subclasses acessam)");
        }
    }
}

// Campo do objeto do método em execução, se o nome não for uma variável local/parâmetro.
GhostValue* GhostInterpreter::fieldRef(const std::string& name, const GhostFieldDef** info) {
    static const GhostFieldDef untyped;
    if (!inMethod()) return nullptr;
    if (!frames.empty() && frames.back().count(name)) return nullptr; // local/parâmetro esconde o campo
    auto& obj = this_stack.back().obj;
    auto it = obj.map.find(name);
    if (it == obj.map.end()) return nullptr;
    std::string decl;
    const GhostFieldDef* fd = findField(obj.str, name, &decl);
    if (fd) checkAccess(decl, fd->priv, fd->prot, "campo " + name);
    if (info) *info = fd ? fd : &untyped;
    return &it->second;
}

GhostValue* GhostInterpreter::rootRef(const std::string& name) {
    if (name == "this") return inMethod() ? &this_stack.back().obj : nullptr;
    if (name.find("::") != std::string::npos) { // Classe::campo_static (Registro.log.push(...))
        auto s = static_memory.find(name);
        return s != static_memory.end() ? &s->second : nullptr;
    }
    if (GhostValue* f = fieldRef(name)) return f;
    if (GhostValue* s = staticRef(name)) return s;
    auto it = coll_memory.find(name);
    return it != coll_memory.end() ? &it->second : nullptr;
}

GhostValue* GhostInterpreter::resolvePath(const LPath& p, bool createKey, bool quiet, const GhostFieldDef** lastField, std::string* lastListElemSpec) {
    auto fail = [&](const std::string& msg) -> GhostValue* {
        if (quiet) return nullptr;
        throw std::runtime_error(msg);
    };
    if (lastField) *lastField = nullptr;
    if (lastListElemSpec) *lastListElemSpec = "";
    GhostValue* cur = nullptr;
    try {
        cur = rootRef(p.root);
    } catch (...) {
        if (quiet) return nullptr;
        throw;
    }
    if (!cur) return fail("Variavel nao encontrada: " + p.root);
    for (size_t s = 0; s < p.steps.size(); ++s) {
        const LStep& st = p.steps[s];
        const bool last = s + 1 == p.steps.size();
        if (st.field) {
            const std::string& name = st.key.str;
            if (cur->kind != GhostValue::Object) return fail("'." + name + "': o valor nao e um objeto");
            std::string decl;
            const GhostFieldDef* fd = findField(cur->str, name, &decl);
            auto it = cur->map.find(name);
            if (!fd || it == cur->map.end()) return fail("a classe " + cur->str + " nao tem o campo " + name);
            try {
                checkAccess(decl, fd->priv, fd->prot, "campo " + name); // escrita: protected tambem
            } catch (...) {
                if (quiet) return nullptr;
                throw;
            }
            if (last && lastField) *lastField = fd;
            cur = &it->second;
        } else if (cur->kind == GhostValue::List) {
            if (st.key.kind != GhostValue::Num && st.key.kind != GhostValue::Bool) return fail("indice de lista precisa ser numero");
            long long i = static_cast<long long>(st.key.num);
            const long long orig = i, n = static_cast<long long>(cur->list.size());
            if (i < 0) i += n;
            if (i < 0 || i >= n) return fail("indice " + std::to_string(orig) + " fora da lista de " + std::to_string(n) + " elemento(s)");
            if (last && lastListElemSpec) *lastListElemSpec = cur->str;
            cur = &cur->list[static_cast<size_t>(i)];
        } else if (cur->kind == GhostValue::Map) {
            const std::string key = toText(st.key, false);
            auto it = cur->map.find(key);
            if (it == cur->map.end()) {
                if (!(createKey && last)) return fail("chave \"" + key + "\" nao existe no mapa");
                cur = &cur->map[key];
            } else {
                cur = &it->second;
            }
        } else {
            return fail(cur->kind == GhostValue::Str ? "textos nao podem ser alterados por indice" : "'[ ]' so vale em lista ou mapa");
        }
    }
    return cur;
}

GhostValue GhostInterpreter::readField(const GhostValue& obj, const std::string& field) {
    std::string decl;
    const GhostFieldDef* fd = findField(obj.str, field, &decl);
    auto it = obj.map.find(field);
    if (!fd || it == obj.map.end()) {
        std::string owner;
        if (const GhostFieldDef* sfd = findStatic(obj.str, field, &owner)) { // obj.campo_static (vale como Classe.campo)
            checkAccess(owner, sfd->priv, sfd->prot, "campo static " + field);
            return static_memory[owner + "::" + field];
        }
        if (findMethod(obj.str, field, nullptr)) throw std::runtime_error(obj.str + "." + field + " e metodo: use " + field + "(...)");
        throw std::runtime_error("a classe " + obj.str + " nao tem o campo " + field);
    }
    checkAccess(decl, fd->priv, fd->prot, "campo " + field);
    return it->second;
}

// Converte para o tipo declarado (int trunca, bool vira true/false...). Objetos: confere a classe e fatia.
GhostValue GhostInterpreter::coerce(const GhostValue& v, GTokenType type, const std::string& cls, const std::string& what) {
    if (!cls.empty()) ensureTypeReady(cls);
    if (isResultSpec(cls)) return coerceResult(v, cls, what); // src/InterpreterResult.cpp
    if (isSetSpec(cls)) return coerceSet(v, cls, what);
    if (isFnSpec(cls)) return coerceFn(v, cls, what);
    if (isListSpec(cls)) return coerceList(v, cls, what);
    if (interface_memory.count(cls)) return coerceIface(v, cls, what);
    if (!cls.empty()) {
        if (v.kind != GhostValue::Object) throw std::runtime_error(what + ": esperado objeto " + cls + ", recebido " + toText(v, true));
        if (v.str == cls) return v;
        if (!isSubclass(v.str, cls)) throw std::runtime_error(what + ": " + v.str + " nao e " + cls);
        GhostValue r;
        r.kind = GhostValue::Object;
        r.str = cls;
        std::string c = cls;
        int guard = 0;
        while (!c.empty() && guard++ < 256) {
            const GhostClassDef& d = class_memory.at(c);
            for (const auto& f : d.fields) {
                auto it = v.map.find(f.name);
                if (it != v.map.end()) r.map[f.name] = it->second;
            }
            c = d.base;
        }
        return r;
    }
    switch (type) {
        case GTokenType::KwInt: return GhostValue::number(std::trunc(toNum(v, what)), true);
        case GTokenType::KwDouble: case GTokenType::KwFloat: return GhostValue::number(toNum(v, what));
        case GTokenType::KwBool: return GhostValue::boolean(truthy(v));
        case GTokenType::KwString: return GhostValue::text(toText(v, false));
        case GTokenType::KwComplex:
            if (v.kind == GhostValue::Complex) return v;
            if (v.kind == GhostValue::Num) return GhostValue::cplx(v.num, 0.0);
            throw std::runtime_error(what + ": esperado complex, recebido " + toText(v, true));
        case GTokenType::KwList:
            if (v.kind != GhostValue::List) throw std::runtime_error(what + ": esperado lista, recebido " + toText(v, true));
            return v;
        case GTokenType::KwMap:
            if (v.kind != GhostValue::Map) throw std::runtime_error(what + ": esperado mapa, recebido " + toText(v, true));
            return v;
        case GTokenType::KwQreg: case GTokenType::KwQcircuit: {
            const bool reg = type == GTokenType::KwQreg;
            if (v.kind != GhostValue::Quantum || (reg ? !v.qreg : !v.qcirc))
                throw std::runtime_error(what + ": esperado " + std::string(reg ? "qreg (use qreg(n))" : "qcircuit (use qcircuit(n))") + ", recebido " + toText(v, true));
            return v;
        }
        default: return v;
    }
}

// ---------------------------------------------------------------- criação de objetos

GhostValue GhostInterpreter::construct(const std::string& cls, std::vector<GhostValue> args) {
    if (!class_memory.count(cls)) throw std::runtime_error("classe nao declarada: " + cls);
    if (construct_depth >= 200) throw std::runtime_error(cls + ": objetos aninhados demais ao construir");
    struct DepthGuard {
        int& d;
        explicit DepthGuard(int& x) : d(x) { ++d; }
        ~DepthGuard() { --d; }
    } guard(construct_depth);
    RetGuard retGuard(ret_stack, ""); // construtor e valores iniciais: r? não vale
    GhostValue obj;
    obj.kind = GhostValue::Object;
    obj.str = cls;
    initObject(obj, cls, std::move(args));
    obj.str = cls;
    return obj;
}

// Valores iniciais dos campos da classe cls (as bases já foram feitas). this = obj durante as expressões.
void GhostInterpreter::initFields(GhostValue& obj, const std::string& cls) {
    const std::vector<GhostFieldDef> fields = class_memory.at(cls).fields;
    const size_t idx = this_stack.size();
    this_stack.push_back({obj, cls, frames.size()});
    try {
        for (const auto& f : fields) {
            GhostValue v;
            if (f.type == GTokenType::KwQreg || f.type == GTokenType::KwQcircuit) { // qreg q(n);
                std::vector<Token> init = f.init;
                v = makeQuantum(f.type, toNum(evalValue(init), cls + "." + f.name + " (qubits)"));
            } else if (!f.init.empty()) {
                std::vector<Token> init = f.init;
                v = coerce(evalValue(init), f.type, f.cls, cls + "." + f.name);
            } else if (!f.cls.empty()) {
                v = defaultOf(f.cls); // objeto padrão ou result<T> sem valor
            } else {
                switch (f.type) {
                    case GTokenType::KwInt: v = GhostValue::number(0, true); break;
                    case GTokenType::KwDouble: case GTokenType::KwFloat: v = GhostValue::number(0); break;
                    case GTokenType::KwBool: v = GhostValue::boolean(false); break;
                    case GTokenType::KwString: v = GhostValue::text(""); break;
                    case GTokenType::KwComplex: v = GhostValue::cplx(0, 0); break;
                    case GTokenType::KwList: v.kind = GhostValue::List; break;
                    case GTokenType::KwMap: v.kind = GhostValue::Map; break;
                    default: throw std::runtime_error(cls + "." + f.name + ": campo var precisa de valor inicial");
                }
            }
            this_stack[idx].obj.map[f.name] = v;
        }
    } catch (...) {
        obj = std::move(this_stack[idx].obj);
        this_stack.pop_back();
        throw;
    }
    obj = std::move(this_stack[idx].obj);
    this_stack.pop_back();
}

void GhostInterpreter::initObject(GhostValue& obj, const std::string& cls, std::vector<GhostValue> args) {
    const GhostClassDef def = class_memory.at(cls); // cópia: o corpo pode redefinir classes
    if (!def.hasCtor) {
        if (!args.empty()) throw std::runtime_error(cls + ": a classe nao tem construtor e recebeu " + std::to_string(args.size()) + " argumento(s)");
        if (!def.base.empty()) initObject(obj, def.base, {});
        obj.str = cls;
        initFields(obj, cls);
        return;
    }
    const GhostFunction& f = def.ctor;
    if (def.ctorPrivate) checkAccess(cls, true, "construtor de " + cls);
    if (args.size() > f.parameters.size())
        throw std::runtime_error(cls + "(): recebeu " + std::to_string(args.size()) + " argumento(s), espera ate " + std::to_string(f.parameters.size()));
    for (size_t k = args.size(); k < f.parameters.size(); ++k) {
        if (k < f.defaults.size() && !f.defaults[k].empty()) {
            auto d = f.defaults[k];
            args.push_back(evalValue(d));
        } else {
            throw std::runtime_error(cls + "(): faltou o argumento '" + f.parameters[k].second + "'");
        }
    }
    if (call_depth >= 400) throw std::runtime_error(cls + "(): recursao profunda demais (limite 400)");

    // Quadro do construtor: os parâmetros valem em super(...), nos campos e no corpo.
    pushFrame();
    ++call_depth;
    bool pushedThis = false;
    auto leave = [&]() {
        if (pushedThis) {
            obj = std::move(this_stack.back().obj);
            this_stack.pop_back();
            pushedThis = false;
        }
        popFrameAndRestore();
        --call_depth;
    };
    try {
        for (size_t k = 0; k < f.parameters.size(); ++k) {
            const std::string& pn = f.parameters[k].second;
            const std::string pc = k < f.paramClasses.size() && knownType(f.paramClasses[k]) ? f.paramClasses[k] : "";
            GhostValue pv = coerce(args[k], f.parameters[k].first, pc, pn);
            declareLocal(pn);
            storeVar(pn, pv, pc.empty() ? f.parameters[k].first : GTokenType::KwVar);
            if (!pc.empty()) var_class[pn] = pc;
        }
        if (!def.base.empty()) {
            std::vector<GhostValue> sv;
            if (def.hasSuperCall) {
                for (auto r : def.superArgs) sv.push_back(evalValue(r));
            }
            initObject(obj, def.base, sv);
        } else if (def.hasSuperCall) {
            throw std::runtime_error(cls + ": super(...) sem classe base (use class " + cls + " extends Base)");
        }
        obj.str = cls;
        initFields(obj, cls);
        this_stack.push_back({obj, cls, frames.size()});
        pushedThis = true;
        executeLine(f.body);
    } catch (GhostReturnSignal&) {
        // return; no construtor só encerra o corpo
    } catch (...) {
        leave();
        throw;
    }
    leave();
}

// ---------------------------------------------------------------- métodos

GhostValue GhostInterpreter::callMethod(GhostValue& self, const std::string& startClass, const std::string& m, std::vector<std::vector<Token>>& raw, bool isVirtual) {
    if (self.kind != GhostValue::Object) throw std::runtime_error("." + m + "(): o valor nao e um objeto");
    const std::string lookup = isVirtual ? self.str : startClass;
    std::string impl;
    bool priv = false, prot = false;
    auto ovs = findMethodOverloads(lookup, m, &impl, &priv, &prot);
    if (ovs.empty()) {
        std::string owner;
        bool sp = false, sprot = false;
        auto sovs = findStaticMethodOverloads(lookup, m, &owner, &sp, &sprot);
        if (!sovs.empty()) {
            checkAccess(owner, sp, sprot, "metodo static " + m + "()");
            if (sovs.size() == 1) {
                return callStatic(owner, sovs[0], m, raw);
            }
            std::vector<GhostValue> preVals;
            for (auto& r : raw) preVals.push_back(evalValue(r));
            const GhostFunction* chosen = selectOverload(sovs, preVals, owner + "." + m);
            const GhostFunction copy = *chosen;
            std::vector<GhostValue> vals = preVals;
            for (size_t k = vals.size(); k < copy.parameters.size(); ++k) {
                if (k < copy.defaults.size() && !copy.defaults[k].empty()) {
                    auto d = copy.defaults[k];
                    vals.push_back(evalValue(d));
                } else throw std::runtime_error(owner + "." + m + "(): faltou o argumento '" + copy.parameters[k].second + "'");
            }
            return runFunction(copy, owner + "." + m, vals, nullptr, owner);
        }
        if (findField(self.str, m)) throw std::runtime_error(self.str + "." + m + " e campo, nao metodo");
        throw std::runtime_error("a classe " + lookup + " nao tem o metodo " + m + "()");
    }
    checkAccess(impl, priv, prot, "metodo " + m + "()");
    if (ovs.size() == 1) {
        const GhostFunction f = ovs[0];
        const std::string display = impl + "." + m;
        std::vector<GhostValue> vals = evalArgs(f, display, raw);
        return runFunction(f, display, vals, &self, impl);
    }
    std::vector<GhostValue> preVals;
    for (auto& r : raw) preVals.push_back(evalValue(r));
    const GhostFunction* chosen = selectOverload(ovs, preVals, impl + "." + m);
    const GhostFunction f = *chosen;
    const std::string display = impl + "." + m;
    std::vector<GhostValue> vals = preVals;
    for (size_t k = vals.size(); k < f.parameters.size(); ++k) {
        if (k < f.defaults.size() && !f.defaults[k].empty()) {
            auto d = f.defaults[k];
            vals.push_back(evalValue(d));
        } else throw std::runtime_error(display + "(): faltou o argumento '" + f.parameters[k].second + "'");
    }
    return runFunction(f, display, vals, &self, impl);
}

// Chama o método e grava o objeto alterado de volta no lugar de onde ele veio (variável, campo, elemento, this).
GhostValue GhostInterpreter::callOnPath(const LPath& path, GhostValue self, const std::string& startClass, const std::string& m, std::vector<std::vector<Token>>& raw, bool isVirtual) {
    LPath effPath = path;
    if (effPath.root == "this" && inMethod()) {
        const LPath& parentPath = this_stack.back().path;
        if (!parentPath.root.empty()) {
            effPath = parentPath;
            for (const auto& st : path.steps) effPath.steps.push_back(st);
        }
    }
    LPath prevPending = std::move(pending_method_path);
    pending_method_path = effPath;
    GhostValue r;
    try {
        r = callMethod(self, startClass, m, raw, isVirtual);
    } catch (...) {
        pending_method_path = std::move(prevPending);
        throw;
    }
    pending_method_path = std::move(prevPending);
    if (GhostValue* p = resolvePath(path, false, true)) *p = self;
    if (effPath.root != path.root) if (GhostValue* ep = resolvePath(effPath, false, true)) *ep = self;
    return r;
}

// Animal a = new Animal("Rex");   Contador c;   (c recebe Contador())
bool GhostInterpreter::tryClassVarDecl(const std::string& cls, const Token& nameTok, Lexer& lex) {
    const std::string n = nameTok.value;
    Token nx = lex.getNextToken();
    GhostValue v;
    if (nx.type == GTokenType::Semicolon || nx.type == GTokenType::EndOfFile) {
        v = defaultOf(cls);
    } else if (nx.type == GTokenType::Assign) {
        std::vector<Token> rhs;
        int depth = 0;
        while (true) {
            Token t = lex.peekNextToken();
            if (t.type == GTokenType::EndOfFile) break;
            if (depth == 0 && t.type == GTokenType::Semicolon) { lex.getNextToken(); break; }
            if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
            if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) depth--;
            rhs.push_back(lex.getNextToken());
        }
        if (rhs.empty()) throw std::runtime_error(cls + " " + n + " = : faltou o valor");
        v = evalValue(rhs);
    } else {
        throw std::runtime_error("depois de " + cls + " " + n +
                                 (isResultSpec(cls) ? " use ';' ou '= ok(...)'"
                                                    : (isFnSpec(cls) ? " use ';' ou '= (...) => ...'" : " use ';' ou '= new " + cls + "(...)'")));
    }
    v = coerce(v, GTokenType::KwVar, cls, n);
    declareLocal(n);
    storeVar(n, v, GTokenType::KwVar);
    var_class[n] = cls;
    return true;
}

// class Nome [extends Base] { campos; function ...; public:/private: }
void GhostInterpreter::parseClassDef(Lexer& lex) {
    Token nameTok = lex.getNextToken();
    if (nameTok.type != GTokenType::Identifier) throw std::runtime_error("Esperado o nome da classe depois de class.");
    GhostClassDef def;
    def.name = nameTok.value;
    if (functions_memory.count(def.name)) throw std::runtime_error("class " + def.name + ": ja existe uma funcao com esse nome");
    Token t = lex.getNextToken();
    std::vector<std::string> params; // class Pilha<T> / class Par<A, B> / class Ranking<T implements Medivel>
    std::vector<GhostConstraint> constraints;
    if (t.type == GTokenType::Lt) {
        std::vector<Token> inner;
        int depth = 1;
        while (true) {
            Token k = lex.getNextToken();
            if (k.type == GTokenType::EndOfFile || k.type == GTokenType::LBrace) throw std::runtime_error("class " + def.name + "<...>: faltou '>'");
            if (k.type == GTokenType::Lt) depth++;
            else if (k.type == GTokenType::Gt) depth--;
            else if (k.type == GTokenType::Chain) {
                depth -= 2;
                if (depth == 0) { k.type = GTokenType::Gt; k.value = ">"; inner.push_back(k); break; } // ...Pilha<int>>
            }
            if (depth <= 0) break;
            inner.push_back(k);
        }
        parseTypeParamTokens(inner, "class " + def.name, params, constraints);
        t = lex.getNextToken();
    }
    if (params.empty() && generic_memory.count(def.name)) throw std::runtime_error("ja existe uma classe generica chamada " + def.name);
    if (!params.empty() && class_memory.count(def.name)) throw std::runtime_error("ja existe uma classe chamada " + def.name);
    if (t.type == GTokenType::KwExtends) {
        Token b = lex.getNextToken();
        if (b.type != GTokenType::Identifier) throw std::runtime_error("Esperado o nome da classe base depois de extends.");
        std::string baseSpec = b.value;
        if (generic_memory.count(b.value)) { // extends Pilha<int>
            if (lex.peekNextToken().type != GTokenType::Lt) throw std::runtime_error(b.value + " e uma classe generica: use extends " + b.value + "<tipo>");
            baseSpec = readResultSpec(lex, false, b.value);
            for (const auto& p : params) { // class B<T> extends Pilha<T>: não (igual ao compilador)
                Lexer bx(baseSpec);
                for (Token k = bx.getNextToken(); k.type != GTokenType::EndOfFile; k = bx.getNextToken())
                    if (k.type == GTokenType::Identifier && k.value == p)
                        throw std::runtime_error("class " + def.name + ": a classe base nao pode depender de " + p + " (use um tipo fixo, ex.: extends " + b.value + "<int>)");
            }
            ensureInstance(baseSpec);
        } else if (!class_memory.count(b.value)) {
            throw std::runtime_error("a classe base '" + b.value + "' precisa ser declarada antes de " + def.name + ".");
        }
        if (baseSpec == def.name || isSubclass(baseSpec, def.name)) throw std::runtime_error("class " + def.name + ": heranca circular");
        def.base = baseSpec;
        t = lex.getNextToken();
    }
    if (interface_memory.count(def.name) || generic_iface_memory.count(def.name))
        throw std::runtime_error("ja existe uma interface chamada " + def.name);
    if (t.type == GTokenType::Identifier && t.value == "implements") { // class X implements A, B, Colecao<int>, Somavel<T>
        while (true) {
            Token n = lex.getNextToken();
            if (n.type != GTokenType::Identifier)
                throw std::runtime_error("esperado nome da interface apos implements em " + def.name);
            std::string ifaceSpec = n.value;
            if (lex.peekNextToken().type == GTokenType::Lt) {
                ifaceSpec = readResultSpec(lex, false, n.value);
                if (params.empty()) ensureIfaceInstance(ifaceSpec);
            } else if (generic_iface_memory.count(n.value)) {
                throw std::runtime_error(n.value + " e uma interface generica: use " + n.value + "<tipo>");
            } else if (!interface_memory.count(n.value)) {
                throw std::runtime_error("'" + n.value + "' nao e uma interface declarada (implements de " + def.name + ")");
            }
            def.interfaces.push_back(ifaceSpec);
            t = lex.getNextToken();
            if (t.type != GTokenType::Comma) break;
        }
    }
    if (t.type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' depois de class " + def.name + ".");
    const std::string body = lex.extractBlock();
    if (!params.empty()) { // modelo: cada Pilha<int> usado depois vira uma classe (ensureInstance)
        GhostGenericDef g;
        g.name = def.name;
        g.base = def.base;
        g.params = params;
        g.constraints = constraints;
        g.interfaces = def.interfaces;
        g.body = body;
        for (auto it = class_memory.begin(); it != class_memory.end();) // redefinição: esquece as instâncias antigas
            it = it->first.rfind(def.name + "<", 0) == 0 ? class_memory.erase(it) : std::next(it);
        generic_memory[def.name] = g;
        return;
    }
    defineClass(def, body, def.name);
}

// ---------------------------------------------------------------- classes genéricas

namespace {
// Troca os nomes de tipo (T) pelos tipos concretos no código do corpo, fora de textos e comentários.
// Nomes depois de '.' (obj.T) não são tipos e ficam como estão.
std::string substituteTypes(const std::string& src, const std::vector<std::string>& params, const std::vector<std::string>& args) {
    std::string out;
    size_t i = 0;
    char prevSignificant = 0;
    while (i < src.size()) {
        const char c = src[i];
        if (c == '"' || c == '`' || c == '\'') { // texto (inclusive f-strings): copia sem trocar
            const char q = c;
            out += src[i++];
            while (i < src.size() && src[i] != q) { if (src[i] == '\\' && i + 1 < src.size()) out += src[i++]; out += src[i++]; }
            if (i < src.size()) out += src[i++];
            prevSignificant = q;
            continue;
        }
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '/') { while (i < src.size() && src[i] != '\n') out += src[i++]; continue; }
        if (c == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            const size_t e = src.find("*/", i + 2);
            const size_t end = e == std::string::npos ? src.size() : e + 2;
            out += src.substr(i, end - i);
            i = end;
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t j = i;
            while (j < src.size() && (std::isalnum(static_cast<unsigned char>(src[j])) || src[j] == '_')) ++j;
            const std::string word = src.substr(i, j - i);
            auto it = std::find(params.begin(), params.end(), word);
            out += (it != params.end() && prevSignificant != '.') ? args[static_cast<size_t>(it - params.begin())] : word;
            prevSignificant = 'a';
            i = j;
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) prevSignificant = c;
        out += src[i++];
    }
    return out;
}
} // namespace

std::string GhostInterpreter::specFromTokens(const std::vector<Token>& t, size_t& i, const std::string& head) {
    std::string s = head + "<";
    int depth = 0;
    for (; i < t.size(); ++i) {
        const auto ty = t[i].type;
        if (ty == GTokenType::Lt) depth++;
        else if (ty == GTokenType::Gt) depth--;
        else if (ty == GTokenType::Chain) depth -= 2;
        if (depth == 1 && ty == GTokenType::Lt && s.size() == head.size() + 1) continue; // o primeiro '<'
        if (ty == GTokenType::Comma) s += ",";
        else s += t[i].value;
        if (depth <= 0) { ++i; break; }
    }
    if (depth > 0) throw std::runtime_error(head + "<...>: faltou '>'");
    return s;
}

// ---------------------------------------------------------------- funções genéricas

// Depois de 'function': se o cabeçalho for "tipo nome<T, U>(...)", guarda o modelo e devolve true.
bool GhostInterpreter::tryGenericFunctionDef(Lexer& lex) {
    GhostGenericFn g;
    if (!readGenericFn(lex, g)) return false;
    const std::string name = g.name;
    if (name == "main") throw std::runtime_error("main nao pode ser generica");
    if (class_memory.count(name) || generic_memory.count(name)) throw std::runtime_error("function " + name + ": ja existe uma classe com esse nome");
    for (auto it = functions_memory.begin(); it != functions_memory.end();) // redefinição: esquece as instâncias antigas
        it = it->first.rfind(name + "<", 0) == 0 ? functions_memory.erase(it) : std::next(it);
    functions_memory.erase(name);
    generic_functions[name] = g;
    return true;
}

bool GhostInterpreter::readGenericFn(Lexer& lex, GhostGenericFn& g) {
    const size_t start = lex.getPosition();
    std::vector<Token> head;
    int angle = 0;
    while (true) { // cabeçalho até o '(' fora de < >
        Token k = lex.getNextToken();
        if (k.type == GTokenType::EndOfFile || k.type == GTokenType::LBrace || k.type == GTokenType::Semicolon) { lex.setPosition(start); return false; }
        if (k.type == GTokenType::LParen && angle <= 0) break;
        if (k.type == GTokenType::Lt) angle++;
        else if (k.type == GTokenType::Gt) angle--;
        else if (k.type == GTokenType::Chain) { // '>>' vira dois '>'
            angle -= 2;
            k.type = GTokenType::Gt;
            k.value = ">";
            head.push_back(k);
        }
        head.push_back(k);
    }
    // ... nome < T extends Animal , U >   (o nome vem logo antes do '<' que fecha no fim)
    if (head.size() < 4 || head.back().type != GTokenType::Gt) { lex.setPosition(start); return false; }
    size_t lt = 0;
    {
        int d = 0;
        for (size_t k = head.size(); k-- > 0;) {
            if (head[k].type == GTokenType::Gt) d++;
            else if (head[k].type == GTokenType::Lt && --d == 0) { lt = k; break; }
        }
    }
    if (lt == 0 || head[lt - 1].type != GTokenType::Identifier || head[lt + 1].type != GTokenType::Identifier) { lex.setPosition(start); return false; }
    const std::string name = head[lt - 1].value;
    std::vector<std::string> params;
    std::vector<GhostConstraint> constraints;
    parseTypeParamTokens(std::vector<Token>(head.begin() + static_cast<long>(lt) + 1, head.end() - 1), "function " + name, params, constraints);
    // parâmetros (até o ')') e corpo
    std::vector<Token> ptoks;
    int depth = 1;
    while (true) {
        Token k = lex.getNextToken();
        if (k.type == GTokenType::EndOfFile) throw std::runtime_error("Faltou ')' nos parametros de " + name);
        if (k.type == GTokenType::LParen) depth++;
        if (k.type == GTokenType::RParen && --depth == 0) break;
        ptoks.push_back(k);
    }
    if (lex.getNextToken().type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' apos declaracao de parametros.");
    const std::string body = lex.extractBlock();
    g = GhostGenericFn();
    g.name = name;
    g.params = params;
    g.constraints = constraints;
    std::vector<Token> ret(head.begin(), head.begin() + static_cast<long>(lt - 1));
    g.source = tokensToSource(ret) + name + " ( " + tokensToSource(ptoks) + ") {" + body + "}";
    // tipo de cada parâmetro (texto sem espaços): "T", "list<T>", "Pilha<T>"; T[] vira list<T>
    std::vector<Token> cur;
    int d = 0;
    auto flush = [&]() {
        if (cur.empty()) return;
        size_t end = cur.size();
        for (size_t k = 0; k < cur.size(); ++k) if (cur[k].type == GTokenType::Assign) { end = k; break; }
        bool arr = end >= 3 && cur[end - 2].type == GTokenType::LBracket && cur[end - 1].type == GTokenType::RBracket; // T xs[]
        const size_t nameIdx = arr ? end - 3 : end - 1;
        std::string ty;
        for (size_t k = 0; k < nameIdx; ++k) ty += cur[k].type == GTokenType::Comma ? std::string(",") : cur[k].value;
        if (ty.size() > 2 && ty.compare(ty.size() - 2, 2, "[]") == 0) { ty.resize(ty.size() - 2); arr = true; } // T[] xs
        if (arr) ty = "list<" + ty + ">";
        g.paramTypes.push_back(ty);
        cur.clear();
    };
    for (const auto& k : ptoks) {
        if (k.type == GTokenType::LParen || k.type == GTokenType::LBracket || k.type == GTokenType::Lt) d++;
        else if (k.type == GTokenType::RParen || k.type == GTokenType::RBracket || k.type == GTokenType::Gt) d--;
        else if (k.type == GTokenType::Chain) d -= 2;
        if (k.type == GTokenType::Comma && d == 0) { flush(); continue; }
        cur.push_back(k);
    }
    flush();
    return true;
}

// <T, U extends Animal, V implements A & B, N extends number>: nomes e restrições (tokens sem os '<' '>' de fora)
void GhostInterpreter::parseTypeParamTokens(const std::vector<Token>& inner, const std::string& what,
                                            std::vector<std::string>& names, std::vector<GhostConstraint>& cons) {
    std::vector<std::vector<Token>> pieces(1);
    int depth = 0;
    for (const auto& k : inner) {
        if (k.type == GTokenType::Lt) depth++;
        else if (k.type == GTokenType::Gt) depth--;
        else if (k.type == GTokenType::Chain) depth -= 2;
        if (k.type == GTokenType::Comma && depth == 0) { pieces.emplace_back(); continue; }
        pieces.back().push_back(k);
    }
    for (const auto& pc : pieces) {
        if (!pc.empty() && pc[0].type == GTokenType::Identifier) names.push_back(pc[0].value);
    }
    const auto declaredParamNames = names;
    names.clear();
    for (const auto& pc : pieces) {
        if (pc.empty() || pc[0].type != GTokenType::Identifier) throw std::runtime_error(what + "<...>: use nomes de tipo, ex.: <T> ou <T extends Classe>");
        const std::string p = pc[0].value;
        if (std::find(names.begin(), names.end(), p) != names.end()) throw std::runtime_error(what + ": tipo " + p + " repetido entre < >");
        if (class_memory.count(p) || generic_memory.count(p))
            throw std::runtime_error(what + ": '" + p + "' ja e uma classe; use outro nome para o tipo (ex.: T)");
        if (interface_memory.count(p) || generic_iface_memory.count(p))
            throw std::runtime_error(what + ": '" + p + "' ja e uma interface; use outro nome para o tipo (ex.: T)");
        GhostConstraint c;
        if (pc.size() > 1) {
            const bool ext = pc[1].type == GTokenType::KwExtends;
            const bool imp = pc[1].type == GTokenType::Identifier && pc[1].value == "implements";
            if (!ext && !imp) throw std::runtime_error(what + ": depois de " + p + " use ',' , extends ou implements");
            c.kind = ext ? 1 : 2;
            std::string cur;
            int d = 0;
            auto flush = [&]() {
                if (cur.empty()) throw std::runtime_error(what + ": faltou o tipo depois de " + std::string(ext ? "extends" : "implements"));
                if (ext && cur != "number") {
                    const size_t lt = cur.find('<');
                    if (lt != std::string::npos && generic_memory.count(cur.substr(0, lt))) ensureInstance(cur);
                    else if (!class_memory.count(cur))
                        throw std::runtime_error(what + ": " + p + " extends " + cur + ": '" + cur + "' nao e uma classe declarada (use uma classe ou number)");
                }
                if (imp) {
                    const size_t lt = cur.find('<');
                    const std::string head = lt == std::string::npos ? cur : cur.substr(0, lt);
                    if (lt != std::string::npos && generic_iface_memory.count(head)) {
                        bool hasUnresolvedParam = false;
                        for (const auto& dpn : declaredParamNames) {
                            if (cur.find(dpn) != std::string::npos) { hasUnresolvedParam = true; break; }
                        }
                        if (!hasUnresolvedParam) ensureIfaceInstance(cur);
                    } else if (generic_iface_memory.count(head)) {
                        throw std::runtime_error(what + ": " + p + " implements " + head + " e generica: use " + head + "<tipo>");
                    } else if (!interface_memory.count(cur)) {
                        throw std::runtime_error(what + ": " + p + " implements " + cur + ": '" + cur + "' nao e uma interface declarada");
                    }
                }
                c.names.push_back(cur);
                cur.clear();
            };
            for (size_t k = 2; k < pc.size(); ++k) {
                const auto ty = pc[k].type;
                if (ty == GTokenType::Lt) d++;
                else if (ty == GTokenType::Gt) d--;
                if (ty == GTokenType::BitAnd && d == 0) { flush(); continue; }
                cur += ty == GTokenType::Comma ? std::string(",") : pc[k].value;
            }
            flush();
        }
        names.push_back(p);
        cons.push_back(c);
    }
}

void GhostInterpreter::checkConstraints(const std::vector<std::string>& params, const std::vector<GhostConstraint>& cons,
                                        const std::vector<std::string>& args, const std::string& where) {
    for (size_t k = 0; k < params.size() && k < cons.size() && k < args.size(); ++k) {
        const GhostConstraint& c = cons[k];
        const std::string& a = args[k];
        ensureTypeReady(a);
        for (const auto& rawN : c.names) {
            const std::string n = substituteTypes(rawN, params, args);
            ensureTypeReady(n);
            if (c.kind == 1 && n == "number") {
                if (a != "int" && a != "double" && a != "float")
                    throw std::runtime_error(where + ": " + a + " nao e numero (restricao " + params[k] + " extends number)");
            } else if (c.kind == 1) {
                const size_t lt = a.find('<');
                if (lt != std::string::npos && generic_memory.count(a.substr(0, lt))) ensureInstance(a);
                if (!class_memory.count(a) || !isSubclass(a, n))
                    throw std::runtime_error(where + ": " + a + " nao e " + n + " nem subclasse (restricao " + params[k] + " extends " + n + ")");
            } else if (c.kind == 2) {
                const bool ok = (class_memory.count(a) && implementsIface(a, n)) || (interface_memory.count(a) && ifaceExtends(a, n));
                if (!ok) throw std::runtime_error(where + ": " + a + " nao implementa " + n + " (restricao " + params[k] + " implements " + n + ")");
            }
        }
    }
}

bool GhostInterpreter::genericCallAt(const std::vector<Token>& t, size_t i) {
    if (i >= t.size() || t[i].type != GTokenType::Lt) return false;
    int depth = 0;
    for (size_t k = i; k < t.size(); ++k) {
        const auto ty = t[k].type;
        if (ty == GTokenType::Lt) depth++;
        else if (ty == GTokenType::Gt) depth--;
        else if (ty == GTokenType::Chain) depth -= 2;
        else if (ty == GTokenType::Semicolon || ty == GTokenType::LParen || ty == GTokenType::RParen) return false;
        if (depth <= 0) return k + 1 < t.size() && t[k + 1].type == GTokenType::LParen;
    }
    return false;
}

std::vector<std::string> GhostInterpreter::typeArgsAt(const std::vector<Token>& t, size_t& i, const std::string& head) {
    const std::string spec = specFromTokens(t, i, head);
    std::vector<std::string> out;
    int depth = 0;
    std::string cur;
    for (size_t k = head.size() + 1; k + 1 < spec.size(); ++k) {
        const char c = spec[k];
        if (c == '<') depth++;
        if (c == '>') depth--;
        if (c == ',' && depth == 0) { out.push_back(cur); cur.clear(); continue; }
        cur += c;
    }
    out.push_back(cur);
    return out;
}

const GhostInterpreter::GhostClassDef::GenericMethod* GhostInterpreter::findGenericMethod(const std::string& cls, const std::string& name, std::string* owner) const {
    std::string c = cls;
    for (int guard = 0; !c.empty() && guard < 256; ++guard) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return nullptr;
        auto m = it->second.genericMethods.find(name);
        if (m != it->second.genericMethods.end()) { if (owner) *owner = c; return &m->second; }
        c = it->second.base;
    }
    return nullptr;
}

std::string GhostInterpreter::instantiateFunction(const std::string& name, const std::vector<std::string>& args) {
    const GhostGenericFn& g = generic_functions.at(name);
    std::string spec = name + "<";
    for (size_t k = 0; k < args.size(); ++k) spec += (k ? "," : "") + args[k];
    spec += ">";
    if (functions_memory.count(spec)) return spec;
    checkConstraints(g.params, g.constraints, args, spec);
    for (const auto& a : args) { ensureTypeReady(a); }
    Lexer fl(substituteTypes(g.source, g.params, args));
    std::string got;
    GhostFunction f = readFunctionDef(fl, got);
    functions_memory[spec] = f;
    return spec;
}

namespace {
// Tipo de um valor para deduzir T ("" se não der: lista vazia, mapa, misturas...)
std::string typeOfValue(const GhostValue& v);
std::string elemTypeOf(const std::vector<GhostValue>& xs) {
    if (xs.empty()) return "";
    bool allInt = true, allNum = true;
    for (const auto& e : xs) {
        if (e.kind != GhostValue::Num) { allNum = false; allInt = false; break; }
        if (!e.isInt) allInt = false;
    }
    if (allNum) return allInt ? "int" : "double";
    const std::string first = typeOfValue(xs[0]);
    for (const auto& e : xs) if (typeOfValue(e) != first) return "";
    return first;
}
std::string typeOfValue(const GhostValue& v) {
    switch (v.kind) {
        case GhostValue::Num: return v.isInt ? "int" : "double";
        case GhostValue::Bool: return "bool";
        case GhostValue::Str: return "string";
        case GhostValue::Object: case GhostValue::Iface: return v.str;
        case GhostValue::Result: return v.str;
        case GhostValue::Set: { const std::string e = v.str.empty() ? elemTypeOf(v.list) : v.str; return e.empty() ? "" : "set<" + e + ">"; }
        case GhostValue::List: { const std::string e = v.str.empty() ? elemTypeOf(v.list) : v.str; return e.empty() ? "" : "list<" + e + ">"; }
        case GhostValue::Fn: {
            if (v.fn && (v.fn->isPolyLambda || v.fn->isGenericFnVal)) return "";
            if (GhostInterpreter::isFnSpec(v.str)) return v.str;
            if (!v.fn) return "";
            std::string ret = !v.fn->wrapperRetSpec.empty() ? v.fn->wrapperRetSpec
                                                            : (v.fn->hasExplicitRet ? v.fn->returnSpec : v.fn->inferredRetSpec);
            if (ret.rfind("ok<", 0) == 0 && ret.back() == '>') ret = "result<" + ret.substr(3, ret.size() - 4) + ">";
            if (ret.empty() || ret == "result") return "";
            std::string ps;
            for (size_t k = 0; k < v.fn->paramSpecs.size(); ++k) ps += (k ? "," : "") + v.fn->paramSpecs[k];
            return "fn<" + ret + "(" + ps + ")>";
        }
        default: return "";
    }
}
// Separa "Pilha<int,list<int>>" em cabeça "Pilha" e argumentos {"int", "list<int>"};
// e "fn<Ret(P1,P2)>" em cabeça "fn/2" e argumentos {"Ret", "P1", "P2"}.
bool splitSpec(const std::string& s, std::string& head, std::vector<std::string>& args) {
    if (GhostInterpreter::isFnSpec(s)) {
        std::string ret;
        std::vector<std::string> ps;
        if (!GhostInterpreter::parseFnSignature(s, ret, ps)) return false;
        head = "fn/" + std::to_string(ps.size());
        args.clear();
        args.push_back(ret);
        for (const auto& p : ps) args.push_back(p);
        return true;
    }
    const size_t lt = s.find('<');
    if (lt == std::string::npos || s.back() != '>') return false;
    head = s.substr(0, lt);
    args.clear();
    std::string cur;
    int depth = 0;
    for (size_t k = lt + 1; k + 1 < s.size(); ++k) {
        const char c = s[k];
        if (c == '<') depth++;
        if (c == '>') depth--;
        if (c == ',' && depth == 0) { args.push_back(cur); cur.clear(); continue; }
        cur += c;
    }
    args.push_back(cur);
    return true;
}
// Casa o tipo do parâmetro (com T) com o tipo do argumento e anota T. Falso se T sair com dois tipos.
bool deduce(const std::string& pattern, const std::string& actual, const std::vector<std::string>& params,
            std::map<std::string, std::string>& got, std::string& conflict) {
    if (actual.empty()) return true;
    if (std::find(params.begin(), params.end(), pattern) != params.end()) {
        auto it = got.find(pattern);
        if (it != got.end() && it->second != actual) { conflict = pattern + " deduzido como " + it->second + " e como " + actual; return false; }
        got[pattern] = actual;
        return true;
    }
    std::string ph, ah;
    std::vector<std::string> pa, aa;
    if (!splitSpec(pattern, ph, pa) || !splitSpec(actual, ah, aa) || ph != ah || pa.size() != aa.size()) return true;
    for (size_t k = 0; k < pa.size(); ++k)
        if (!deduce(pa[k], aa[k], params, got, conflict)) return false;
    return true;
}
} // namespace

GhostValue GhostInterpreter::callGenericFunction(const std::string& name, const std::vector<std::string>& explicitArgs, std::vector<std::vector<Token>>& raw) {
    const GhostGenericFn g = generic_functions.at(name);
    std::vector<GhostValue> vals;
    for (auto& r : raw) vals.push_back(evalValue(r));
    const std::vector<std::string> args = deduceGenericTypes(g, explicitArgs, vals, name);
    const std::string spec = instantiateFunction(name, args);
    const GhostFunction f = functions_memory.at(spec);
    if (vals.size() > f.parameters.size())
        throw std::runtime_error(spec + "(): recebeu " + std::to_string(vals.size()) + " argumento(s), espera ate " + std::to_string(f.parameters.size()));
    for (size_t k = vals.size(); k < f.parameters.size(); ++k) {
        if (k < f.defaults.size() && !f.defaults[k].empty()) { auto d = f.defaults[k]; vals.push_back(evalValue(d)); }
        else throw std::runtime_error(spec + "(): faltou o argumento '" + f.parameters[k].second + "'");
    }
    GhostValue res = runFunction(f, spec, vals, nullptr, "");
    syncQuantumArgs(f, raw, vals);
    return res;
}

// Tipos de uma função/método genérico: os explícitos (<double>) e o resto deduzido pelos argumentos, como no C++
std::vector<std::string> GhostInterpreter::deduceGenericTypes(const GhostGenericFn& g, const std::vector<std::string>& explicitArgs,
                                                              const std::vector<GhostValue>& vals, const std::string& name) {
    if (explicitArgs.size() > g.params.size())
        throw std::runtime_error(name + " tem " + std::to_string(g.params.size()) + " tipo(s) entre < >, recebeu " + std::to_string(explicitArgs.size()));
    std::map<std::string, std::string> got;
    for (size_t k = 0; k < explicitArgs.size(); ++k) got[g.params[k]] = explicitArgs[k];
    std::string conflict;
    for (size_t k = 0; k < vals.size() && k < g.paramTypes.size(); ++k) {
        bool fixed = false; // tipos explícitos não são deduzidos de novo
        for (size_t e = 0; e < explicitArgs.size(); ++e) if (g.paramTypes[k] == g.params[e]) fixed = true;
        if (fixed) continue;
        std::map<std::string, std::string> trial = got;
        for (size_t e = 0; e < explicitArgs.size(); ++e) trial.erase(g.params[e]);
        if (!deduce(g.paramTypes[k], typeOfValue(vals[k]), g.params, trial, conflict))
            throw std::runtime_error(name + "(): " + conflict + " (use " + name + "<tipo>(...))");
        for (const auto& kv : trial) if (!got.count(kv.first)) got[kv.first] = kv.second;
    }
    // Passe 2: deduz tipos restantes a partir de lambdas polimorficas ou funcoes genericas passadas como valor
    for (size_t k = 0; k < vals.size() && k < g.paramTypes.size(); ++k) {
        if (vals[k].kind != GhostValue::Fn || !vals[k].fn) continue;
        const auto& fnObj = *vals[k].fn;
        if (!fnObj.isPolyLambda && !fnObj.isGenericFnVal) continue;
        const std::string pat = g.paramTypes[k];
        if (!isFnSpec(pat)) continue;
        std::string patRet;
        std::vector<std::string> patParams;
        if (!parseFnSignature(pat, patRet, patParams)) continue;
        std::vector<std::string> concretePatParams;
        bool allParamsKnown = true;
        for (const auto& pp : patParams) {
            std::string sub = pp;
            for (const auto& kv : got) {
                if (sub == kv.first) sub = kv.second;
            }
            for (const auto& p : g.params) {
                if (sub.find(p) != std::string::npos) { allParamsKnown = false; break; }
            }
            if (!allParamsKnown) break;
            concretePatParams.push_back(sub);
        }
        if (!allParamsKnown) continue;
        std::string derivedSpec;
        if (fnObj.isPolyLambda) {
            auto copy = fnObj;
            copy.paramSpecs = concretePatParams;
            copy.params.resize(concretePatParams.size());
            copy.paramClasses.resize(concretePatParams.size());
            for (size_t pi = 0; pi < concretePatParams.size(); ++pi) {
                copy.params[pi].second = pi < fnObj.params.size() ? fnObj.params[pi].second : "arg" + std::to_string(pi);
                GTokenType dummyTy;
                std::string dummyCls;
                if (concretePatParams[pi] == "int") dummyTy = GTokenType::KwInt;
                else if (concretePatParams[pi] == "double") dummyTy = GTokenType::KwDouble;
                else if (concretePatParams[pi] == "bool") dummyTy = GTokenType::KwBool;
                else if (concretePatParams[pi] == "string") dummyTy = GTokenType::KwString;
                else { dummyTy = GTokenType::KwVar; dummyCls = concretePatParams[pi]; }
                copy.params[pi].first = dummyTy;
                copy.paramClasses[pi] = dummyCls;
            }
            std::string retT = copy.hasExplicitRet ? copy.returnSpec : inferLambdaRetSpec(copy);
            if (retT.rfind("ok<", 0) == 0 && retT.back() == '>') retT = "result<" + retT.substr(3, retT.size() - 4) + ">";
            if (!retT.empty() && retT != "result") {
                std::string ps;
                for (size_t pi = 0; pi < concretePatParams.size(); ++pi) ps += (pi ? "," : "") + concretePatParams[pi];
                derivedSpec = "fn<" + retT + "(" + ps + ")>";
            }
        } else if (fnObj.isGenericFnVal) {
            auto git = generic_functions.find(fnObj.genericFnName);
            if (git != generic_functions.end()) {
                try {
                    std::vector<GhostValue> dummyVals;
                    for (const auto& cp : concretePatParams) dummyVals.push_back(defaultOf(cp));
                    std::vector<std::string> subArgs = deduceGenericTypes(git->second, {}, dummyVals, git->second.name);
                    std::string instN = instantiateFunction(git->second.name, subArgs);
                    if (functions_memory.count(instN)) {
                        const auto& instF = functions_memory.at(instN);
                        std::string ps;
                        for (size_t pi = 0; pi < instF.paramSpecs.size(); ++pi) ps += (pi ? "," : "") + instF.paramSpecs[pi];
                        std::string retT = instF.returnSpec.empty() ? "void" : instF.returnSpec;
                        derivedSpec = "fn<" + retT + "(" + ps + ")>";
                    }
                } catch (...) {}
            }
        }
        if (!derivedSpec.empty()) {
            std::map<std::string, std::string> trial = got;
            if (deduce(pat, derivedSpec, g.params, trial, conflict)) {
                for (const auto& kv : trial) if (!got.count(kv.first)) got[kv.first] = kv.second;
            }
        }
    }
    std::vector<std::string> args;
    for (const auto& p : g.params) {
        auto it = got.find(p);
        if (it == got.end()) throw std::runtime_error(name + "(): nao deu para descobrir o tipo " + p + " pelos argumentos (use " + name + "<tipo>(...))");
        args.push_back(it->second);
    }
    return args;
}

// obj.m(...) / obj.m<int>(...) / Classe.m(...) com m genérico: cria m<int> na classe dona na primeira vez
GhostValue GhostInterpreter::callGenericMember(GhostValue* self, const std::string& owner, const std::string& name,
                                               const std::vector<std::string>& explicitArgs, std::vector<std::vector<Token>>& raw) {
    const GhostClassDef::GenericMethod gm = class_memory.at(owner).genericMethods.at(name);
    checkAccess(owner, gm.priv, gm.prot, "metodo " + name + "()");
    if (!gm.isStatic && !self) throw std::runtime_error(owner + "." + name + "() nao e static: chame em um objeto");
    std::vector<GhostValue> vals;
    for (auto& r : raw) vals.push_back(evalValue(r));
    const std::vector<std::string> args = deduceGenericTypes(gm.fn, explicitArgs, vals, owner + "." + name);
    std::string spec = name + "<";
    for (size_t k = 0; k < args.size(); ++k) spec += (k ? "," : "") + args[k];
    spec += ">";
    GhostClassDef& def = class_memory.at(owner);
    auto& table = gm.isStatic ? def.staticMethods : def.methods;
    if (!table.count(spec)) {
        checkConstraints(gm.fn.params, gm.fn.constraints, args, owner + "." + spec);
        for (const auto& a : args) { ensureTypeReady(a); }
        Lexer fl(substituteTypes(gm.fn.source, gm.fn.params, args));
        std::string got;
        GhostFunction f = readFunctionDef(fl, got);
        GhostClassDef& d2 = class_memory.at(owner);
        (gm.isStatic ? d2.staticMethods : d2.methods)[spec] = f;
        (gm.isStatic ? d2.privateStatic : d2.privateMethod)[spec] = gm.priv;
    }
    const GhostFunction f = (gm.isStatic ? class_memory.at(owner).staticMethods : class_memory.at(owner).methods).at(spec);
    const std::string display = owner + "." + spec;
    if (vals.size() > f.parameters.size())
        throw std::runtime_error(display + "(): recebeu " + std::to_string(vals.size()) + " argumento(s), espera ate " + std::to_string(f.parameters.size()));
    for (size_t k = vals.size(); k < f.parameters.size(); ++k) {
        if (k < f.defaults.size() && !f.defaults[k].empty()) { auto d = f.defaults[k]; vals.push_back(evalValue(d)); }
        else throw std::runtime_error(display + "(): faltou o argumento '" + f.parameters[k].second + "'");
    }
    return runFunction(f, display, vals, gm.isStatic ? nullptr : self, owner);
}

// Pilha<int>: cria a classe concreta na primeira vez (T trocado por int no corpo)
void GhostInterpreter::ensureInstance(const std::string& spec) {
    if (class_memory.count(spec)) return;
    if (std::find(instantiating.begin(), instantiating.end(), spec) != instantiating.end()) return; // em construção
    const size_t lt = spec.find('<');
    if (lt == std::string::npos || spec.back() != '>') throw std::runtime_error("tipo invalido: " + spec);
    const std::string name = spec.substr(0, lt);
    auto g = generic_memory.find(name);
    if (g == generic_memory.end()) throw std::runtime_error(name + " nao e uma classe generica");
    std::vector<std::string> args;
    std::string cur;
    int depth = 0;
    for (size_t k = lt + 1; k + 1 < spec.size(); ++k) {
        const char c = spec[k];
        if (c == '<') depth++;
        if (c == '>') depth--;
        if (c == ',' && depth == 0) { args.push_back(cur); cur.clear(); continue; }
        cur += c;
    }
    args.push_back(cur);
    if (args.size() != g->second.params.size())
        throw std::runtime_error(name + " precisa de " + std::to_string(g->second.params.size()) + " tipo(s) entre < >, recebeu " + std::to_string(args.size()));
    for (const auto& a : args) if (a.empty()) throw std::runtime_error("tipo vazio em " + spec);
    if (instantiating.size() > 32) throw std::runtime_error(spec + ": tipos genericos aninhados demais");
    const GhostGenericDef gd = g->second;
    checkConstraints(gd.params, gd.constraints, args, spec);
    instantiating.push_back(spec);
    try {
        GhostClassDef def;
        def.name = spec;
        def.base = gd.base;
        for (const auto& iface : gd.interfaces) {
            std::string subIface = substituteTypes(iface, gd.params, args);
            ensureTypeReady(subIface);
            def.interfaces.push_back(subIface);
        }
        defineClass(def, substituteTypes(gd.body, gd.params, args), gd.name);
    } catch (...) {
        instantiating.pop_back();
        throw;
    }
    instantiating.pop_back();
}

void GhostInterpreter::defineClass(GhostClassDef def, const std::string& body, const std::string& ctorName) {
    Lexer bl(body);
    bool secPriv = false, secProt = false; // seção atual (private: / protected: / public:)
    bool priv = false;
    bool prot = false;
    const bool isInstance = def.name != ctorName; // Pilha<int> (de uma classe genérica)
    while (true) {
        Token c = bl.getNextToken();
        if (c.type == GTokenType::EndOfFile) break;
        if (c.type == GTokenType::Semicolon) continue;
        priv = secPriv;
        prot = secProt;
        if (c.type == GTokenType::KwPublic || c.type == GTokenType::KwPrivate || (c.type == GTokenType::Identifier && c.value == "protected")) {
            const bool p1 = c.type == GTokenType::KwPrivate;
            const bool p2 = (c.type == GTokenType::Identifier && c.value == "protected");
            if (bl.peekNextToken().type == GTokenType::Colon) { // seção: vale até a próxima
                bl.getNextToken();
                secPriv = p1;
                secProt = p2;
                continue;
            }
            // modificador do membro: private double saldo; / private function void f() { }
            priv = p1;
            prot = p2;
            c = bl.getNextToken();
        }
        bool isStatic = false; // static function f() { }   static int total = 0;
        if (c.type == GTokenType::Identifier && c.value == "static" && bl.peekNextToken().type != GTokenType::Semicolon) {
            isStatic = true;
            c = bl.getNextToken();
        }
        if (c.type == GTokenType::KwFunction) { // function T m<T>(...): método genérico (modelo)
            GhostGenericFn g;
            if (readGenericFn(bl, g)) {
                if (g.name == ctorName) throw std::runtime_error("o construtor de " + ctorName + " nao pode ser generico");
                if (def.methods.count(g.name) || def.staticMethods.count(g.name) || def.genericMethods.count(g.name))
                    throw std::runtime_error("class " + def.name + ": metodo " + g.name + " declarado duas vezes");
                GhostClassDef::GenericMethod gm;
                gm.fn = g;
                gm.isStatic = isStatic;
                gm.priv = priv;
                gm.prot = prot;
                def.genericMethods[g.name] = gm;
                generic_method_names.insert(g.name);
                continue;
            }
        }
        if (isStatic && c.type == GTokenType::KwFunction) {
            std::string mname;
            GhostFunction f = readFunctionDef(bl, mname);
            if (mname == ctorName) throw std::runtime_error("o construtor de " + def.name + " nao pode ser static");
            auto& ovs = def.static_method_overloads[mname];
            bool dup = false;
            for (const auto& existing : ovs) {
                if (existing.paramSpecs == f.paramSpecs) { dup = true; break; }
            }
            if (dup) throw std::runtime_error("class " + def.name + ": metodo static " + mname + " com os mesmos parametros declarado duas vezes");
            ovs.push_back(f);
            def.staticMethods[mname] = f;
            def.privateStatic[mname] = priv;
            def.protectedStatic[mname] = prot;
            continue;
        }
        if (isStatic && (c.type == GTokenType::KwQreg || c.type == GTokenType::KwQcircuit || c.type == GTokenType::KwQlayer))
            throw std::runtime_error("class " + def.name + ": campos static " + c.value + " nao existem (use um campo normal)");
        if (c.type == GTokenType::KwFunction) {
            std::string mname;
            GhostFunction f = readFunctionDef(bl, mname);
            if (mname == ctorName) {
                if (def.hasCtor) throw std::runtime_error("class " + def.name + ": so pode haver um construtor");
                // super(args); na primeira linha: guardado à parte, roda antes do corpo
                Lexer sb(f.body);
                Token a = sb.getNextToken();
                if (a.type == GTokenType::KwSuper && sb.peekNextToken().type == GTokenType::LParen) {
                    std::vector<Token> grp;
                    int d = 0;
                    while (true) {
                        Token x = sb.getNextToken();
                        if (x.type == GTokenType::EndOfFile) throw std::runtime_error(def.name + ": super(... sem ')'");
                        if (x.type == GTokenType::LParen) d++;
                        if (x.type == GTokenType::RParen) d--;
                        grp.push_back(x);
                        if (d == 0) break;
                    }
                    if (sb.getNextToken().type != GTokenType::Semicolon) throw std::runtime_error(def.name + ": esperado ';' depois de super(...)");
                    size_t i = 0;
                    def.superArgs = splitArgs(grp, i);
                    def.hasSuperCall = true;
                    f.body = f.body.substr(sb.getPosition());
                }
                def.hasCtor = true;
                def.ctorPrivate = priv;
                def.ctor = f;
            } else {
                auto& ovs = def.method_overloads[mname];
                bool dup = false;
                for (const auto& existing : ovs) {
                    if (existing.paramSpecs == f.paramSpecs) { dup = true; break; }
                }
                if (dup) throw std::runtime_error("class " + def.name + ": metodo " + mname + " com os mesmos parametros declarado duas vezes");
                ovs.push_back(f);
                def.methods[mname] = f;
                def.privateMethod[mname] = priv;
                def.protectedMethod[mname] = prot;
            }
            continue;
        }
        // Campo tipado: int x;  string nome = "a";  list<int> xs;  Motor m;  double[] v;
        GhostFieldDef fd;
        fd.priv = priv;
        fd.prot = prot;
        if (isFieldType(c.type)) {
            fd.type = c.type;
            if ((c.type == GTokenType::KwList || c.type == GTokenType::KwMap) && bl.peekNextToken().type == GTokenType::Lt) {
                std::string suf = "<";
                bl.getNextToken(); // '<'
                int depth = 1;
                while (depth > 0) {
                    Token g = bl.getNextToken();
                    if (g.type == GTokenType::EndOfFile) throw std::runtime_error("tipo generico sem '>'");
                    if (g.type == GTokenType::Lt) depth++;
                    else if (g.type == GTokenType::Gt) depth--;
                    else if (g.type == GTokenType::Chain) depth -= 2;
                    if (g.type == GTokenType::Comma) suf += ",";
                    else suf += g.value;
                }
                std::string norm = validateAndNormalizeTypeSpec(c.value + suf, false);
                ensureTypeReady(norm);
                if (c.type == GTokenType::KwList && isListSpec(norm)) fd.cls = norm;
            }
        } else if (c.type == GTokenType::Identifier &&
                   (c.value == "result" || c.value == "set" || (c.value == "fn" && !class_memory.count("fn") && !interface_memory.count("fn"))) &&
                   bl.peekNextToken().type == GTokenType::Lt) {
            fd.type = GTokenType::KwVar;
            fd.cls = readResultSpec(bl, false, c.value);
            ensureTypeReady(fd.cls);
        } else if (c.type == GTokenType::Identifier && c.value == "fn" && !class_memory.count("fn") && !interface_memory.count("fn")) {
            throw std::runtime_error("class " + def.name + ": use fn<retorno(parametros)> para o campo, ex.: fn<int(int)> cb;");
        } else if (c.type == GTokenType::Identifier && generic_memory.count(c.value)) {
            if (bl.peekNextToken().type != GTokenType::Lt) throw std::runtime_error(c.value + " e uma classe generica: use " + c.value + "<tipo>");
            fd.type = GTokenType::KwVar;
            fd.cls = readResultSpec(bl, false, c.value);
            if (fd.cls == def.name) throw std::runtime_error("class " + def.name + ": um campo nao pode ser da propria classe (use list<" + def.name + ">)");
            ensureInstance(fd.cls);
        } else if (c.type == GTokenType::Identifier && generic_iface_memory.count(c.value)) {
            if (bl.peekNextToken().type != GTokenType::Lt) throw std::runtime_error(c.value + " e uma interface generica: use " + c.value + "<tipo>");
            fd.type = GTokenType::KwVar;
            fd.cls = readResultSpec(bl, false, c.value);
            ensureIfaceInstance(fd.cls);
        } else if (c.type == GTokenType::Identifier && (c.value == def.name || c.value == ctorName)) {
            throw std::runtime_error("class " + def.name + ": um campo nao pode ser da propria classe (use list<" + def.name + ">)");
        } else if (c.type == GTokenType::Identifier && (class_memory.count(c.value) || interface_memory.count(c.value))) {
            fd.type = GTokenType::KwVar;
            fd.cls = c.value;
        } else if (c.type == GTokenType::KwQreg || c.type == GTokenType::KwQcircuit) {
            // qreg q(2);  qcircuit c(n + 1);  (o número de qubits pode usar campos já declarados)
            fd.type = c.type;
            Token fn = bl.getNextToken();
            if (fn.type != GTokenType::Identifier) throw std::runtime_error("class " + def.name + ": esperado o nome do campo depois de " + c.value);
            fd.name = fn.value;
            for (const auto& other : def.fields)
                if (other.name == fd.name) throw std::runtime_error("class " + def.name + ": campo " + fd.name + " declarado duas vezes");
            if (!def.base.empty() && findField(def.base, fd.name)) throw std::runtime_error("class " + def.name + ": o campo " + fd.name + " ja existe na classe base");
            if (bl.getNextToken().type != GTokenType::LParen)
                throw std::runtime_error("class " + def.name + ": use " + c.value + " " + fd.name + "(numero_de_qubits);");
            int depth = 1;
            while (true) {
                Token x = bl.getNextToken();
                if (x.type == GTokenType::EndOfFile) throw std::runtime_error("class " + def.name + ": faltou ')' em " + fd.name);
                if (x.type == GTokenType::LParen) depth++;
                if (x.type == GTokenType::RParen && --depth == 0) break;
                fd.init.push_back(x);
            }
            if (fd.init.empty()) throw std::runtime_error("class " + def.name + ": " + c.value + " " + fd.name + "() precisa do numero de qubits");
            const Token semi = bl.getNextToken();
            if (semi.type != GTokenType::Semicolon && semi.type != GTokenType::EndOfFile) throw std::runtime_error("class " + def.name + ": esperado ';' depois do campo " + fd.name);
            def.fields.push_back(fd);
            continue;
        } else if (c.type == GTokenType::KwQlayer) {
            throw std::runtime_error("class " + def.name + ": campos qlayer nao existem em classes (use qreg ou qcircuit)");
        } else {
            throw std::runtime_error("'" + c.value + "' nao vale dentro da classe " + def.name + " (use campos tipados, function e public:/private:)");
        }
        if (bl.peekNextToken().type == GTokenType::LBracket) { // T[] nome / T[][] nome
            std::string baseElem = !fd.cls.empty() ? fd.cls : c.value;
            while (bl.peekNextToken().type == GTokenType::LBracket) {
                bl.getNextToken();
                if (bl.getNextToken().type != GTokenType::RBracket) throw std::runtime_error("class " + def.name + ": use T[] nome;");
                baseElem = "list<" + baseElem + ">";
            }
            std::string norm = validateAndNormalizeTypeSpec(baseElem, false);
            ensureTypeReady(norm);
            fd.type = GTokenType::KwList;
            fd.cls = isListSpec(norm) ? norm : "";
        }
        Token fn = bl.getNextToken();
        if (fn.type != GTokenType::Identifier) throw std::runtime_error("class " + def.name + ": esperado o nome do campo depois de '" + c.value + "'");
        fd.name = fn.value;
        for (const auto& other : def.fields)
            if (other.name == fd.name) throw std::runtime_error("class " + def.name + ": campo " + fd.name + " declarado duas vezes");
        for (const auto& other : def.staticFields)
            if (other.name == fd.name) throw std::runtime_error("class " + def.name + ": campo " + fd.name + " declarado duas vezes");
        if (!def.base.empty() && findField(def.base, fd.name)) throw std::runtime_error("class " + def.name + ": o campo " + fd.name + " ja existe na classe base");
        Token after = bl.getNextToken();
        if (after.type == GTokenType::Assign) {
            int depth = 0;
            while (true) {
                Token x = bl.peekNextToken();
                if (x.type == GTokenType::EndOfFile) break;
                if (depth == 0 && x.type == GTokenType::Semicolon) { bl.getNextToken(); break; }
                if (x.type == GTokenType::LParen || x.type == GTokenType::LBracket || x.type == GTokenType::LBrace) depth++;
                if (x.type == GTokenType::RParen || x.type == GTokenType::RBracket || x.type == GTokenType::RBrace) depth--;
                fd.init.push_back(bl.getNextToken());
            }
            if (fd.init.empty()) throw std::runtime_error("class " + def.name + ": faltou o valor do campo " + fd.name);
        } else if (after.type != GTokenType::Semicolon && after.type != GTokenType::EndOfFile) {
            throw std::runtime_error("class " + def.name + ": esperado ';' depois do campo " + fd.name);
        }
        if (fd.type == GTokenType::KwVar && fd.cls.empty() && fd.init.empty())
            throw std::runtime_error("class " + def.name + ": campo var " + fd.name + " precisa de valor inicial");
        if (isStatic) def.staticFields.push_back(fd);
        else def.fields.push_back(fd);
    }

    // Métodos genéricos não são virtuais (como no C++): não podem redefinir nem ser redefinidos
    if (!def.base.empty()) {
        for (const auto& m : def.methods)
            if (findGenericMethod(def.base, m.first, nullptr))
                throw std::runtime_error(def.name + "." + m.first + "(): o metodo generico de " + def.base + " nao pode ser redefinido (metodos genericos nao sao virtuais)");
        for (const auto& g : def.genericMethods)
            if (findMethod(def.base, g.first, nullptr) || findGenericMethod(def.base, g.first, nullptr))
                throw std::runtime_error(def.name + "." + g.first + "(): um metodo generico nao pode redefinir o metodo " + g.first + " de " + def.base);
    }
    // implements: cada metodo das interfaces existe (aqui, na base ou herdado como default), publico, nao static e com os mesmos tipos
    std::map<std::string, std::pair<std::string, const GhostFunction*>> candidateDefaults;
    for (const auto& iface : def.interfaces) {
        ensureTypeReady(iface);
        for (const auto& req : ifaceMethodList(iface)) {
            const GhostFunction* m = nullptr;
            bool mp = false;
            auto own = def.methods.find(req.first);
            if (own != def.methods.end()) { m = &own->second; mp = def.privateMethod[req.first]; }
            else if (!def.base.empty()) m = findMethod(def.base, req.first, nullptr, &mp);
            if (!m && req.second->hasDefault) {
                auto itDef = candidateDefaults.find(req.first);
                if (itDef == candidateDefaults.end()) {
                    candidateDefaults[req.first] = {iface, req.second};
                } else {
                    const std::string& prevIface = itDef->second.first;
                    if (ifaceExtends(iface, prevIface)) {
                        candidateDefaults[req.first] = {iface, req.second};
                    } else if (ifaceExtends(prevIface, iface)) {
                        // prevIface mais derivada, mantem
                    } else {
                        throw std::runtime_error("conflito de metodo padrao '" + req.first + "()' herdado de interfaces diferentes (" +
                                                 prevIface + " e " + iface + "): redefina o metodo na classe " + def.name);
                    }
                }
                continue;
            }
            if (!m) {
                if (def.genericMethods.count(req.first) || (!def.base.empty() && findGenericMethod(def.base, req.first, nullptr)))
                    throw std::runtime_error(def.name + "." + req.first + "() e generico; a interface " + iface + " precisa de um metodo normal");
                if (def.staticMethods.count(req.first)) throw std::runtime_error(def.name + "." + req.first + "() e static; a interface " + iface + " precisa de um metodo normal");
                throw std::runtime_error("a classe " + def.name + " implementa " + iface + " mas nao tem o metodo " + req.first + "()");
            }
            if (mp) throw std::runtime_error(def.name + "." + req.first + "() e private; metodos da interface " + iface + " precisam ser publicos");
            const GhostFunction& r = *req.second;
            bool same = m->returnType == r.returnType && m->returnClass == r.returnClass && m->parameters.size() == r.parameters.size();
            for (size_t k = 0; same && k < r.parameters.size(); ++k) {
                const std::string pc = k < m->paramClasses.size() ? m->paramClasses[k] : "";
                const std::string rc = k < r.paramClasses.size() ? r.paramClasses[k] : "";
                same = m->parameters[k].first == r.parameters[k].first && pc == rc;
            }
            if (!same) throw std::runtime_error(def.name + "." + req.first + "() tem tipos diferentes dos da interface " + iface);
        }
    }
    // Copia metodos padrao herdados para os metodos da classe
    for (const auto& kv : candidateDefaults) {
        if (!def.methods.count(kv.first) && (def.base.empty() || !findMethod(def.base, kv.first, nullptr))) {
            def.methods[kv.first] = *kv.second.second;
            def.privateMethod[kv.first] = false;
        }
    }
    class_memory[def.name] = def;

    // Campos static: criados agora, uma vez por classe (redefinir a classe recomeça os valores)
    for (auto it = static_memory.begin(); it != static_memory.end();)
        it = it->first.rfind(def.name + "::", 0) == 0 ? static_memory.erase(it) : std::next(it);
    static_stack.push_back({def.name, frames.size()});
    try {
        for (const auto& f : def.staticFields) {
            GhostValue v;
            const std::string what = def.name + "." + f.name;
            if (!f.init.empty()) { std::vector<Token> init = f.init; v = coerce(evalValue(init), f.type, f.cls, what); }
            else if (!f.cls.empty()) v = defaultOf(f.cls);
            else {
                switch (f.type) {
                    case GTokenType::KwInt: v = GhostValue::number(0, true); break;
                    case GTokenType::KwDouble: case GTokenType::KwFloat: v = GhostValue::number(0); break;
                    case GTokenType::KwBool: v = GhostValue::boolean(false); break;
                    case GTokenType::KwString: v = GhostValue::text(""); break;
                    case GTokenType::KwComplex: v = GhostValue::cplx(0, 0); break;
                    case GTokenType::KwList: v.kind = GhostValue::List; break;
                    case GTokenType::KwMap: v.kind = GhostValue::Map; break;
                    default: throw std::runtime_error(what + ": campo static var precisa de valor inicial");
                }
            }
            static_memory[def.name + "::" + f.name] = v;
        }
    } catch (...) {
        static_stack.pop_back();
        throw;
    }
    static_stack.pop_back();
}

// ---------------------------------------------------------------- interfaces

// interface Forma [extends A, B] { function double area(); function void escalar(double k); }
void GhostInterpreter::parseInterfaceDef(const Token& nameTok, Lexer& lex) {
    GhostIfaceDef def;
    def.name = nameTok.value;
    if (class_memory.count(def.name) || functions_memory.count(def.name) || generic_memory.count(def.name))
        throw std::runtime_error("interface " + def.name + ": ja existe uma classe ou funcao com esse nome");
    Token t = lex.getNextToken();
    std::vector<std::string> params;
    std::vector<GhostConstraint> constraints;
    if (t.type == GTokenType::Lt) {
        std::vector<Token> inner;
        int depth = 1;
        while (true) {
            Token k = lex.getNextToken();
            if (k.type == GTokenType::EndOfFile || k.type == GTokenType::LBrace)
                throw std::runtime_error("interface " + def.name + "<...>: faltou '>'");
            if (k.type == GTokenType::Lt) depth++;
            else if (k.type == GTokenType::Gt) depth--;
            else if (k.type == GTokenType::Chain) {
                depth -= 2;
                if (depth == 0) { k.type = GTokenType::Gt; k.value = ">"; inner.push_back(k); break; }
            }
            if (depth <= 0) break;
            inner.push_back(k);
        }
        parseTypeParamTokens(inner, "interface " + def.name, params, constraints);
        t = lex.getNextToken();
    }
    if (params.empty() && generic_iface_memory.count(def.name))
        throw std::runtime_error("ja existe uma interface generica chamada " + def.name);
    if (!params.empty() && interface_memory.count(def.name))
        throw std::runtime_error("ja existe uma interface chamada " + def.name);
    if (t.type == GTokenType::KwExtends) {
        while (true) {
            Token b = lex.getNextToken();
            if (b.type != GTokenType::Identifier)
                throw std::runtime_error("interface " + def.name + ": esperado nome da interface base depois de extends");
            std::string baseSpec = b.value;
            if (lex.peekNextToken().type == GTokenType::Lt) {
                baseSpec = readResultSpec(lex, false, b.value);
                if (params.empty()) ensureIfaceInstance(baseSpec);
            } else if (generic_iface_memory.count(b.value)) {
                throw std::runtime_error("interface " + def.name + ": '" + b.value + "' e uma interface generica: use " + b.value + "<tipo>");
            } else if (!interface_memory.count(b.value)) {
                throw std::runtime_error("interface " + def.name + ": '" + b.value + "' nao e uma interface declarada antes");
            }
            def.bases.push_back(baseSpec);
            t = lex.getNextToken();
            if (t.type != GTokenType::Comma) break;
        }
    }
    if (t.type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' depois de interface " + def.name + ".");
    const std::string body = lex.extractBlock();
    if (!params.empty()) {
        GhostGenericIfaceDef g;
        g.name = def.name;
        g.params = params;
        g.constraints = constraints;
        g.bases = def.bases;
        g.body = body;
        for (auto it = interface_memory.begin(); it != interface_memory.end();)
            it = it->first.rfind(def.name + "<", 0) == 0 ? interface_memory.erase(it) : std::next(it);
        generic_iface_memory[def.name] = g;
        return;
    }
    defineInterface(def, body);
}

void GhostInterpreter::defineInterface(GhostIfaceDef def, const std::string& body) {
    Lexer bl(body);
    while (true) {
        Token c = bl.getNextToken();
        if (c.type == GTokenType::EndOfFile) break;
        if (c.type == GTokenType::Semicolon) continue;
        if (c.type != GTokenType::KwFunction)
            throw std::runtime_error("dentro de interface " + def.name + " so vale: function tipo nome(parametros); ou com corpo { ... }");
        std::string mname;
        GhostFunction f = readFunctionDef(bl, mname, true, true);
        for (const auto& d : f.defaults)
            if (!d.empty()) throw std::runtime_error("interface " + def.name + ": parametros de interface nao tem valor padrao");
        if (def.methods.count(mname)) throw std::runtime_error("interface " + def.name + ": metodo " + mname + " declarado duas vezes");
        def.methods[mname] = f;
        def.order.push_back(mname);
    }
    interface_memory[def.name] = def;
}

void GhostInterpreter::ensureIfaceInstance(const std::string& spec) {
    if (interface_memory.count(spec)) return;
    const size_t lt = spec.find('<');
    if (lt == std::string::npos || spec.back() != '>') throw std::runtime_error("tipo invalido: " + spec);
    const std::string name = spec.substr(0, lt);
    auto g = generic_iface_memory.find(name);
    if (g == generic_iface_memory.end()) throw std::runtime_error(name + " nao e uma interface generica");
    std::vector<std::string> args;
    std::string cur;
    int depth = 0;
    for (size_t k = lt + 1; k + 1 < spec.size(); ++k) {
        const char c = spec[k];
        if (c == '<') depth++;
        if (c == '>') depth--;
        if (c == ',' && depth == 0) { args.push_back(cur); cur.clear(); continue; }
        cur += c;
    }
    args.push_back(cur);
    if (args.size() != g->second.params.size())
        throw std::runtime_error(name + " precisa de " + std::to_string(g->second.params.size()) +
                                 " tipo(s) entre < >, recebeu " + std::to_string(args.size()));
    for (const auto& a : args) if (a.empty()) throw std::runtime_error("tipo vazio em " + spec);
    checkConstraints(g->second.params, g->second.constraints, args, spec);
    GhostIfaceDef def;
    def.name = spec;
    for (const auto& base : g->second.bases) {
        std::string subBase = substituteTypes(base, g->second.params, args);
        ensureTypeReady(subBase);
        def.bases.push_back(subBase);
    }
    defineInterface(def, substituteTypes(g->second.body, g->second.params, args));
}

void GhostInterpreter::ensureTypeReady(const std::string& spec) {
    if (spec.empty()) return;
    std::function<void(const std::string&)> walk = [&](const std::string& t) {
        if (isFnSpec(t)) {
            std::string r;
            std::vector<std::string> ps;
            if (parseFnSignature(t, r, ps)) {
                walk(r);
                for (const auto& p : ps) walk(p);
            }
            return;
        }
        const size_t lt = t.find('<');
        if (lt == std::string::npos || t.back() != '>') return;
        const std::string h = t.substr(0, lt);
        std::string cur;
        int d = 0;
        for (size_t k = lt + 1; k + 1 < t.size(); ++k) {
            const char c = t[k];
            if (c == '<') d++;
            else if (c == '>') d--;
            else if (c == ',' && d == 0) { walk(cur); cur.clear(); continue; }
            cur += c;
        }
        if (!cur.empty()) walk(cur);
        if (generic_memory.count(h)) ensureInstance(t);
        if (generic_iface_memory.count(h)) ensureIfaceInstance(t);
    };
    walk(spec);
}

std::vector<std::pair<std::string, const GhostFunction*>> GhostInterpreter::ifaceMethodList(const std::string& iface) const {
    std::vector<std::pair<std::string, const GhostFunction*>> out;
    std::vector<std::string> seen;
    std::vector<std::string> stack{iface};
    std::function<void(const std::string&, int)> walk = [&](const std::string& i, int depth) {
        auto it = interface_memory.find(i);
        if (it == interface_memory.end() || depth > 64) return;
        for (const auto& b : it->second.bases) walk(b, depth + 1);
        for (const auto& n : it->second.order) {
            auto itSeen = std::find(seen.begin(), seen.end(), n);
            if (itSeen != seen.end()) {
                size_t idx = static_cast<size_t>(itSeen - seen.begin());
                out[idx].second = &it->second.methods.at(n);
                continue;
            }
            seen.push_back(n);
            out.push_back({n, &it->second.methods.at(n)});
        }
    };
    walk(iface, 0);
    return out;
}

bool GhostInterpreter::ifaceExtends(const std::string& a, const std::string& b) const {
    if (a == b) return true;
    auto it = interface_memory.find(a);
    if (it == interface_memory.end()) return false;
    for (const auto& base : it->second.bases)
        if (ifaceExtends(base, b)) return true;
    return false;
}

bool GhostInterpreter::implementsIface(const std::string& cls, const std::string& iface) const {
    std::string c = cls;
    for (int guard = 0; !c.empty() && guard < 256; ++guard) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return false;
        for (const auto& i : it->second.interfaces)
            if (ifaceExtends(i, iface)) return true;
        c = it->second.base;
    }
    return false;
}

// Forma f = Quadrado(2);  descrever(Forma f)  ...: guarda o objeto inteiro, se a classe implementa a interface
GhostValue GhostInterpreter::coerceIface(const GhostValue& v, const std::string& iface, const std::string& what) {
    GhostValue r;
    r.kind = GhostValue::Iface;
    r.str = iface;
    if (v.kind == GhostValue::Iface) {
        if (v.list.empty()) {
            if (!ifaceExtends(v.str, iface)) throw std::runtime_error(what + ": a interface " + v.str + " nao e " + iface);
            return r;
        }
        const GhostValue& inner = v.list[0];
        if (!implementsIface(inner.str, iface)) throw std::runtime_error(what + ": " + inner.str + " nao implementa a interface " + iface);
        r.list.push_back(inner);
        return r;
    }
    if (v.kind != GhostValue::Object) throw std::runtime_error(what + ": esperado objeto que implementa " + iface + ", recebido " + toText(v, true));
    if (!implementsIface(v.str, iface)) throw std::runtime_error(what + ": " + v.str + " nao implementa a interface " + iface);
    r.list.push_back(v);
    return r;
}

// ---------------------------------------------------------------- membros static

std::string GhostInterpreter::currentClassCtx() const {
    if (inMethod()) return this_stack.back().cls;
    if (!static_stack.empty() && static_stack.back().frameDepth == frames.size()) return static_stack.back().cls;
    return "";
}

const GhostInterpreter::GhostFieldDef* GhostInterpreter::findStatic(const std::string& cls, const std::string& name, std::string* owner) const {
    std::string c = cls;
    for (int guard = 0; !c.empty() && guard < 256; ++guard) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return nullptr;
        for (const auto& f : it->second.staticFields)
            if (f.name == name) { if (owner) *owner = c; return &f; }
        c = it->second.base;
    }
    return nullptr;
}

const GhostFunction* GhostInterpreter::findStaticMethod(const std::string& cls, const std::string& name, std::string* owner, bool* priv, bool* prot) const {
    std::string c = cls;
    for (int guard = 0; !c.empty() && guard < 256; ++guard) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return nullptr;
        auto m = it->second.staticMethods.find(name);
        if (m != it->second.staticMethods.end()) {
            if (owner) *owner = c;
            if (priv) { auto p = it->second.privateStatic.find(name); *priv = p != it->second.privateStatic.end() && p->second; }
            if (prot) { auto pr = it->second.protectedStatic.find(name); *prot = pr != it->second.protectedStatic.end() && pr->second; }
            return &m->second;
        }
        c = it->second.base;
    }
    return nullptr;
}

std::vector<GhostFunction> GhostInterpreter::findStaticMethodOverloads(const std::string& cls, const std::string& name, std::string* owner, bool* priv, bool* prot) const {
    std::vector<GhostFunction> res;
    std::string c = cls;
    for (int guard = 0; !c.empty() && guard < 256; ++guard) {
        auto it = class_memory.find(c);
        if (it == class_memory.end()) return res;
        auto mo = it->second.static_method_overloads.find(name);
        if (mo != it->second.static_method_overloads.end() && !mo->second.empty()) {
            if (owner && res.empty()) *owner = c;
            if (priv && res.empty()) { auto p = it->second.privateStatic.find(name); *priv = p != it->second.privateStatic.end() && p->second; }
            if (prot && res.empty()) { auto pr = it->second.protectedStatic.find(name); *prot = pr != it->second.protectedStatic.end() && pr->second; }
            for (const auto& f : mo->second) res.push_back(f);
        } else {
            auto m = it->second.staticMethods.find(name);
            if (m != it->second.staticMethods.end()) {
                if (owner && res.empty()) *owner = c;
                if (priv && res.empty()) { auto p = it->second.privateStatic.find(name); *priv = p != it->second.privateStatic.end() && p->second; }
                if (prot && res.empty()) { auto pr = it->second.protectedStatic.find(name); *prot = pr != it->second.protectedStatic.end() && pr->second; }
                res.push_back(m->second);
            }
        }
        if (!res.empty()) return res;
        c = it->second.base;
    }
    return res;
}

GhostValue* GhostInterpreter::staticRef(const std::string& name, const GhostFieldDef** info) {
    if (!frames.empty() && frames.back().count(name)) return nullptr; // variável local esconde o campo
    const std::string ctx = currentClassCtx();
    if (ctx.empty()) return nullptr;
    std::string owner;
    const GhostFieldDef* fd = findStatic(ctx, name, &owner);
    if (!fd) return nullptr;
    checkAccess(owner, fd->priv, fd->prot, "campo static " + name);
    if (info) *info = fd;
    auto it = static_memory.find(owner + "::" + name);
    return it != static_memory.end() ? &it->second : nullptr;
}

const GhostFunction* GhostInterpreter::selectOverload(const std::vector<GhostFunction>& overloads, const std::vector<GhostValue>& args, const std::string& name) const {
    if (overloads.empty()) return nullptr;
    if (overloads.size() == 1) return &overloads[0];

    const GhostFunction* best = nullptr;
    int bestScore = -1;
    bool ambiguous = false;

    for (const auto& fn : overloads) {
        const size_t totalParams = fn.parameters.size();
        size_t requiredParams = 0;
        for (size_t k = 0; k < totalParams; ++k) {
            if (k >= fn.defaults.size() || fn.defaults[k].empty()) requiredParams++;
        }
        if (args.size() < requiredParams || args.size() > totalParams) continue;

        int score = 0;
        bool match = true;
        for (size_t k = 0; k < args.size(); ++k) {
            const GhostValue& a = args[k];
            const std::string spec = k < fn.paramSpecs.size() ? fn.paramSpecs[k] : "";
            const GTokenType pType = k < fn.parameters.size() ? fn.parameters[k].first : GTokenType::KwVar;
            if (spec.empty() || spec == "var" || spec == "auto" || pType == GTokenType::KwVar) {
                score += 1;
                continue;
            }
            if (spec == "int" || pType == GTokenType::KwInt) {
                if (a.kind == GhostValue::Num && (a.isInt || std::trunc(a.num) == a.num)) score += 4;
                else if (a.kind == GhostValue::Num) score += 2;
                else { match = false; break; }
            } else if (spec == "double" || spec == "float" || pType == GTokenType::KwDouble || pType == GTokenType::KwFloat) {
                if (a.kind == GhostValue::Num) score += ((a.isInt || std::trunc(a.num) == a.num) ? 2 : 4);
                else { match = false; break; }
            } else if (spec == "string" || pType == GTokenType::KwString) {
                if (a.kind == GhostValue::Str) score += 4;
                else { match = false; break; }
            } else if (spec == "bool" || pType == GTokenType::KwBool) {
                if (a.kind == GhostValue::Bool) score += 4;
                else { match = false; break; }
            } else if (a.kind == GhostValue::List && (spec.rfind("list", 0) == 0 || pType == GTokenType::KwList)) {
                score += 4;
            } else if (a.kind == GhostValue::Map && (spec.rfind("map", 0) == 0 || pType == GTokenType::KwMap)) {
                score += 4;
            } else if (a.kind == GhostValue::Set && spec.rfind("set", 0) == 0) {
                score += 4;
            } else if (a.kind == GhostValue::Object && (a.str == spec || isSubclass(a.str, spec) || implementsIface(a.str, spec))) {
                score += 4;
            } else {
                match = false;
                break;
            }
        }
        if (!match) continue;

        score += static_cast<int>(totalParams - args.size());

        if (score > bestScore) {
            bestScore = score;
            best = &fn;
            ambiguous = false;
        } else if (score == bestScore) {
            ambiguous = true;
        }
    }

    if (ambiguous) throw std::runtime_error("chamada ambigua para '" + name + "': multiplas sobrecargas compativeis");
    if (!best) throw std::runtime_error("nenhuma sobrecarga compativel para '" + name + "' com os argumentos fornecidos");
    return best;
}

GhostValue GhostInterpreter::callStatic(const std::string& owner, const GhostFunction& f, const std::string& name, std::vector<std::vector<Token>>& raw) {
    const std::string display = owner + "." + name;
    std::vector<GhostValue> vals = evalArgs(f, display, raw);
    return runFunction(f, display, vals, nullptr, owner); // sem objeto + classe = método static
}
