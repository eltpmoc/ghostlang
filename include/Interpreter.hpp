#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "Lexer.hpp"
#include "GhostMatrix.hpp"
#include "GhostLLM.hpp"
#include "GhostQML.hpp"
#include "GhostQTape.hpp"
#include "GhostServer.hpp"
#include "GhostHybrid.hpp"
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <cstdint>

using namespace std;

struct GhostFunction {
    GTokenType returnType;
    std::vector<std::pair<GTokenType, std::string>> parameters;
    std::string body;
    std::vector<std::vector<Token>> defaults; // valor padrão de cada parâmetro (vazio = obrigatório)
    std::vector<std::string> paramClasses;    // classe do parâmetro ("" se não for objeto)
    std::string returnClass;                  // classe do retorno ("" se não for objeto)
    std::string returnSpec;                   // tipo completo do retorno ("int", "list<int>", "fn<int(int)>", "void")
    std::vector<std::string> paramSpecs;      // tipo completo de cada parâmetro ("int", "list<int>", "fn<...>")
    bool hasDefault = false;                  // método padrão com corpo { ... } dentro de interface
};

struct GhostClosure;

// Valor do REPL: número (inteiro ou real), texto, bool, lista, mapa (chaves em texto), objeto ou função.
// Objeto: str = nome da classe, map = campos. Objetos têm semântica de valor, como no programa compilado.
// Result (ok/err): num = 1 se ok; list[0] = valor (ok) ou texto do erro (err); str = tipo "result<T>"
// ("" enquanto ok(...)/err(...) ainda não foi guardado numa variável, parâmetro, retorno ou campo tipado).
// Set (set<T>): list = elementos ordenados e sem repetidos; str = tipo dos elementos ("int"; "" sem tipo).
// Quantum (campo qreg/qcircuit, qreg(n)): qreg ou qcirc. Cópias dividem o ponteiro até alguém mudar o
// registrador; a mudança grava um ponteiro novo (semântica de valor, como a cópia do C++).
// Iface (variável, parâmetro, retorno ou campo do tipo de uma interface): str = interface; list[0] = o objeto
// guardado, inteiro (sem fatiamento); list vazia = interface sem objeto.
// Fn (fn<Ret(Args)>, lambda ou função nomeada): str = assinatura "fn<Ret(P1,P2)>"; fn = closure (nullptr = vazia).
// Cópias compartilham o ponteiro até uma chamada mutar o ambiente capturado (copy-on-write = semântica de valor).
struct GhostValue {
    enum Kind { Num, Str, Bool, List, Map, Object, Result, Set, Quantum, Iface, Fn, Complex } kind = Num;
    double num = 0.0;
    double imag = 0.0;
    bool isInt = false;
    std::string str;
    std::vector<GhostValue> list;
    std::map<std::string, GhostValue> map;
    std::shared_ptr<GhostQReg> qreg;
    std::shared_ptr<GhostQCircuit> qcirc;
    std::shared_ptr<GhostClosure> fn;
    static GhostValue number(double v, bool integer = false) { GhostValue g; g.kind = Num; g.num = v; g.isInt = integer; return g; }
    static GhostValue text(const std::string& s) { GhostValue g; g.kind = Str; g.str = s; return g; }
    static GhostValue boolean(bool b) { GhostValue g; g.kind = Bool; g.num = b ? 1.0 : 0.0; return g; }
    static GhostValue cplx(double r, double im) { GhostValue g; g.kind = Complex; g.num = r; g.imag = im; return g; }
};

// Caminho atribuível no REPL: raiz (variável, campo ou this) + passos .campo / [indice]
struct GhostLStep { bool field = false; GhostValue key; };
struct GhostLPath { std::string root; std::vector<GhostLStep> steps; };

