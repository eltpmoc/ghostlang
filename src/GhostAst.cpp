// GhostAst.cpp - parser da AST da GhostLang (c40, etapa 1). Veja include/GhostAst.hpp.
//
// Descida recursiva sobre a lista de tokens do Lexer (modo REPL: literais e f-strings no texto original).
// Precedência (docs/NUCLEO.md): ?: < || < && < | < ^ < & < == != < < <= > >= < << >> < + - < * / % < unários < **
// Recuperação: um erro descarta o resto do comando (até ';' ou o '}' do bloco) e a leitura continua;
// ';' faltando no fim da linha é anotado sem descartar nada.
#include "../include/GhostAst.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

#include "../include/GhostDiag.hpp"
#include "../include/GhostTryOp.hpp"
#include "../include/Lexer.hpp"

namespace {
using T = GTokenType;

struct SyntaxError {
    int line, col;
    std::string msg;
};

bool isTypeKeyword(T t) {
    switch (t) {
        case T::KwInt: case T::KwDouble: case T::KwFloat: case T::KwString: case T::KwBool: case T::KwVoid:
        case T::KwGhost: case T::KwComplex: case T::KwJson: case T::KwMatrix: case T::KwDate: case T::KwTime: case T::KwDatetime:
        case T::KwList: case T::KwMap: case T::KwQreg: case T::KwQcircuit: case T::KwQlayer: case T::KwTextModel:
        case T::KwLlm: case T::KwVar:
            return true;
        default:
            return false;
    }
}

// Palavras-chave que, dentro de expressões, são nomes comuns (sin(x), transpose(m), epochs=10, ...)
bool isKeywordName(T t) {
    switch (t) {
        case T::KwRelu: case T::KwSigmoid: case T::KwSin: case T::KwCos: case T::KwTan: case T::KwTranspose:
        case T::KwHadamard: case T::KwPauliX: case T::KwPauliY: case T::KwPauliZ: case T::KwOptimize:
        case T::KwEpochs: case T::KwLr: case T::KwOpt: case T::KwLoad: case T::KwLoadGguf: case T::KwShow:
        case T::KwConv: case T::KwGpu: case T::KwRoute: case T::KwListen: case T::KwInput: case T::KwApply:
        case T::KwActivate: case T::KwState: case T::KwGate: case T::KwJson:
            return true;
        default:
            return false;
    }
}

// Começo de comando (para a recuperação parar no comando seguinte)
bool isStatementKeyword(T t) {
    switch (t) {
        case T::KwIf: case T::KwFor: case T::KwWhile: case T::KwDo: case T::KwForeach: case T::KwReturn:
        case T::KwPrint: case T::KwFunction: case T::KwClass: case T::KwSwitch: case T::KwTry: case T::KwThrow:
        case T::KwBreak: case T::KwContinue: case T::KwCircuit: case T::KwImport:
            return true;
        default:
            return isTypeKeyword(t);
    }
}

int binaryPrec(T t) {
    switch (t) {
        case T::LogicalOr: return 2;
        case T::LogicalAnd: return 3;
        case T::BitOr: return 4;
        case T::Caret: return 5;
        case T::BitAnd: return 6;
        case T::Eq: case T::Neq: return 7;
        case T::Lt: case T::Gt: case T::Leq: case T::Geq: return 8;
        case T::ShiftLeft: case T::Chain: return 9;
        case T::Plus: case T::Minus: return 10;
        case T::Star: case T::Slash: case T::Percent: return 11;
        default: return 0;
    }
}

bool isAssignOp(T t) {
    return t == T::Assign || t == T::PlusAssign || t == T::MinusAssign || t == T::StarAssign || t == T::SlashAssign ||
           t == T::PercentAssign;
}

const char* punctName(T t) {
    switch (t) {
        case T::Semicolon: return ";";
        case T::LParen: return "(";
        case T::RParen: return ")";
        case T::LBrace: return "{";
        case T::RBrace: return "}";
        case T::LBracket: return "[";
        case T::RBracket: return "]";
        case T::Comma: return ",";
        case T::Colon: return ":";
        case T::Assign: return "=";
        case T::Arrow: return "=>";
        case T::Dot: return ".";
        case T::Lt: return "<";
        case T::Gt: return ">";
        case T::BitOr: return "|";
        case T::LogicalOr: return "||";
        case T::Identifier: return "um nome";
        case T::KwIn: return "in";
        case T::KwWhile: return "while";
        default: return "";
    }
}

// Texto do token como o programador escreveu (this vem do lexer como "(*this)")
std::string shown(const Token& t) {
    switch (t.type) {
        case T::KwThis: return "this";
        case T::KwNew: return "new";
        case T::String: return "\"" + t.value + "\"";
        case T::FString: return "f\"" + t.value + "\"";
        default: return t.value;
    }
}

bool identLike(const Token& t) {
    return !t.value.empty() && (std::isalpha(static_cast<unsigned char>(t.value[0])) || t.value[0] == '_') &&
           t.type != T::String && t.type != T::FString && t.type != T::Number;
}

class AstParser {
public:
    AstParser(const std::string& src, const std::set<std::string>* knownGenerics, int lineBase, int colBase) {
        Lexer lex(src);
        try {
            for (;;) {
                Token t = lex.getNextToken();
                if (lineBase > 0) {
                    t.line = lineBase + t.line - 1;
                    t.column = colBase;
                }
                toks.push_back(t);
                if (t.type == T::EndOfFile) break;
            }
        } catch (const std::exception& e) {
            const int l = toks.empty() ? 1 : toks.back().line;
            record(l, 0, e.what());
            toks.push_back(Token{T::EndOfFile, "EOF", l, 0});
        }
        normalizeTokens();
        if (knownGenerics) generics = *knownGenerics;
        else collectGenerics();
    }

    AstParseResult run() {
        AstParseResult r;
        r.program = parseProgram();
        r.errors = std::move(errors);
        return r;
    }

    // ${expr} ou ${expr:formato} de uma f-string
    AstPtr parseEmbedded(std::string& spec, const std::string& raw) {
        AstPtr e;
        try {
            e = parseExpr();
            if (at(T::Colon)) {
                // o formato é o texto cru depois do ':' (os tokens estão todos na coluna da f-string)
                size_t depth = 0, k = 0;
                for (; k < raw.size(); ++k) {
                    const char c = raw[k];
                    if (c == '(' || c == '[' || c == '{') depth++;
                    else if ((c == ')' || c == ']' || c == '}') && depth) depth--;
                    else if (c == ':' && depth == 0) break;
                }
                spec = k < raw.size() ? raw.substr(k + 1) : std::string();
            } else if (!at(T::EndOfFile)) {
                fail(cur(), "esperado '}' no fim de ${...}, encontrado " + found());
            }
        } catch (const SyntaxError& err) {
            record(err.line, err.col, err.msg);
        }
        return e;
    }

    std::vector<AstDiagnostic> errors;

private:
    std::vector<Token> toks;
    size_t pos = 0;
    int pendingGt = 0;            // '>>' fechando dois tipos genéricos
    bool eofReported = false;
    std::set<int> errLines;
    std::set<std::string> generics;  // classes, interfaces, funções e métodos genéricos declarados

    // ------------------------------------------------------------ tokens
    const Token& cur() const { return toks[pos]; }
    const Token& peek(size_t k = 1) const { return toks[std::min(pos + k, toks.size() - 1)]; }
    const Token& prev() const { return toks[pos ? pos - 1 : 0]; }
    bool at(T t) const { return toks[pos].type == t; }
    void advance() {
        if (toks[pos].type != T::EndOfFile) ++pos;
    }
    bool accept(T t) {
        if (!at(t)) return false;
        advance();
        return true;
    }
    std::string found() const {
        return at(T::EndOfFile) ? std::string("o fim do arquivo") : "'" + shown(cur()) + "'";
    }
    [[noreturn]] void fail(const Token& t, const std::string& msg) const { throw SyntaxError{t.line, t.column, msg}; }
    AstPtr node(AstKind k, const Token& t) const { return std::make_unique<AstNode>(k, t.line, t.column); }

