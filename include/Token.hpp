#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

// Todos os tipos de "palavras" e "símbolos" que a linguagem Ghost entende
enum class GTokenType {
    // Palavras-chave (Keywords)
    KwState,    // state
    KwGate,     // gate
    KwOptimize, // optimize
    KwShow,     // show
    KwEpochs,   // epochs
    KwLr,       // lr
    KwOpt,      // opt (Otimizador Adam/RMSProp)

    // Literais e Identificadores
    Identifier, // Nomes de variaveis (ex: initial, vqc)
    Number,     // Numeros (ex: 1.0, 10, 0.1)

    // Keywords de Fluxo, Variaveis e I/O
    KwIf,       // if
    KwElse,     // else
    KwFor,      // for
    KwWhile,    // while
    KwDo,       // do
    KwForeach,  // foreach
    KwIn,       // in
    KwVar,      // var
    KwPrint,    // print
    KwInput,    // input
    KwFunction, // function
    KwReturn,   // return
    KwVoid,     // void
    
    // Switch Statements
    KwSwitch,   // switch
    KwCase,     // case
    KwDefault,  // default
    KwBreak,    // break
    
    // Multiverso (Linhas do Tempo Otimizadas)
    KwHypothesis, // hypothesis
    KwCollapseMax, // collapse_max
    KwCollapseMin, // collapse_min
    
    // Threads
    KwSpawn,    // spawn
    KwSleep,    // sleep
    KwMutex,    // mutex
    KwLock,     // lock
    KwUnlock,   // unlock
    
    // OOP (Orientacao a Objetos)
    KwClass,    // class
    KwPublic,   // public
    KwPrivate,  // private
    KwThis,     // this
    KwNew,      // new
    
    // Tipagem Forte e Primitivas
    KwString,   // string
    KwDate,     // date
    KwTime,     // time
    KwDatetime, // datetime
    KwInt,      // int
    KwDouble,   // double
    KwFloat,    // float
    KwComplex,  // complex
    KwGhost,    // ghost
    KwJson,     // json
    KwMatrix,   // matrix ou GhostMatrix
    KwServer,   // server
    KwRoute,    // route
    KwListen,   // listen
    KwTextModel,
    LogicalAnd, LogicalOr,
    KwLlm,      // llm

    // Visão Computacional e Hardware
    KwPragma,   // pragma
    KwGpu,      // gpu
    KwConv,     // conv

    // Simbolos e Operadores
    Assign,     // =
    Arrow,      // =>
    ThinArrow,  // ->
    Chain,      // >>
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /
    Power,      // ^
    LBracket,   // [
    RBracket,   // ]
    LParen,     // (
    RParen,     // )
    Comma,      // ,
    Semicolon,  // ;
    Colon,      // :
    Dot,        // .
    LBrace,     // {
    RBrace,     // }

    // Comparadores Logicos
    Eq,         // ==
    Neq,        // !=
    Lt,         // <
    Gt,         // >
    Leq,        // <=
    Geq,        // >=

    KwLoad,     // load
    KwLoadGguf, // load_gguf
    KwImport,   // import
    String,     // "texto"
    FString,    // f"texto" ou f`texto`
    KwActivate, // activate
    KwApply,    // apply
    KwRelu,     // relu
    KwSigmoid,  // sigmoid
    KwSin,      // sin
    KwCos,      // cos
    KwTan,      // tan
    KwTranspose,// transpose

    KwHadamard, // hadamard
    KwPauliX,   // paulix
    KwPauliY,   // pauliy
    KwPauliZ,   // pauliz

    // Adicione junto aos outros símbolos (Arrow, LBracket, etc)
    KwNetwork,
    KwSwarm,
    KwQreg,     // qreg (registrador quantico)
    KwQcircuit, // qcircuit (circuito parametrizado/treinavel)
    KwQlayer,   // qlayer (camada quantica para network >>)
    KwCircuit,  // circuit nome(params) { portas } (sub-circuito reutilizavel)

    // Nucleo da linguagem (tipos, controle de fluxo e erros)
    KwBool,     // bool
    KwContinue, // continue
    KwTry,      // try
    KwCatch,    // catch
    KwThrow,    // throw
    KwFinally,  // finally
    KwExtends,  // extends
    KwSuper,    // super
    KwList,     // list<T>
    KwMap,      // map<K, V>

    // Operadores (nucleo)
    Percent,      // %
    Not,          // !
    BitAnd,       // &
    BitOr,        // |
    Caret,        // ^  (XOR bit a bit; potencia e pow(x, y))
    Tilde,        // ~
    ShiftLeft,    // <<
    Question,     // ?
    PlusAssign,   // +=
    MinusAssign,  // -=
    StarAssign,   // *=
    SlashAssign,  // /=
    PercentAssign,// %=
    Increment,    // ++
    Decrement,    // --

    // Controle
    EndOfFile,  // Fim do arquivo
    Unknown     // Erro lexico
};

// Estrutura do Token
struct Token {
    GTokenType type;
    std::string value;
    int line;
    int column;
};

#endif