// Closure / valor de função no REPL: lambda (por valor ou referência), lambda polimórfica ou função nomeada/genérica.
struct GhostClosure {
    std::vector<std::pair<GTokenType, std::string>> params;
    std::vector<std::string> paramClasses;
    std::vector<std::string> paramSpecs;
    GTokenType returnType = GTokenType::KwVar;
    std::string returnClass;
    std::string returnSpec;
    bool hasExplicitRet = false;
    std::string inferredRetSpec;   // retorno inferido quando não há '-> tipo'
    std::string wrapperRetSpec;    // retorno alvo quando convertido para fn<Ret(...)> (ex.: int -> double ou void)
    bool isExprBody = true;
    std::vector<Token> exprTokens; // corpo quando (params) => expr
    std::string body;              // corpo quando (params) => { ... }
    bool isNamedFn = false;
    std::string targetFuncName;
    GhostFunction targetFunc;
    struct CapturedVar {
        GhostValue val;
        GTokenType declType = GTokenType::KwVar;
        std::string varCls;
    };
    std::map<std::string, CapturedVar> env;
    bool hasThis = false;
    GhostValue capturedThis;
    std::string capturedClass;
    std::string capturedStaticClass;
    bool isPolyLambda = false;      // (x) => ... com pelo menos um parâmetro de tipo inferido
    bool isGenericFnVal = false;    // valor obtido pelo nome direto de função genérica
    std::string genericFnName;
    bool disallowUncapturedLocals = false; // lambda com [] ou [x] sem '=' nem '&'
    bool defaultRefInFunction = false;     // [&] criado dentro de função (não pode ser retornado)
    struct RefCapture {
        size_t ownerDepth = static_cast<size_t>(-1); // SIZE_MAX = variável global
        uint64_t ownerFrameId = 0;
    };
    std::map<std::string, RefCapture> refEnv;
    bool refThis = false;
    GhostLPath refThisPath;
    size_t refThisOwnerDepth = static_cast<size_t>(-1);
    uint64_t refThisOwnerFrameId = 0;
};

// Sinais de controle do REPL (não derivam de std::exception: catch da linguagem não os captura)
struct GhostBreakSignal {};
struct GhostContinueSignal {};
struct GhostReturnSignal { GhostValue value; bool hasValue = false; };

class GhostInterpreter {
private:
    std::map<std::string, GhostMatrix> memory;
    map<string, double> numeric_memory;
    map<string, string> string_memory;
    map<string, GTokenType> type_memory;
    map<string, GhostFunction> functions_memory;
    std::map<std::string, std::vector<GhostFunction>> function_overloads;
    std::map<std::string, std::shared_ptr<std::mutex>> mutex_memory;
    std::map<std::string, std::shared_ptr<GhostServer>> server_memory;
    std::map<std::string, std::shared_ptr<GhostQLayer>> qlayer_memory;
    std::map<std::string, std::shared_ptr<GhostHybridNetwork>> hybrid_memory;
    std::map<std::string, GhostLLM> llm_memory;

    // Objetos quanticos (src/InterpreterQuantum.cpp)
    std::map<std::string, GhostQReg> qreg_memory;
    std::map<std::string, GhostQCircuit> qcircuit_memory;
    struct QValue {
        enum Kind { None, Number, Text, Map } kind = None;
        double num = 0.0;
        std::string str;
        std::map<std::string, GhostValue> map;
    };
    bool isQuantum(const std::string& name) const;
    void parseQuantumDecl(Lexer& lex, GTokenType type);
    // tokens[i] e o objeto (ou quantum_backend/quantum_device); avanca i ate o ')' final da chamada
    QValue evalQuantumAt(std::vector<Token>& tokens, size_t& i);
    QValue callQuantum(const std::string& obj, const std::string& method, std::vector<std::vector<Token>>& args);
    std::vector<std::vector<Token>> splitArgs(std::vector<Token>& tokens, size_t& i);