    void record(int line, int col, const std::string& msg) {
        if (errLines.count(line) || errors.size() >= 50) return;
        errLines.insert(line);
        errors.push_back({line, col, msg});
    }
    void record(const SyntaxError& e) { record(e.line, e.col, e.msg); }

    // "a -1": o lexer lê -1 como número; depois de um operando é a subtração.
    // 2.0i / 3.5g / 5.0theta: o modo REPL do lexer deixa o sufixo como nome colado ao número.
    void normalizeTokens() {
        std::vector<Token> out;
        out.reserve(toks.size());
        for (size_t i = 0; i < toks.size(); ++i) {
            Token t = toks[i];
            if (t.type == T::Number && i + 1 < toks.size()) {
                const Token& n = toks[i + 1];
                if (n.type == T::Identifier && n.line == t.line && n.column == t.column + static_cast<int>(t.value.size()) &&
                    (n.value == "i" || n.value == "g" || n.value == "theta")) {
                    t.value += n.value;
                    ++i;
                }
            }
            if (t.type == T::Number && t.value.size() > 1 && t.value[0] == '-' && !out.empty()) {
                const T p = out.back().type;
                if (p == T::Identifier || p == T::Number || p == T::String || p == T::FString || p == T::RParen ||
                    p == T::RBracket || p == T::KwThis || p == T::Increment || p == T::Decrement || isKeywordName(p)) {
                    out.push_back(Token{T::Minus, "-", t.line, t.column});
                    t.value.erase(0, 1);
                    t.column += 1;
                }
            }
            out.push_back(t);
        }
        toks.swap(out);
    }

    // índice logo depois do '>' que fecha o '<' em i (ou 0 se não fecha)
    size_t closeAngleAt(size_t i) const {
        int depth = 0;
        for (size_t k = i; k < toks.size(); ++k) {
            const T ty = toks[k].type;
            if (ty == T::Lt) depth++;
            else if (ty == T::Gt) { if (--depth == 0) return k + 1; }
            else if (ty == T::Chain) { depth -= 2; if (depth <= 0) return k + 1; }
            else if (ty == T::Semicolon || ty == T::LBrace || ty == T::RBrace || ty == T::EndOfFile) return 0;
        }
        return 0;
    }

    void collectGenerics() {
        for (size_t i = 0; i + 2 < toks.size(); ++i) {
            const Token& t = toks[i];
            if (t.type == T::KwClass && toks[i + 1].type == T::Identifier && toks[i + 2].type == T::Lt)
                generics.insert(toks[i + 1].value);
            if (t.type == T::Identifier && t.value == "interface" && toks[i + 1].type == T::Identifier && toks[i + 2].type == T::Lt)
                generics.insert(toks[i + 1].value);
            if (t.type != T::KwFunction) continue;
            for (size_t k = i + 1; k < toks.size(); ++k) {
                const T ty = toks[k].type;
                if (ty == T::LParen || ty == T::LBrace || ty == T::Semicolon || ty == T::EndOfFile) break;
                if (ty == T::Identifier && k + 1 < toks.size() && toks[k + 1].type == T::Lt) {
                    const size_t after = closeAngleAt(k + 1);
                    if (after == 0) break;
                    if (after < toks.size() && toks[after].type == T::LParen) {
                        generics.insert(toks[k].value);
                        break;
                    }
                    k = after - 1;
                }
            }
        }
    }

    // ------------------------------------------------------------ esperar tokens
    void expect(T t) {
        if (accept(t)) return;
        const std::string want = punctName(t);
        if (!want.empty()) fail(cur(), "esperado '" + want + "', encontrado " + found());
        fail(cur(), "token inesperado " + found());
    }
    // ';' faltando no fim da linha: anota e segue (não descarta o comando seguinte)
    void expectSemi() {
        if (accept(T::Semicolon)) return;
        const Token& p = prev();
        if (pos > 0 && cur().line > p.line) {
            record(p.line, p.column + static_cast<int>(shown(p).size()), "faltou ';' no fim da linha (depois de '" + shown(p) + "')");
            return;
        }
        fail(cur(), "esperado ';', encontrado " + found());
    }
    void expectClose(T close, const Token& open) {
        if (accept(close)) return;
        const std::string cs = punctName(close);
        if (at(T::Semicolon) || at(T::EndOfFile) || (close != T::RBrace && at(T::RBrace)) ||
            (cur().line > open.line && cur().line > prev().line))
            fail(open, "faltou '" + cs + "' para fechar o '" + open.value + "' aberto aqui");
        fail(cur(), "esperado '" + cs + "', encontrado " + found());
    }

    // Pula o resto do comando com erro: até ';' (consumido), o '}' do bloco (não consumido),
    // um bloco { } inteiro, ou um comando que começa numa linha seguinte.
    void synchronize() {
        int depth = 0;
        const int errLine = cur().line;
        bool moved = false;
        while (!at(T::EndOfFile)) {
            const T t = cur().type;
            if (t == T::LBrace) {
                depth++;
            } else if (t == T::RBrace) {
                if (depth == 0) return;
                advance();
                if (--depth == 0) return;
                continue;
            } else if (t == T::Semicolon && depth == 0) {
                advance();
                return;
            } else if (depth == 0 && moved && cur().line > errLine && isStatementKeyword(t)) {
                return;
            }
            advance();
            moved = true;
        }
    }

    // ------------------------------------------------------------ tipos
    void closeAngle() {
        if (pendingGt > 0) { pendingGt--; return; }
        if (accept(T::Gt)) return;
        if (at(T::Chain)) { advance(); pendingGt = 1; return; }
        fail(cur(), "esperado '>' no tipo generico, encontrado " + found());
    }

    AstPtr parseType() {
        const Token t = cur();
        auto n = node(AstKind::Type, t);
        if (isTypeKeyword(t.type) || t.type == T::Identifier) {
            n->text = t.value;
            advance();
        } else {
            fail(t, "tipo esperado, encontrado " + found());
        }
        if (pendingGt == 0 && at(T::Lt) && t.type != T::KwVar) {
            advance();
            if (n->text == "fn") { // fn<retorno(parametros)>
                n->add(parseType());
                const Token open = cur();
                expect(T::LParen);
                while (!at(T::RParen) && !at(T::EndOfFile)) {
                    n->add(parseType());
                    if (!accept(T::Comma)) break;
                }
                expectClose(T::RParen, open);
            } else {
                do {
                    n->add(parseType());
                } while (pendingGt == 0 && accept(T::Comma));
            }
            closeAngle();
        }
        while (pendingGt == 0 && at(T::LBracket) && peek().type == T::RBracket) { // int[] -> list<int>
            const Token b = cur();
            advance();
            advance();
            auto l = node(AstKind::Type, b);
            l->text = "list";
            l->add(std::move(n));
            n = std::move(l);
        }
        return n;
    }

    // Tipo seguido de nome (declaração), sem consumir nada
    bool typeThenIdent() {
        if (!isTypeKeyword(cur().type) && !at(T::Identifier)) return false;
        const size_t save = pos;
        const int saveGt = pendingGt;
        bool ok = false;
        try {
            parseType();
            ok = pendingGt == 0 && at(T::Identifier);
        } catch (const SyntaxError&) {
        }
        pos = save;
        pendingGt = saveGt;
        return ok;
    }

    // Nome<tipos> numa expressão (classe, interface ou função genérica): confere sem consumir
    bool typeArgsAhead(bool needCall) {
        if (!at(T::Lt)) return false;
        const size_t save = pos;
        const int saveGt = pendingGt;
        bool ok = false;
        try {
            advance();
            do {
                parseType();
            } while (pendingGt == 0 && accept(T::Comma));
            closeAngle();
            ok = pendingGt == 0 && (!needCall || at(T::LParen) || at(T::Dot));
        } catch (const SyntaxError&) {
        }
        pos = save;
        pendingGt = saveGt;
        return ok;
    }

    void parseTypeArgsInto(AstNode* n) {
        advance(); // '<'
        do {
            n->add(parseType());
        } while (pendingGt == 0 && accept(T::Comma));
        closeAngle();
    }