    // circuit nome(params) { portas }: sub-circuitos reutilizaveis (src/InterpreterQuantum.cpp)
    struct GhostCircuitDef {
        std::vector<std::pair<GTokenType, std::string>> params; // tipo (KwInt/KwDouble/Identifier) e nome
        std::string body;
    };
    std::map<std::string, GhostCircuitDef> circuit_memory;
    std::string circuit_target;  // registrador alvo enquanto um circuit executa
    int circuit_depth = 0;
    // Gravadores temporarios de nome_dg (inverso): nomes internos "__inv<N>__"
    std::map<std::string, GhostQTape> qtape_memory;
    int inverse_counter = 0;
    void parseCircuitDef(Lexer& lex);
    void runCircuit(const std::string& obj, const std::string& name, std::vector<std::vector<Token>>& args, bool inverse = false, bool controlled = false);
    // "bell" para bell, bell_dg, bell_ctrl e bell_dg_ctrl; "" se nao for circuit. Preenche os sufixos.
    std::string circuitBase(const std::string& called, bool* inverse = nullptr, bool* controlled = nullptr) const;
    // Chamada sem objeto: bell(q, 0, 1); ou, dentro de um circuit, h(a); / outro(a, b);
    bool tryCircuitStatement(const Token& name, const Token& next, Lexer& lex);

    // ---- Núcleo da linguagem no REPL (src/InterpreterCore.cpp) ----
    // Caminho atribuível: raiz (variável, campo ou this) + passos .campo / [indice]
    using LStep = GhostLStep;
    using LPath = GhostLPath;
    std::map<std::string, GhostValue> coll_memory; // listas, mapas e objetos
    std::vector<std::string> error_stack;          // mensagens dos catch ativos (throw; relança)
    int call_depth = 0;
    GhostValue applyBinary(GTokenType op, const GhostValue& a, const GhostValue& b);
    GhostValue evalValue(std::vector<Token>& tokens);
    GhostValue exprTernary(std::vector<Token>& t, size_t& i, bool eval);
    GhostValue exprBinary(std::vector<Token>& t, size_t& i, int minPrec, bool eval);
    GhostValue exprUnary(std::vector<Token>& t, size_t& i, bool eval);
    GhostValue exprPrimary(std::vector<Token>& t, size_t& i, bool eval);
    GhostValue exprPostfix(GhostValue base, const std::string& varName, std::vector<Token>& t, size_t& i, bool eval);
    GhostValue callBuiltin(const std::string& name, std::vector<std::vector<Token>>& args, bool& found);
    // Objetos nativos da IA (src/InterpreterNative.cpp): text_model, transformer_model(_v5),
    // matrix_g, temporal_context, knowledge_consult
    static bool isNativeObject(const GhostValue& v);
    bool callNativeFactory(const std::string& name, std::vector<GhostValue>& args, GhostValue& out);
    GhostValue callNativeMethod(const GhostValue& obj, const std::string& method, std::vector<GhostValue>& args);
    // path: onde fica a coleção (métodos que alteram, como push); nullptr = valor temporário
    GhostValue collMethod(const LPath* path,const GhostValue& self, const std::string& m, std::vector<std::vector<Token>>& args);
    GhostValue callUserFunction(const std::string& name, std::vector<std::vector<Token>>& args);
    GhostValue lookupVar(const std::string& name, bool& found);
    void storeVar(const std::string& name, const GhostValue& v, GTokenType declared);
    std::string toText(const GhostValue& v, bool quoted = false);
    bool truthy(const GhostValue& v);
    double toNum(const GhostValue& v, const std::string& ctx);
    bool tryDeclaration(GTokenType type, Lexer& lex);
    bool tryCoreStatement(const Token& name, const Token& next, Lexer& lex);
    // Quadros de função: variáveis declaradas (e parâmetros) são locais; o resto altera as de fora.
    struct SavedVar {
        bool num = false, str = false, coll = false, typ = false, vc = false, qreg = false, qcirc = false;
        double n = 0; std::string s; GhostValue c; GTokenType t = GTokenType::KwVar; std::string vcls;
        std::shared_ptr<GhostQReg> qr; std::shared_ptr<GhostQCircuit> qc;
    };
    std::vector<std::map<std::string, SavedVar>> frames;
    std::vector<uint64_t> frame_ids;
    uint64_t next_frame_id = 0;
    void pushFrame() { frames.emplace_back(); frame_ids.push_back(++next_frame_id); }
    void popFrameAndRestore() {
        auto frame = std::move(frames.back());
        frames.pop_back();
        if (!frame_ids.empty()) frame_ids.pop_back();
        for (auto& kv : frame) restoreVar(kv.first, kv.second);
    }
    void checkReturnedClosureRefs(const GhostValue& v, size_t returningFrameDepth) const;
    void declareLocal(const std::string& name);
    SavedVar saveVar(const std::string& name);
    void restoreVar(const std::string& name, const SavedVar& s);
    void parseTryRepl(Lexer& lex);
    void parseThrowRepl(Lexer& lex);
    void runForeach(const std::vector<std::string>& vars, std::vector<Token>& coll, const std::string& body, const std::string& elemSpec = "");
    std::string tokensToSource(const std::vector<Token>& tokens);
    // signatureOnly: "function tipo nome(params);" em interfaces (com corpo opcional se allowInterfaceDefault)
    GhostFunction readFunctionDef(Lexer& lex, std::string& name, bool signatureOnly = false, bool allowInterfaceDefault = false);

    // ---- Classes no REPL (src/InterpreterClass.cpp) ----
    struct GhostFieldDef {
        GTokenType type = GTokenType::KwVar;
        std::string cls;               // campo do tipo de uma classe
        std::string name;
        std::vector<Token> init;       // valor inicial (vazio = zero do tipo)
        bool priv = false;
        bool prot = false;
    };
    // Modelo de função ou método genérico: function T maior<T>(T a, T b) { ... }; maior<int> vira uma função de
    // verdade na primeira vez (T trocado por int no texto)
    // Restrição de um tipo genérico: <T extends Animal>, <T implements A & B>, <T extends number>
    struct GhostConstraint {
        int kind = 0;                    // 0 nenhuma, 1 extends (classe ou number), 2 implements
        std::vector<std::string> names;
    };
    struct GhostGenericFn {
        std::string name;
        std::vector<GhostConstraint> constraints;              // uma por tipo (mesma ordem de params)
        std::vector<std::string> params;                       // T, U
        std::string source;                                    // "T maior(T a, T b) { ... }" (sem <T>)
        std::vector<std::string> paramTypes;                   // tipo de cada parâmetro, como texto ("list<T>")
    };
    struct GhostClassDef {
        std::string name, base;
        std::vector<GhostFieldDef> fields;               // só os campos desta classe, na ordem
        std::map<std::string, GhostFunction> methods;    // só os métodos desta classe
        std::map<std::string, bool> privateMethod;
        std::map<std::string, bool> protectedMethod;
        std::map<std::string, std::vector<GhostFunction>> method_overloads;
        bool hasCtor = false, ctorPrivate = false, hasSuperCall = false;
        GhostFunction ctor;                              // corpo sem a linha super(...)
        std::vector<std::vector<Token>> superArgs;
        std::vector<std::string> interfaces;             // implements A, B
        std::vector<GhostFieldDef> staticFields;         // static int total = 0;
        std::map<std::string, GhostFunction> staticMethods;
        std::map<std::string, bool> privateStatic;
        std::map<std::string, bool> protectedStatic;
        std::map<std::string, std::vector<GhostFunction>> static_method_overloads;
        // métodos genéricos: modelo; cada combinação de tipos vira methods["m<int>"] (ou staticMethods)
        struct GenericMethod { GhostGenericFn fn; bool isStatic = false; bool priv = false; bool prot = false; };
        std::map<std::string, GenericMethod> genericMethods;
    };
    // interface Nome [extends A, B] { function tipo metodo(params); }
    struct GhostIfaceDef {
        std::string name;
        std::vector<std::string> bases;
        std::vector<std::string> order;                  // métodos na ordem declarada
        std::map<std::string, GhostFunction> methods;    // só assinaturas
    };
    std::map<std::string, GhostIfaceDef> interface_memory;
    struct GhostGenericIfaceDef {
        std::string name;
        std::vector<std::string> params;
        std::vector<GhostConstraint> constraints;
        std::vector<std::string> bases;
        std::string body;
    };
    std::map<std::string, GhostGenericIfaceDef> generic_iface_memory;
    std::map<std::string, GhostValue> static_memory;    // "Classe::campo" -> valor
    struct StaticFrame { std::string cls; size_t frameDepth = 0; };
    std::vector<StaticFrame> static_stack;               // métodos static em execução
    void parseInterfaceDef(const Token& nameTok, Lexer& lex);
    // import "arquivo.g" [as m];  (mesma busca e mesma renomeação do compilador: include/GhostModules.hpp)
    void importModule(Lexer& lex);
    std::set<std::string> imported_modules;          // caminho|apelido já importados
    std::vector<std::string> import_stack;           // arquivos em leitura (ciclo) e pasta de quem importa
    void defineInterface(GhostIfaceDef def, const std::string& body);
    void ensureIfaceInstance(const std::string& spec);
    void ensureTypeReady(const std::string& spec);
    // class Pilha<T> { ... }: guardada como modelo; Pilha<int> vira uma classe de verdade ("Pilha<int>")
    // com T trocado por int no corpo (como a instanciação de template do compilador)
    struct GhostGenericDef {
        std::string name, base;
        std::vector<std::string> params, interfaces;
        std::vector<GhostConstraint> constraints;
        std::string body;
    };
    std::map<std::string, GhostGenericDef> generic_memory;
    std::vector<std::string> instantiating;              // evita recursão (método que devolve Pilha<T>)
    std::map<std::string, GhostGenericFn> generic_functions;
    bool tryGenericFunctionDef(Lexer& lex);                    // depois de 'function': lê se for genérica
    bool readGenericFn(Lexer& lex, GhostGenericFn& out);       // idem, sem registrar (funções e métodos)
    std::vector<std::string> deduceGenericTypes(const GhostGenericFn& g, const std::vector<std::string>& explicitArgs,
                                                const std::vector<GhostValue>& vals, const std::string& display);
    std::set<std::string> generic_method_names;                // para separar argumentos com obj.m<A, B>(...)
    // tokens entre '<' e '>': nomes dos tipos e restrições (confere se as classes/interfaces existem)
    void parseTypeParamTokens(const std::vector<Token>& inner, const std::string& what,
                              std::vector<std::string>& names, std::vector<GhostConstraint>& cons);
    // Pilha<int>, maior<Ponto>...: cada tipo respeita a restrição do seu T
    void checkConstraints(const std::vector<std::string>& params, const std::vector<GhostConstraint>& cons,
                          const std::vector<std::string>& args, const std::string& where);
    const GhostClassDef::GenericMethod* findGenericMethod(const std::string& cls, const std::string& name, std::string* owner) const;
    // self == nullptr: método static
    GhostValue callGenericMember(GhostValue* self, const std::string& owner, const std::string& name,
                                 const std::vector<std::string>& explicitArgs, std::vector<std::vector<Token>>& raw);
    // t[i] == '<': o '<...>' termina num '>' seguido de '(' (chamada com tipos explícitos)
    static bool genericCallAt(const std::vector<Token>& t, size_t i);
    // lê "<int,string>" (t[i] == '<') e devolve os tipos; i fica depois do '>'
    std::vector<std::string> typeArgsAt(const std::vector<Token>& t, size_t& i, const std::string& head);
    std::string instantiateFunction(const std::string& name, const std::vector<std::string>& args);
    // maior(3, 5): deduz T pelos argumentos (como o C++); maior<double>(3, 5): tipos explícitos
    GhostValue callGenericFunction(const std::string& name, const std::vector<std::string>& explicitArgs, std::vector<std::vector<Token>>& raw);
    void ensureInstance(const std::string& spec);
    // t[i] == '<' depois do nome genérico: lê até o '>' e devolve "Pilha<int>" (i fica depois do '>')
    std::string specFromTokens(const std::vector<Token>& t, size_t& i, const std::string& head);
    // corpo de uma classe (depois do '{'): campos, métodos, static, conferência de interfaces e statics
    void defineClass(GhostClassDef def, const std::string& body, const std::string& ctorName);
    std::vector<std::pair<std::string, const GhostFunction*>> ifaceMethodList(const std::string& iface) const;
    bool ifaceExtends(const std::string& a, const std::string& b) const;
    bool implementsIface(const std::string& cls, const std::string& iface) const;
    GhostValue coerceIface(const GhostValue& v, const std::string& iface, const std::string& what);
    // classe do método em execução (de objeto ou static); "" fora de métodos
    std::string currentClassCtx() const;
    const GhostFieldDef* findStatic(const std::string& cls, const std::string& name, std::string* owner) const;
    // campo static visível pelo nome dentro de métodos da classe (ou subclasses)
    GhostValue* staticRef(const std::string& name, const GhostFieldDef** info = nullptr);
    GhostValue callStatic(const std::string& owner, const GhostFunction& f, const std::string& name, std::vector<std::vector<Token>>& raw);
    std::map<std::string, GhostClassDef> class_memory;
    std::map<std::string, std::string> var_class;      // variável declarada com tipo de classe
    // Objeto do método em execução. cls = classe que definiu o método (super e private usam ela).
    struct ThisFrame { GhostValue obj; std::string cls; size_t frameDepth = 0; LPath path; };
    std::vector<ThisFrame> this_stack;
    LPath pending_method_path;
    int construct_depth = 0;
    void parseClassDef(Lexer& lex);
    bool inMethod() const;
    GhostValue* fieldRef(const std::string& name, const GhostFieldDef** info = nullptr);
    GhostValue* rootRef(const std::string& name);
    // quiet: devolve nullptr em vez de lançar erro quando o caminho não existe mais
    GhostValue* resolvePath(const LPath& p, bool createKey, bool quiet, const GhostFieldDef** lastField = nullptr, std::string* lastListElemSpec = nullptr);
    const GhostFieldDef* findField(const std::string& cls, const std::string& name, std::string* declaredIn = nullptr) const;
    const GhostFunction* findMethod(const std::string& cls, const std::string& name, std::string* implClass, bool* priv = nullptr, bool* prot = nullptr) const;
    std::vector<GhostFunction> findMethodOverloads(const std::string& cls, const std::string& name, std::string* implClass, bool* priv = nullptr, bool* prot = nullptr) const;
    const GhostFunction* findStaticMethod(const std::string& cls, const std::string& name, std::string* owner, bool* priv = nullptr, bool* prot = nullptr) const;
    std::vector<GhostFunction> findStaticMethodOverloads(const std::string& cls, const std::string& name, std::string* owner, bool* priv = nullptr, bool* prot = nullptr) const;
    const GhostFunction* selectOverload(const std::vector<GhostFunction>& overloads, const std::vector<GhostValue>& args, const std::string& name) const;
    bool isSubclass(const std::string& cls, const std::string& base) const;
    void checkAccess(const std::string& declaredIn, bool priv, const std::string& what) { checkAccess(declaredIn, priv, false, what); }
    void checkAccess(const std::string& declaredIn, bool priv, bool prot, const std::string& what);
    GhostValue readField(const GhostValue& obj, const std::string& field);
    GhostValue coerce(const GhostValue& v, GTokenType type, const std::string& cls, const std::string& what);
    GhostValue construct(const std::string& cls, std::vector<GhostValue> args);
    void initObject(GhostValue& obj, const std::string& cls, std::vector<GhostValue> args);
    void initFields(GhostValue& obj, const std::string& cls);
    std::vector<GhostValue> evalArgs(const GhostFunction& f, const std::string& name, std::vector<std::vector<Token>>& raw);
    GhostValue runFunction(const GhostFunction& f, const std::string& name, std::vector<GhostValue>& vals, GhostValue* self, const std::string& implClass);
    // obj.m(args): virtual = despacho pela classe real do objeto; senão começa em startClass (super.m)
    GhostValue callMethod(GhostValue& self, const std::string& startClass, const std::string& m, std::vector<std::vector<Token>>& raw, bool isVirtual);
    GhostValue callOnPath(const LPath& path, GhostValue self, const std::string& startClass, const std::string& m, std::vector<std::vector<Token>>& raw, bool isVirtual);
    // cls: nome de classe ou tipo "result<T>"
    bool tryClassVarDecl(const std::string& cls, const Token& name, Lexer& lex);