    void parseTypeParams(AstNode* owner) {
        advance(); // '<'
        do {
            if (!at(T::Identifier)) fail(cur(), "<...>: use nomes de tipo, ex.: <T>");
            auto tp = node(AstKind::TypeParam, cur());
            tp->name = cur().value;
            advance();
            if (at(T::KwExtends) || (at(T::Identifier) && cur().value == "implements")) {
                tp->attr = at(T::KwExtends) ? "extends" : "implements";
                advance();
                tp->add(parseType());
                while (pendingGt == 0 && accept(T::BitAnd)) tp->add(parseType());
            }
            owner->add(std::move(tp));
        } while (pendingGt == 0 && accept(T::Comma));
        closeAngle();
    }

    // ------------------------------------------------------------ programa e declarações
    AstPtr parseProgram() {
        auto p = node(AstKind::Program, cur());
        while (!at(T::EndOfFile)) {
            const size_t before = pos;
            try {
                if (at(T::RBrace)) fail(cur(), "'}' sem '{' correspondente");
                auto d = parseTopLevel();
                if (d) p->add(std::move(d));
            } catch (const SyntaxError& e) {
                record(e);
                synchronize();
                if (pos == before) advance();
            }
        }
        return p;
    }

    AstPtr parseTopLevel() {
        switch (cur().type) {
            case T::KwClass: return parseClass();
            case T::KwFunction: return parseFunction("", "");
            case T::KwCircuit: return parseCircuit();
            case T::KwImport: return parseImport();
            default: break;
        }
        if (at(T::Identifier) && cur().value == "interface" && peek().type == T::Identifier) return parseInterface();
        return parseStatement();
    }

    AstPtr parseImport() {
        auto n = node(AstKind::Import, cur());
        advance();
        if (!at(T::String)) fail(cur(), "use import \"arquivo.g\"; ou import \"arquivo.g\" as nome;");
        n->text = cur().value;
        advance();
        if (at(T::Identifier) && cur().value == "as") {
            advance();
            if (!at(T::Identifier)) fail(cur(), "import ... as: esperado um nome depois de as");
            n->name = cur().value;
            advance();
        }
        expectSemi();
        return n;
    }

    static std::string joinAttr(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        return a + " " + b;
    }

    AstPtr parseFunction(const std::string& attr, const std::string& className) {
        const Token kw = cur();
        advance(); // function
        auto f = node(AstKind::FuncDecl, kw);
        f->attr = attr;
        const bool isCtor = !className.empty() && at(T::Identifier) && cur().value == className && peek().type == T::LParen;
        if (isCtor) {
            auto v = node(AstKind::Type, cur());
            v->text = "void";
            f->add(std::move(v));
            f->attr = joinAttr(f->attr, "ctor");
        } else if (typeThenIdent() || (isTypeKeyword(cur().type) && identLike(peek()))) {
            f->add(parseType());
        } else {
            auto v = node(AstKind::Type, cur());
            v->text = "void";
            f->add(std::move(v));
        }
        if (!at(T::Identifier) && !(identLike(cur()) && peek().type == T::LParen))
            fail(cur(), "esperado o nome da funcao, encontrado " + found());
        f->name = cur().value;
        if (!className.empty() && f->name == className && !f->has("ctor")) {
            f->attr = joinAttr(f->attr, "ctor");
        }
        advance();
        if (at(T::Lt)) parseTypeParams(f.get());
        const Token open = cur();
        expect(T::LParen);
        while (!at(T::RParen) && !at(T::EndOfFile)) {
            if (!typeThenIdent())
                fail(cur(), "parametro " + found() + " da funcao " + f->name + " precisa de tipo (ex.: int x)");
            auto p = node(AstKind::Param, cur());
            p->add(parseType());
            p->name = cur().value;
            advance();
            if (at(T::LBracket) && peek().type == T::RBracket) { // string args[]
                advance();
                advance();
                auto l = node(AstKind::Type, prev());
                l->text = "list";
                l->add(std::move(p->kids[0]));
                p->kids[0] = std::move(l);
            }
            if (accept(T::Assign)) p->add(parseExpr());
            f->add(std::move(p));
            if (!accept(T::Comma)) break;
        }
        expectClose(T::RParen, open);
        if (accept(T::Semicolon)) {
            f->attr = joinAttr(f->attr, "abstract");
        } else {
            f->add(parseBlock());
        }
        return f;
    }

    AstPtr parseClass() {
        const Token kw = cur();
        advance(); // class
        auto c = node(AstKind::ClassDecl, kw);
        if (!at(T::Identifier)) fail(cur(), "esperado o nome da classe, encontrado " + found());
        c->name = cur().value;
        advance();
        if (at(T::Lt)) parseTypeParams(c.get());
        if (accept(T::KwExtends)) {
            auto b = parseType();
            b->attr = "extends";
            c->add(std::move(b));
        }
        if (at(T::Identifier) && cur().value == "implements") {
            advance();
            do {
                auto i = parseType();
                i->attr = "implements";
                c->add(std::move(i));
            } while (accept(T::Comma));
        }
        const Token open = cur();
        expect(T::LBrace);
        std::string vis;
        while (!at(T::RBrace)) {
            if (at(T::EndOfFile)) {
                unclosed(open, "classe " + c->name);
                return c;
            }
            const size_t before = pos;
            try {
                const bool visKw = at(T::KwPublic) || at(T::KwPrivate) || (at(T::Identifier) && cur().value == "protected");
                if (visKw && peek().type == T::Colon) { // public: / private: / protected: (o gerador escreve a seção)
                    vis = cur().value;
                    auto sec = node(AstKind::Empty, cur());
                    sec->text = "section";
                    sec->name = vis;
                    c->add(std::move(sec));
                    advance();
                    advance();
                    continue;
                }
                std::string attr = vis;
                if (visKw) {
                    attr = cur().value;
                    advance();
                }
                if (at(T::Identifier) && cur().value == "static") {
                    attr = joinAttr(attr, "static");
                    advance();
                }
                if (at(T::KwFunction)) c->add(parseFunction(attr, c->name));
                else if (at(T::KwQreg) || at(T::KwQcircuit) || at(T::KwQlayer) || at(T::KwMatrix)) c->add(parseDomain());
                else if (typeThenIdent()) c->add(parseVarDecl(attr, true));
                else fail(cur(), found() + " nao vale dentro da classe " + c->name);
            } catch (const SyntaxError& e) {
                record(e);
                synchronize();
                if (pos == before) advance();
            }
        }
        advance(); // '}'
        return c;
    }

    AstPtr parseInterface() {
        const Token kw = cur();
        advance(); // interface
        auto n = node(AstKind::InterfaceDecl, kw);
        n->name = cur().value;
        advance();
        if (at(T::Lt)) parseTypeParams(n.get());
        if (accept(T::KwExtends)) {
            do {
                auto b = parseType();
                b->attr = "extends";
                n->add(std::move(b));
            } while (accept(T::Comma));
        }
        const Token open = cur();
        expect(T::LBrace);
        while (!at(T::RBrace)) {
            if (at(T::EndOfFile)) {
                unclosed(open, "interface " + n->name);
                return n;
            }
            const size_t before = pos;
            try {
                if (!at(T::KwFunction)) fail(cur(), "interface " + n->name + ": so declara metodos (function tipo nome(parametros);)");
                n->add(parseFunction("", ""));
            } catch (const SyntaxError& e) {
                record(e);
                synchronize();
                if (pos == before) advance();
            }
        }
        advance();
        return n;
    }

    AstPtr parseCircuit() {
        const Token kw = cur();
        advance(); // circuit
        auto n = node(AstKind::CircuitDecl, kw);
        if (!at(T::Identifier)) fail(cur(), "esperado o nome do circuit, encontrado " + found());
        n->name = cur().value;
        advance();
        const Token open = cur();
        expect(T::LParen);
        while (!at(T::RParen) && !at(T::EndOfFile)) {
            auto p = node(AstKind::Param, cur());
            auto ty = node(AstKind::Type, cur());
            ty->text = "var";
            if (at(T::KwInt) || at(T::KwDouble)) {
                ty->text = cur().value;
                advance();
            }
            p->add(std::move(ty));
            if (!at(T::Identifier)) fail(cur(), "parametro invalido em circuit " + n->name + " (use nome, int nome ou double nome).");
            p->name = cur().value;
            advance();
            n->add(std::move(p));
            if (!accept(T::Comma)) break;
        }
        expectClose(T::RParen, open);
        n->add(parseBlock());
        return n;
    }