    // ---- result<T> no REPL (src/InterpreterResult.cpp) ----
    // Tipos guardados como texto ("result<double>", "result<list<int>>") no mesmo lugar do nome da classe.
    bool knownType(const std::string& spec) const;        // classe, interface, list<T>, result<T>, set<T> ou fn<...>
    // Lê "<T>" depois de "result"/"set" (ltTaken: o '<' já foi lido) e devolve "result<T>"/"set<T>"
    std::string readResultSpec(Lexer& lex, bool ltTaken, const std::string& head = "result");
    // set<T>: ordena e tira repetidos (mesma ordem do std::set do compilado para números e textos)
    void normalizeSet(GhostValue& s);
    // Compreensões de lista, conjunto e mapa com um ou vários 'for' e filtros 'if'
    GhostValue comprehension(std::vector<Token>& t, size_t start, size_t close, bool isBrace, bool eval);
    GhostValue coerceList(const GhostValue& v, const std::string& spec, const std::string& what);
    GhostValue coerceSet(const GhostValue& v, const std::string& spec, const std::string& what);
    GhostValue setMethod(const LPath* path, const GhostValue& self, const std::string& m, std::vector<GhostValue>& args);
    GhostValue coerceResult(const GhostValue& v, const std::string& spec, const std::string& what);
    GhostValue coerceSpec(const GhostValue& v, const std::string& spec, const std::string& what);
    GhostValue defaultOf(const std::string& spec);        // Classe nome;  result<T> nome;
    GhostValue resultMethod(const GhostValue& self, const std::string& m, std::vector<GhostValue>& args);
    // Tipo de retorno de cada função em execução ("" em construtores): o operador r? só vale em result<T>
    std::vector<std::string> ret_stack;
    struct RetGuard {
        std::vector<std::string>& s;
        RetGuard(std::vector<std::string>& v, const std::string& spec) : s(v) { s.push_back(spec); }
        ~RetGuard() { s.pop_back(); }
        RetGuard(const RetGuard&) = delete;
        RetGuard& operator=(const RetGuard&) = delete;
    };
    // r?: err sai da função atual com o mesmo erro; ok devolve o valor
    GhostValue tryOperator(const GhostValue& r);