    // ------------------------------------------------------------ comandos
    void unclosed(const Token& open, const std::string& what) {
        if (eofReported) return;
        eofReported = true;
        record(open.line, open.column, "faltou '}' para fechar o '{' " + what + " aberto aqui");
    }

    AstPtr parseBlock() {
        const Token open = cur();
        if (!at(T::LBrace)) fail(cur(), "esperado '{', encontrado " + found());
        advance();
        auto b = node(AstKind::Block, open);
        parseStatementsInto(b.get(), open, false);
        return b;
    }

    void parseStatementsInto(AstNode* b, const Token& open, bool stopAtCase) {
        for (;;) {
            if (at(T::RBrace)) {
                b->endLine = cur().line;
                if (!stopAtCase) advance();
                return;
            }
            if (at(T::EndOfFile)) {
                unclosed(open, "do bloco");
                return;
            }
            if (stopAtCase && (at(T::KwCase) || at(T::KwDefault))) return;
            const size_t before = pos;
            try {
                auto s = parseStatement();
                if (s) b->add(std::move(s)); // ';' sozinho fica (o gerador marca a linha dele)
            } catch (const SyntaxError& e) {
                record(e);
                synchronize();
                if (pos == before) advance();
            }
        }
    }

    bool canStartExpr(T t) const {
        switch (t) {
            case T::Identifier: case T::Number: case T::String: case T::FString: case T::LParen: case T::LBracket:
            case T::KwThis: case T::KwSuper: case T::KwNew: case T::Minus: case T::Plus: case T::Not: case T::Tilde:
            case T::Increment: case T::Decrement:
                return true;
            default:
                return isKeywordName(t) || isTypeKeyword(t);
        }
    }

    AstPtr parseStatement() {
        const Token t = cur();
        switch (t.type) {
            case T::Semicolon: advance(); return node(AstKind::Empty, t);
            case T::KwIf: return parseIf();
            case T::KwWhile: {
                advance();
                auto n = node(AstKind::While, t);
                n->add(parseCondition());
                n->add(parseBlock());
                return n;
            }
            case T::KwDo: {
                advance();
                auto n = node(AstKind::DoWhile, t);
                n->add(parseBlock());
                if (!at(T::KwWhile)) fail(cur(), "do { ... } precisa de while (condicao); no fim, encontrado " + found());
                advance();
                n->add(parseCondition());
                accept(T::Semicolon);
                return n;
            }
            case T::KwFor: return parseFor();
            case T::KwForeach: {
                advance();
                const Token open = cur();
                expect(T::LParen);
                return parseForeachRest(t, open);
            }
            case T::KwSwitch: return parseSwitch();
            case T::KwTry: return parseTry();
            case T::KwThrow: case T::KwReturn: {
                advance();
                auto n = node(t.type == T::KwThrow ? AstKind::Throw : AstKind::Return, t);
                if (!at(T::Semicolon) && !at(T::RBrace) && cur().line == t.line) n->add(parseExpr());
                expectSemi();
                return n;
            }
            case T::KwBreak: case T::KwContinue: {
                advance();
                auto n = node(t.type == T::KwBreak ? AstKind::Break : AstKind::Continue, t);
                expectSemi();
                return n;
            }
            case T::KwPrint: {
                auto n = parsePrint();
                expectSemi();
                return n;
            }
            case T::KwSpawn: {
                advance();
                auto n = node(AstKind::Spawn, t);
                n->add(parseBlock());
                return n;
            }
            case T::KwHypothesis: {
                advance();
                auto n = node(AstKind::Hypothesis, t);
                const Token open = cur();
                expect(T::LParen);
                if (typeThenIdent()) n->add(parseType());
                else {
                    auto v = node(AstKind::Type, cur());
                    v->text = "var";
                    n->add(std::move(v));
                }
                if (!at(T::Identifier)) fail(cur(), "hypothesis: use hypothesis (var x in valores) { ... collapse_max(score); }");
                n->name = cur().value;
                advance();
                if (!accept(T::KwIn)) fail(cur(), "hypothesis: faltou 'in'");
                n->add(parseExpr());
                expectClose(T::RParen, open);
                n->add(parseBlock());
                return n;
            }
            case T::KwCollapseMax: case T::KwCollapseMin: {
                advance();
                auto n = node(AstKind::Collapse, t);
                n->text = t.type == T::KwCollapseMax ? "max" : "min";
                n->add(parseCondition());
                expectSemi();
                return n;
            }
            case T::KwState: case T::KwGate: case T::KwQreg: case T::KwQcircuit: case T::KwQlayer: case T::KwLoad:
            case T::KwShow: case T::KwActivate: case T::KwApply: case T::KwNetwork: case T::KwPragma: case T::KwSwarm:
            case T::KwSleep: case T::KwMutex: case T::KwLock: case T::KwUnlock: case T::KwServer: case T::KwMatrix:
                if (t.type == T::KwLoad && peek().type == T::Dot) break; // load.x(...) seria expressão
                return parseDomain();
            case T::KwElse: fail(t, "'else' sem 'if' correspondente");
            case T::KwCase: case T::KwDefault: fail(t, "'" + t.value + "' fora de switch");
            case T::KwFunction: fail(t, "function so pode ser declarada no nivel principal do arquivo ou dentro de uma classe");
            case T::KwClass: fail(t, "class so pode ser declarada no nivel principal do arquivo");
            case T::KwCircuit: fail(t, "circuit deve ser declarado no nivel principal do arquivo");
            default: break;
        }
        if (typeThenIdent() || (isTypeKeyword(t.type) && peek().type != T::LParen && peek().type != T::Lt))
            return parseVarDecl("", false); // int 10 = x;  ->  erro no nome da variável
        if (t.type == T::Identifier && peek().type == T::Arrow) { // rede => load("x"); m => optimize(...);
            auto n = node(AstKind::Domain, t);
            n->text = "=>";
            n->name = t.value;
            advance();
            advance();
            n->add(parseExpr());
            expectSemi();
            return n;
        }
        if (canStartExpr(t.type)) {
            auto s = node(AstKind::ExprStmt, t);
            s->add(parseExpr());
            expectSemi();
            return s;
        }
        fail(t, "comando inesperado " + found());
    }

    AstPtr parseVarDecl(const std::string& attr, bool field) {
        (void)field;
        auto d = node(AstKind::VarDecl, cur());
        d->attr = attr;
        d->add(parseType());
        if (!at(T::Identifier)) fail(cur(), "esperado o nome da variavel depois do tipo, encontrado " + found());
        d->name = cur().value;
        advance();
        if (accept(T::Assign)) d->add(parseExpr());
        expectSemi();
        return d;
    }

    AstPtr parseCondition() {
        const Token open = cur();
        expect(T::LParen);
        auto c = parseExpr();
        expectClose(T::RParen, open);
        return c;
    }

    AstPtr parseIf() {
        const Token t = cur();
        advance(); // if
        auto n = node(AstKind::If, t);
        n->add(parseCondition());
        n->add(parseBlock());
        if (accept(T::KwElse)) {
            if (at(T::KwIf)) n->add(parseIf());
            else n->add(parseBlock());
        }
        return n;
    }

    bool foreachAhead() {
        const size_t save = pos;
        const int saveGt = pendingGt;
        bool ok = false;
        try {
            if (at(T::KwVar)) advance();
            else if (typeThenIdent()) parseType();
            if (at(T::Identifier)) {
                advance();
                if (accept(T::Comma)) {
                    if (at(T::Identifier)) {
                        advance();
                        ok = at(T::KwIn);
                    }
                } else {
                    ok = at(T::KwIn);
                }
            }
        } catch (const SyntaxError&) {
        }
        pos = save;
        pendingGt = saveGt;
        return ok;
    }

    AstPtr parseForeachRest(const Token& kw, const Token& open) {
        auto n = node(AstKind::Foreach, kw);
        if (at(T::KwVar) || typeThenIdent()) n->add(parseType());
        else {
            auto v = node(AstKind::Type, cur());
            v->text = "var";
            n->add(std::move(v));
        }
        if (!at(T::Identifier)) fail(cur(), "foreach: use foreach (x in colecao) { ... }");
        n->name = cur().value;
        advance();
        if (accept(T::Comma)) {
            if (!at(T::Identifier)) fail(cur(), "foreach (chave, valor in mapa)");
            n->attr = cur().value;
            advance();
        }
        if (!accept(T::KwIn)) fail(cur(), "foreach: faltou 'in' (foreach (x in colecao))");
        n->add(parseExpr());
        expectClose(T::RParen, open);
        n->add(parseBlock());
        return n;
    }

    AstPtr parseFor() {
        const Token kw = cur();
        advance(); // for
        // Forma do Python, sem parênteses: for x in range(10) { ... } / for chave, valor in mapa { ... }
        if (at(T::Identifier) && foreachAhead()) {
            auto n = node(AstKind::Foreach, kw);
            auto v = node(AstKind::Type, cur());
            v->text = "var";
            n->add(std::move(v));
            n->name = cur().value;
            advance();
            if (accept(T::Comma)) {
                if (!at(T::Identifier)) fail(cur(), "for chave, valor in mapa { ... }");
                n->attr = cur().value;
                advance();
            }
            if (!accept(T::KwIn)) fail(cur(), "for: faltou 'in' (for x in colecao { ... })");
            n->add(parseExpr());
            if (!at(T::LBrace)) fail(cur(), "for x in colecao: faltou '{' (o corpo do laco vai entre chaves)");
            n->add(parseBlock());
            return n;
        }
        const Token open = cur();
        expect(T::LParen);
        if (foreachAhead()) return parseForeachRest(kw, open);
        auto n = node(AstKind::For, kw);
        if (at(T::Semicolon)) {
            n->add(node(AstKind::Empty, cur()));
        } else if (typeThenIdent()) {
            auto d = node(AstKind::VarDecl, cur());
            d->add(parseType());
            d->name = cur().value;
            advance();
            if (accept(T::Assign)) d->add(parseExpr());
            n->add(std::move(d));
        } else {
            auto s = node(AstKind::ExprStmt, cur());
            s->add(parseExpr());
            n->add(std::move(s));
        }
        expect(T::Semicolon);
        if (at(T::Semicolon)) n->add(node(AstKind::Empty, cur()));
        else n->add(parseExpr());
        expect(T::Semicolon);
        if (at(T::RParen)) n->add(node(AstKind::Empty, cur()));
        else n->add(parseExpr());
        expectClose(T::RParen, open);
        n->add(parseBlock());
        return n;
    }

    AstPtr parseSwitch() {
        const Token kw = cur();
        advance(); // switch
        auto n = node(AstKind::Switch, kw);
        n->add(parseCondition());
        const Token open = cur();
        expect(T::LBrace);
        while (at(T::KwCase) || at(T::KwDefault)) {
            const Token ct = cur();
            auto c = node(AstKind::Case, ct);
            advance();
            if (ct.type == T::KwDefault) {
                c->attr = "default";
            } else {
                do {
                    c->add(parseTernary());
                } while (accept(T::Comma));
            }
            expect(T::Colon);
            auto body = node(AstKind::Block, ct);
            parseStatementsInto(body.get(), open, true);
            c->add(std::move(body));
            n->add(std::move(c));
        }
        if (at(T::EndOfFile)) {
            unclosed(open, "do switch");
            return n;
        }
        if (!accept(T::RBrace)) fail(cur(), "switch: esperado 'case', 'default' ou '}', encontrado " + found());
        return n;
    }

    AstPtr parseTry() {
        const Token kw = cur();
        advance(); // try
        auto n = node(AstKind::Try, kw);
        n->add(parseBlock());
        if (at(T::KwCatch)) {
            auto c = node(AstKind::Catch, cur());
            advance();
            if (at(T::LParen)) {
                const Token open = cur();
                advance();
                if (at(T::KwString) || at(T::KwVar)) advance();
                if (at(T::Identifier)) {
                    c->name = cur().value;
                    advance();
                }
                expectClose(T::RParen, open);
            }
            c->add(parseBlock());
            n->add(std::move(c));
        }
        if (at(T::KwFinally)) {
            auto f = node(AstKind::Finally, cur());
            advance();
            f->add(parseBlock());
            n->add(std::move(f));
        }
        return n;
    }

    AstPtr parsePrint() {
        const Token kw = cur();
        advance(); // print
        auto n = node(AstKind::Print, kw);
        const Token open = cur();
        expect(T::LParen);
        while (!at(T::RParen) && !at(T::EndOfFile) && !at(T::Semicolon)) {
            n->add(parseExpr());
            if (!accept(T::Comma)) break;
        }
        expectClose(T::RParen, open);
        return n;
    }

    // state s = [..];  gate H = hadamard;  qreg q(2);  network n = a >> b;  load "x";  show(m);  pragma gpu ...
    AstPtr parseDomain() {
        const Token kw = cur();
        advance();
        auto d = node(AstKind::Domain, kw);
        d->text = kw.value;
        switch (kw.type) {
            case T::KwState: case T::KwGate: case T::KwQreg: case T::KwQcircuit: case T::KwQlayer: case T::KwNetwork:
            case T::KwMutex: case T::KwServer: case T::KwMatrix:
                if (!at(T::Identifier)) fail(cur(), "esperado o nome depois de " + kw.value + ", encontrado " + found());
                d->name = cur().value;
                advance();
                break;
            default: break;
        }
        if (at(T::LParen)) {
            const Token open = cur();
            advance();
            while (!at(T::RParen) && !at(T::EndOfFile) && !at(T::Semicolon)) {
                d->add(parseExpr());
                if (!accept(T::Comma)) break;
            }
            expectClose(T::RParen, open);
        }
        if (accept(T::Assign)) {
            d->add(parseExpr());
            d->attr = "=";
        } else if (!at(T::Semicolon) && canStartExpr(cur().type) && cur().line == kw.line) {
            d->add(parseExpr()); // load "arquivo";  pragma gpu
        }
        if (kw.type == T::KwPragma) accept(T::Semicolon);
        else expectSemi();
        return d;
    }

    // ------------------------------------------------------------ expressões
    AstPtr parseExpr() {
        auto lhs = parseTernary();
        if (isAssignOp(cur().type)) {
            const Token op = cur();
            advance();
            auto a = node(AstKind::Assign, op);
            a->text = op.value;
            a->add(std::move(lhs));
            a->add(parseExpr());
            return a;
        }
        return lhs;
    }

    AstPtr parseTernary() {
        auto c = parseBinary(1);
        if (at(T::Question)) {
            const Token q = cur();
            advance();
            auto t = node(AstKind::Ternary, q);
            t->add(std::move(c));
            t->add(parseTernary());
            if (!accept(T::Colon)) fail(cur(), "esperado ':' no operador ternario (cond ? a : b), encontrado " + found());
            t->add(parseTernary());
            return t;
        }
        return c;
    }

    AstPtr parseBinary(int minPrec) {
        auto lhs = parseUnary();
        for (;;) {
            const int p = binaryPrec(cur().type);
            if (p == 0 || p < minPrec) break;
            const Token op = cur();
            advance();
            auto rhs = parseBinary(p + 1);
            auto b = node(AstKind::Binary, op);
            b->text = op.value;
            b->add(std::move(lhs));
            b->add(std::move(rhs));
            lhs = std::move(b);
        }
        return lhs;
    }