    // ---- fn<Ret(Args)> e lambdas/closures no REPL (src/InterpreterResult.cpp e src/InterpreterCore.cpp) ----
    std::string validateAndNormalizeTypeSpec(const std::string& raw, bool allowVoid) const;
    std::string validateAndNormalizeFnSpec(const std::string& spec);
    bool canConvertFnReturn(const std::string& from, const std::string& to) const;
    GhostValue coerceFn(const GhostValue& v, const std::string& spec, const std::string& what);
    GhostValue makeNamedFnValue(const std::string& name, const GhostFunction& f) const;
    bool isLambdaAt(const std::vector<Token>& t, size_t i) const;
    bool isCaptureLambdaAt(const std::vector<Token>& t, size_t i) const;
    GhostValue evalLambdaAt(std::vector<Token>& t, size_t& i, bool eval);
    GhostValue invokeFnValue(const LPath* path, const GhostValue& fnVal, std::vector<std::vector<Token>>& rawArgs);
    GhostValue invokeFnValueDirect(const GhostValue& fnVal, const std::vector<GhostValue>& argVals, const LPath* path = nullptr);
    std::string inferLambdaRetSpec(const GhostClosure& c);
    std::string inferExprType(const std::vector<Token>& tokens, const std::map<std::string, std::string>& localTypes);
    bool opensTypeAngle(const Token& prev) const;

    // ---- qreg/qcircuit dentro de valores (campos de classe, qreg(n)) (src/InterpreterQuantum.cpp) ----
    int qvalue_counter = 0;
    GhostValue makeQuantum(GTokenType type, double qubits);
    // Copia o registrador do valor para qreg_memory/qcircuit_memory com um nome temporário
    std::string bindQuantum(const GhostValue& v);
    void unbindQuantum(const std::string& tmp, GhostValue* back); // back == nullptr: só descarta
    // v.metodo(args): t[i] é o '(' da chamada; path diz onde gravar o registrador alterado
    GhostValue quantumCall(const LPath* path, const GhostValue& v, const std::string& m, std::vector<Token>& t, size_t& i, bool eval);
    void syncQuantumArgs(const GhostFunction& f, const std::vector<std::vector<Token>>& raw, const std::vector<GhostValue>& vals);
    // obj.campo = e;  this.x += 1;  xs[i][j] = e;  obj.lista.push(1);  obj.metodo();
    bool tryLvalueStatement(const Token& first, const Token& next, Lexer& lex);

    // Funcoes internas
    void executeLine(const string& line);
    double evaluateMath(std::vector<Token>& tokens);
    bool evaluateCondition(std::vector<Token>& tokens);
    std::string evaluateStringExpression(std::vector<Token>& tokens);
    std::string evaluateFString(const std::string& fstr);

    
    void parseState(Lexer& lex);
    void parseGate(Lexer& lex);
    void parseShow(Lexer& lex);
    void parseLoad(Lexer& lex);
    void parseWhile(Lexer& lex);
    void parseDoWhile(Lexer& lex);
    void parseFor(Lexer& lex);
    void parseForeach(Lexer& lex);
    void foreachFromHead(std::vector<Token>& head, const std::string& body);
    void parseHypothesis(Lexer& lex);
    void parseSwitch(Lexer& lex);
    void parseFunction(Lexer& lex);
    void parseReturn(Lexer& lex);
    void printGate(const string& name);
    void parseActivate(Lexer& lex);
    void parseApply(Lexer& lex);
    void parseOptimize(const std::string& objName, Lexer& lex);
    void parseNetwork(Lexer& lex);
    void parseMathAssignment(const std::string& targetName, Lexer& lex);
    void parseMathAssignmentLeft(const std::string& targetName, const Token& leftToken, Lexer& lex);
    void parseVar(Lexer& lex, GTokenType type);
    void parsePrint(Lexer& lex);
    void parseIf(Lexer& lex);
    void parseSpawn(Lexer& lex);
    void parseSleep(Lexer& lex);
    void parseMutex(Lexer& lex);
    void parseLock(Lexer& lex);
    void parseUnlock(Lexer& lex);
    void parseServer(Lexer& lex);
    bool tryServerMethod(const std::string& objName, const Token& next, Lexer& lex);
    bool tryHybridStatement(const std::string& objName, const Token& next, Lexer& lex);

public:
    static bool isFnSpec(const std::string& spec);
    static bool parseFnSignature(const std::string& spec, std::string& ret, std::vector<std::string>& params);
    static bool isResultSpec(const std::string& spec);
    static bool isSetSpec(const std::string& spec);
    static bool isListSpec(const std::string& spec);
    void repl();
};

#endif