    AstPtr parseUnary() {
        const Token t = cur();
        switch (t.type) {
            case T::Minus: case T::Plus: case T::Not: case T::Tilde: case T::Increment: case T::Decrement: {
                advance();
                auto u = node(AstKind::Unary, t);
                u->text = t.value;
                u->add(parseUnary());
                return u;
            }
            default: break;
        }
        // -2 ** 2 == -(2 ** 2): o sinal do literal fica fora da potência
        if (t.type == T::Number && t.value.size() > 1 && t.value[0] == '-' && peek().type == T::Power) {
            auto u = node(AstKind::Unary, t);
            u->text = "-";
            u->add(parsePower(true));
            return u;
        }
        return parsePower(false);
    }

    AstPtr parsePower(bool stripNeg) {
        auto base = parsePostfixOps(parsePrimary(stripNeg));
        if (at(T::Power)) {
            const Token op = cur();
            advance();
            auto b = node(AstKind::Binary, op);
            b->text = "**";
            b->add(std::move(base));
            b->add(parseUnary()); // à direita e com sinal: 2 ** -1, 2 ** 3 ** 2
            return b;
        }
        return base;
    }

    bool isTryQuestion() const {
        std::vector<Token> rest(toks.begin() + static_cast<long>(pos) + 1,
                                toks.begin() + static_cast<long>(std::min(toks.size(), pos + 257)));
        return ghost_is_try_question(rest);
    }

    AstPtr parsePostfixOps(AstPtr e) {
        for (;;) {
            const Token t = cur();
            if (t.type == T::LParen) {
                advance();
                auto c = node(AstKind::Call, t);
                c->add(std::move(e));
                while (!at(T::RParen) && !at(T::EndOfFile) && !at(T::Semicolon)) {
                    c->add(parseExpr());
                    if (!accept(T::Comma)) break;
                }
                expectClose(T::RParen, t);
                e = std::move(c);
            } else if (t.type == T::Dot) {
                advance();
                if (!identLike(cur())) fail(cur(), "esperado o nome do membro depois de '.', encontrado " + found());
                auto m = node(AstKind::Member, cur());
                m->text = cur().value;
                advance();
                m->add(std::move(e));
                if (at(T::Lt) && generics.count(m->text) && typeArgsAhead(true)) parseTypeArgsInto(m.get()); // obj.f<int>(x)
                e = std::move(m);
            } else if (t.type == T::LBracket) {
                advance();
                AstPtr start;
                if (at(T::Colon)) start = node(AstKind::Empty, cur());
                else start = parseExpr();
                if (at(T::Colon)) {
                    advance();
                    auto s = node(AstKind::Slice, t);
                    s->add(std::move(e));
                    s->add(std::move(start));
                    if (at(T::Colon) || at(T::RBracket)) s->add(node(AstKind::Empty, cur()));
                    else s->add(parseExpr());
                    if (accept(T::Colon) && !at(T::RBracket)) s->add(parseExpr());
                    else s->add(node(AstKind::Empty, cur()));
                    expectClose(T::RBracket, t);
                    e = std::move(s);
                } else {
                    expectClose(T::RBracket, t);
                    auto ix = node(AstKind::Index, t);
                    ix->add(std::move(e));
                    ix->add(std::move(start));
                    e = std::move(ix);
                }
            } else if ((t.type == T::Increment || t.type == T::Decrement) && t.line == prev().line) {
                advance();
                auto p = node(AstKind::Postfix, t);
                p->text = t.value;
                p->add(std::move(e));
                e = std::move(p);
            } else if (t.type == T::Question && isTryQuestion()) {
                advance();
                auto q = node(AstKind::TryOp, t);
                q->add(std::move(e));
                e = std::move(q);
            } else {
                return e;
            }
        }
    }

    AstPtr ident(const Token& t, const std::string& name) {
        auto n = node(AstKind::Ident, t);
        n->text = name;
        return n;
    }

    AstPtr parsePrimary(bool stripNeg) {
        const Token t = cur();
        switch (t.type) {
            case T::Number: {
                advance();
                auto n = node(AstKind::Number, t);
                n->text = stripNeg ? t.value.substr(1) : t.value;
                return n;
            }
            case T::String: {
                advance();
                auto n = node(AstKind::String, t);
                n->text = t.value;
                return n;
            }
            case T::FString: return parseFString();
            case T::KwThis: advance(); return node(AstKind::This, t);
            case T::KwSuper: advance(); return node(AstKind::Super, t);
            case T::KwNew: {
                advance();
                auto n = node(AstKind::New, t);
                n->add(parsePostfixOps(parsePrimary(false)));
                return n;
            }
            case T::LParen: {
                if (lambdaAhead()) return parseLambda();
                // (int)x: conversão no estilo C (passa direto para o C++)
                if ((peek().type == T::KwInt || peek().type == T::KwDouble || peek().type == T::KwFloat ||
                     peek().type == T::KwBool) && peek(2).type == T::RParen && canStartExpr(peek(3).type)) {
                    auto c = node(AstKind::Cast, t);
                    c->text = peek().value;
                    advance();
                    advance();
                    advance();
                    c->add(parseUnary());
                    return c;
                }
                advance();
                auto e = parseExpr();
                expectClose(T::RParen, t);
                e->parens++;
                return e;
            }
            case T::LBracket:
                if (captureLambdaAhead()) return parseLambda();
                return parseListOrComprehension();
            case T::BitOr:
            case T::LogicalOr:
                if (pipeLambdaAhead()) return parseLambda();
                break;
            case T::LBrace: return parseBraceLiteral();
            case T::Identifier: {
                advance();
                if (at(T::Lt)) {
                    const bool builtin = t.value == "set" || t.value == "result" || t.value == "fn";
                    if ((generics.count(t.value) && typeArgsAhead(false)) || (builtin && typeArgsAhead(true))) {
                        auto g = node(AstKind::GenericRef, t);
                        g->add(ident(t, t.value));
                        parseTypeArgsInto(g.get());
                        return g;
                    }
                }
                return ident(t, t.value);
            }
            case T::KwList: case T::KwMap: {
                auto n = node(AstKind::TypeExpr, t);
                n->add(parseType());
                return n;
            }
            default: break;
        }
        if (isTypeKeyword(t.type) && (peek().type == T::LParen || peek().type == T::Dot)) { // bool(x), int(x), qreg(4), json.parse(...)
            advance();
            return ident(t, t.value);
        }
        if (t.type == T::KwTranspose && peek().type == T::Identifier) { // m2 = transpose m;
            advance();
            auto c = node(AstKind::Call, t);
            c->add(ident(t, "transpose"));
            c->add(ident(cur(), cur().value));
            advance();
            return c;
        }
        if (isKeywordName(t.type)) {
            advance();
            return ident(t, t.value);
        }
        if (t.type == T::EndOfFile) fail(t, "expressao incompleta: o arquivo terminou antes do valor");
        fail(t, "esperado um valor, encontrado " + found());
    }

    // (params) => ...  /  (params) -> tipo => ...  /  [capturas](params) => ...
    bool arrowFollows(size_t close) const {
        if (close + 1 >= toks.size()) return false;
        if (toks[close + 1].type == T::Arrow) return true;
        if (toks[close + 1].type != T::ThinArrow) return false;
        int angle = 0, paren = 0, bracket = 0;
        for (size_t k = close + 2; k < toks.size(); ++k) {
            const T ty = toks[k].type;
            if (angle == 0 && paren == 0 && bracket == 0 && ty == T::Arrow) return k > close + 2;
            if (ty == T::Lt) angle++;
            else if (ty == T::Gt) angle--;
            else if (ty == T::Chain) angle -= 2;
            else if (ty == T::LParen) paren++;
            else if (ty == T::RParen) { if (--paren < 0) return false; }
            else if (ty == T::LBracket) bracket++;
            else if (ty == T::RBracket) { if (--bracket < 0) return false; }
            else if (angle == 0 && paren == 0 && bracket == 0 && (ty == T::Semicolon || ty == T::LBrace || ty == T::Assign))
                return false;
            else if (ty == T::EndOfFile) return false;
        }
        return false;
    }
    // índice do fechamento do grupo aberto em i (0 se o fechamento não for do mesmo tipo)
    size_t matchingClose(size_t i) const {
        int depth = 0;
        for (size_t k = i; k < toks.size(); ++k) {
            const T ty = toks[k].type;
            if (ty == T::LParen || ty == T::LBracket || ty == T::LBrace) depth++;
            else if (ty == T::RParen || ty == T::RBracket || ty == T::RBrace) {
                if (--depth == 0) {
                    const T open = toks[i].type;
                    const bool same = (open == T::LParen && ty == T::RParen) || (open == T::LBracket && ty == T::RBracket) ||
                                      (open == T::LBrace && ty == T::RBrace);
                    return same ? k : 0;
                }
            } else if (ty == T::EndOfFile) {
                return 0;
            }
        }
        return 0;
    }
    bool lambdaAhead() const {
        const size_t close = matchingClose(pos);
        return close != 0 && arrowFollows(close);
    }
    bool pipeLambdaAheadAt(size_t i) const {
        if (i >= toks.size()) return false;
        if (toks[i].type == T::LogicalOr) {
            return arrowFollows(i);
        }
        if (toks[i].type != T::BitOr) return false;
        int angle = 0, paren = 0, bracket = 0, brace = 0;
        for (size_t k = i + 1; k < toks.size(); ++k) {
            const T ty = toks[k].type;
            if (ty == T::EndOfFile || ty == T::Semicolon) return false;
            if (angle == 0 && paren == 0 && bracket == 0 && brace == 0 && ty == T::BitOr) {
                return arrowFollows(k);
            }
            if (ty == T::Lt) angle++;
            else if (ty == T::Gt) { if (angle > 0) angle--; }
            else if (ty == T::Chain) { if (angle >= 2) angle -= 2; }
            else if (ty == T::LParen) paren++;
            else if (ty == T::RParen) { if (paren > 0) paren--; }
            else if (ty == T::LBracket) bracket++;
            else if (ty == T::RBracket) { if (bracket > 0) bracket--; }
            else if (ty == T::LBrace) brace++;
            else if (ty == T::RBrace) { if (brace > 0) brace--; }
        }
        return false;
    }
    bool pipeLambdaAhead() const {
        return pipeLambdaAheadAt(pos);
    }
    bool captureLambdaAhead() const {
        size_t k = pos + 1;
        for (; k < toks.size(); ++k) {
            const T ty = toks[k].type;
            if (ty == T::RBracket) break;
            if (ty == T::Comma || ty == T::Assign || ty == T::BitAnd || ty == T::KwThis || ty == T::Identifier) continue;
            return false;
        }
        if (k + 1 >= toks.size()) return false;
        if (toks[k + 1].type == T::LParen) {
            const size_t close = matchingClose(k + 1);
            return close != 0 && arrowFollows(close);
        }
        if (toks[k + 1].type == T::BitOr || toks[k + 1].type == T::LogicalOr) {
            return pipeLambdaAheadAt(k + 1);
        }
        return false;
    }

    AstPtr parseLambda() {
        auto l = node(AstKind::Lambda, cur());
        if (at(T::LBracket)) {
            l->attr = "caps";  // tem lista de captura [..] (mesmo vazia)
            const Token open = cur();
            advance();
            while (!at(T::RBracket) && !at(T::EndOfFile)) {
                auto c = node(AstKind::Capture, cur());
                if (at(T::Assign)) {
                    c->text = "=";
                    advance();
                } else if (at(T::BitAnd)) {
                    advance();
                    if (at(T::Identifier) || at(T::KwThis)) {
                        c->text = at(T::KwThis) ? "this" : cur().value;
                        c->attr = "ref";
                        advance();
                    } else {
                        c->text = "&";
                    }
                } else if (at(T::Identifier) || at(T::KwThis)) {
                    c->text = at(T::KwThis) ? "this" : cur().value;
                    advance();
                } else {
                    fail(cur(), "captura invalida na lambda: use [&x], [x], [=], [&], [] ou [&this]");
                }
                l->add(std::move(c));
                if (!accept(T::Comma)) break;
            }
            expectClose(T::RBracket, open);
        }
        if (at(T::LogicalOr)) {
            advance(); // || => ... (zero parametros)
        } else if (at(T::BitOr)) {
            const Token open = cur();
            advance(); // consome '|'
            while (!at(T::BitOr) && !at(T::EndOfFile)) {
                auto p = node(AstKind::Param, cur());
                if (at(T::KwVar)) {
                    auto v = node(AstKind::Type, cur());
                    v->text = "var";
                    advance();
                    p->add(std::move(v));
                } else if (typeThenIdent()) {
                    p->add(parseType());
                } else {
                    auto v = node(AstKind::Type, cur());
                    v->text = "var";
                    p->add(std::move(v));
                }
                if (!at(T::Identifier)) fail(cur(), "parametro invalido na lambda: " + found());
                p->name = cur().value;
                advance();
                if (at(T::LBracket) && peek().type == T::RBracket) {
                    advance();
                    advance();
                    auto lt = node(AstKind::Type, prev());
                    lt->text = "list";
                    lt->add(std::move(p->kids[0]));
                    p->kids[0] = std::move(lt);
                }
                l->add(std::move(p));
                if (!accept(T::Comma)) break;
            }
            expectClose(T::BitOr, open);
        } else {
            const Token open = cur();
            expect(T::LParen);
            while (!at(T::RParen) && !at(T::EndOfFile)) {
                auto p = node(AstKind::Param, cur());
                if (at(T::KwVar)) {
                    auto v = node(AstKind::Type, cur());
                    v->text = "var";
                    advance();
                    p->add(std::move(v));
                } else if (typeThenIdent()) {
                    p->add(parseType());
                } else {
                    auto v = node(AstKind::Type, cur());
                    v->text = "var";
                    p->add(std::move(v));
                }
                if (!at(T::Identifier)) fail(cur(), "parametro invalido na lambda: " + found());
                p->name = cur().value;
                advance();
                if (at(T::LBracket) && peek().type == T::RBracket) {
                    advance();
                    advance();
                    auto lt = node(AstKind::Type, prev());
                    lt->text = "list";
                    lt->add(std::move(p->kids[0]));
                    p->kids[0] = std::move(lt);
                }
                l->add(std::move(p));
                if (!accept(T::Comma)) break;
            }
            expectClose(T::RParen, open);
        }
        if (accept(T::ThinArrow)) {
            auto r = parseType();
            r->attr = "ret";
            l->add(std::move(r));
        }
        expect(T::Arrow);
        if (at(T::LBrace)) l->add(parseBlock());
        else if (at(T::KwPrint)) l->add(parsePrint());
        else l->add(parseExpr());
        return l;
    }

    void parseCompClauses(AstNode* c) {
        for (;;) {
            if (at(T::KwFor)) {
                auto f = node(AstKind::CompFor, cur());
                advance();
                if (at(T::KwVar) || typeThenIdent()) parseType();
                if (!at(T::Identifier)) fail(cur(), "compreensao: use [expr for x in colecao]");
                f->name = cur().value;
                advance();
                if (accept(T::Comma)) {
                    if (!at(T::Identifier)) fail(cur(), "compreensao: use for chave, valor in mapa");
                    f->attr = cur().value;
                    advance();
                }
                if (!accept(T::KwIn)) fail(cur(), "compreensao: faltou 'in' (for x in colecao)");
                f->add(parseTernary());
                c->add(std::move(f));
            } else if (at(T::KwIf)) {
                auto i = node(AstKind::CompIf, cur());
                advance();
                i->add(parseTernary());
                c->add(std::move(i));
            } else {
                return;
            }
        }
    }

    AstPtr parseListOrComprehension() {
        const Token open = cur();
        advance(); // '['
        auto l = node(AstKind::ListLit, open);
        if (accept(T::RBracket)) return l;
        auto first = parseExpr();
        if (at(T::KwFor)) {
            auto c = node(AstKind::Comprehension, open);
            c->attr = "list";
            c->add(std::move(first));
            parseCompClauses(c.get());
            expectClose(T::RBracket, open);
            return c;
        }
        l->add(std::move(first));
        while (accept(T::Comma)) {
            if (at(T::RBracket)) break;
            l->add(parseExpr());
        }
        expectClose(T::RBracket, open);
        return l;
    }

    AstPtr parseBraceLiteral() {
        const Token open = cur();
        advance(); // '{'
        if (accept(T::RBrace)) return node(AstKind::MapLit, open);
        auto first = parseExpr();
        if (accept(T::Colon)) {
            auto e = node(AstKind::MapEntry, open);
            e->add(std::move(first));
            e->add(parseExpr());
            if (at(T::KwFor)) {
                auto c = node(AstKind::Comprehension, open);
                c->attr = "map";
                c->add(std::move(e));
                parseCompClauses(c.get());
                expectClose(T::RBrace, open);
                return c;
            }
            auto m = node(AstKind::MapLit, open);
            m->add(std::move(e));
            while (accept(T::Comma)) {
                if (at(T::RBrace)) break;
                auto en = node(AstKind::MapEntry, cur());
                en->add(parseExpr());
                if (!accept(T::Colon)) fail(cur(), "mapa: esperado ':' entre chave e valor, encontrado " + found());
                en->add(parseExpr());
                m->add(std::move(en));
            }
            expectClose(T::RBrace, open);
            return m;
        }
        if (at(T::KwFor)) {
            auto c = node(AstKind::Comprehension, open);
            c->attr = "set";
            c->add(std::move(first));
            parseCompClauses(c.get());
            expectClose(T::RBrace, open);
            return c;
        }
        auto s = node(AstKind::ListLit, open);
        s->attr = "set";
        s->add(std::move(first));
        while (accept(T::Comma)) {
            if (at(T::RBrace)) break;
            s->add(parseExpr());
        }
        expectClose(T::RBrace, open);
        return s;
    }

    AstPtr parseFString() {
        const Token t = cur();
        advance();
        auto n = node(AstKind::FString, t);
        n->text = t.value;
        const std::string& s = t.value;
        int lineInside = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\n') lineInside++;
            if (s[i] == '\\' && i + 1 < s.size()) { ++i; continue; }
            if (s[i] == '{' && i + 1 < s.size() && s[i + 1] == '{') { ++i; continue; }
            if (s[i] == '}' && i + 1 < s.size() && s[i + 1] == '}') { ++i; continue; }
            const bool hasDollar = (s[i] == '$' && i + 1 < s.size() && s[i + 1] == '{');
            if (!hasDollar && s[i] != '{') continue;
            const size_t offset = hasDollar ? 2 : 1;
            size_t j = i + offset;
            int depth = 1;
            char q = 0;
            for (; j < s.size(); ++j) {
                const char c = s[j];
                if (q) {
                    if (c == '\\') ++j;
                    else if (c == q) q = 0;
                    continue;
                }
                if (c == '"' || c == '\'' || c == '`') q = c;
                else if (c == '{') depth++;
                else if (c == '}' && --depth == 0) break;
            }
            if (j >= s.size()) {
                record(t.line, t.column, "f-string: faltou '}' para fechar a expressao");
                break;
            }
            const std::string inner = s.substr(i + offset, j - i - offset);
            AstParser sub(inner, &generics, t.line + lineInside, t.column);
            std::string spec;
            AstPtr e = sub.parseEmbedded(spec, inner);
            for (const auto& err : sub.errors) record(err.line, err.col, "f-string: " + err.msg);
            if (e) {
                if (!spec.empty()) e->attr = joinAttr(e->attr, "fmt=" + spec);
                n->add(std::move(e));
            }
            i = j;
        }
        return n;
    }
};

void dumpLine(const AstNode& n, std::ostream& os) {
    const auto loc = GhostDiag::get().locate(n.line);
    os << "  @" << loc.second << "\n";
}

} // namespace

bool AstNode::has(const std::string& mark) const {
    std::istringstream in(attr);
    std::string w;
    while (in >> w)
        if (w == mark) return true;
    return false;
}

AstParseResult ghost_ast_parse(const std::string& source) {
    AstParser p(source, nullptr, 0, 0);
    return p.run();
}

AstPtr ghost_ast_parse_expr(const std::string& source, int line, std::vector<AstDiagnostic>* errors,
                            const std::set<std::string>* generics) {
    AstParser p(source, generics, line, 0);
    std::string spec;
    AstPtr e = p.parseEmbedded(spec, source);
    if (errors) errors->insert(errors->end(), p.errors.begin(), p.errors.end());
    return p.errors.empty() ? std::move(e) : nullptr;
}

const char* ghost_ast_kind_name(AstKind k) {
    static const char* names[] = {
        "Program", "Block", "Import", "VarDecl", "FuncDecl", "Param", "TypeParam", "ClassDecl", "InterfaceDecl", "CircuitDecl",
        "If", "While", "DoWhile", "For", "Foreach", "Switch", "Case", "Try", "Catch", "Finally", "Throw", "Return", "Break", "Continue",
        "Empty", "ExprStmt", "Print", "Spawn", "Collapse", "Hypothesis", "Domain", "Type",
        "Number", "String", "FString", "Ident", "This", "Super", "Unary", "Postfix", "Binary", "Assign", "Ternary", "Call", "GenericRef",
        "Member", "Index", "Slice", "ListLit", "MapLit", "MapEntry", "Lambda", "Capture", "Comprehension", "CompFor", "CompIf",
        "TryOp", "New", "TypeExpr", "Cast"};
    const size_t i = static_cast<size_t>(k);
    return i < sizeof(names) / sizeof(names[0]) ? names[i] : "?";
}

std::string ghost_ast_type_str(const AstNode& t) {
    if (t.kids.empty()) return t.text;
    std::string s = t.text + "<";
    if (t.text == "fn") {
        s += ghost_ast_type_str(*t.kids[0]) + "(";
        for (size_t i = 1; i < t.kids.size(); ++i) s += (i > 1 ? ", " : "") + ghost_ast_type_str(*t.kids[i]);
        return s + ")>";
    }
    for (size_t i = 0; i < t.kids.size(); ++i) s += (i ? ", " : "") + ghost_ast_type_str(*t.kids[i]);
    return s + ">";
}

void ghost_ast_dump(const AstNode& n, std::ostream& os, int indent) {
    const std::string pad(static_cast<size_t>(indent) * 2, ' ');
    os << pad << ghost_ast_kind_name(n.kind);
    size_t first = 0;   // filhos já mostrados na própria linha
    switch (n.kind) {
        case AstKind::Type:
            os << " " << ghost_ast_type_str(n);
            if (!n.attr.empty()) os << " [" << n.attr << "]";
            dumpLine(n, os);
            return;
        case AstKind::VarDecl: case AstKind::Param: case AstKind::FuncDecl:
            os << " " << n.name;
            if (!n.kids.empty() && n.kids[0]->kind == AstKind::Type) {
                os << " : " << ghost_ast_type_str(*n.kids[0]);
                first = 1;
            }
            break;
        case AstKind::Foreach: case AstKind::Hypothesis:
            os << " " << n.name;
            if (n.kind == AstKind::Foreach && !n.attr.empty()) os << ", " << n.attr;
            if (!n.kids.empty() && n.kids[0]->kind == AstKind::Type) {
                os << " : " << ghost_ast_type_str(*n.kids[0]);
                first = 1;
            }
            if (n.kind == AstKind::Foreach) {
                dumpLine(n, os);
                for (size_t i = first; i < n.kids.size(); ++i) ghost_ast_dump(*n.kids[i], os, indent + 1);
                return;
            }
            break;
        case AstKind::String: os << " \"" << n.text << "\""; break;
        case AstKind::FString: os << " f\"" << n.text << "\""; break;
        default:
            if (!n.text.empty()) os << " " << n.text;
            if (!n.name.empty()) os << " " << n.name;
            break;
    }
    if (!n.attr.empty() && n.kind != AstKind::Foreach) os << " [" << n.attr << "]";
    dumpLine(n, os);
    for (size_t i = first; i < n.kids.size(); ++i) ghost_ast_dump(*n.kids[i], os, indent + 1);
}
