#include "../include/Lexer.hpp"
#include "../include/GhostModules.hpp"
#include "../include/GhostFormat.hpp"
#include <algorithm>
#include <cctype>
#include <vector>

// Construtor
Lexer::Lexer(const std::string& sourceCode) : source(sourceCode), position(0), currentLine(1), currentColumn(1) {
    // Editores no Windows costumam salvar UTF-8 com BOM (EF BB BF); ele nao faz parte do codigo.
    if (source.size() >= 3 && (unsigned char)source[0] == 0xEF && (unsigned char)source[1] == 0xBB && (unsigned char)source[2] == 0xBF) {
        source.erase(0, 3);
    }

    // Populando o dicionário de palavras reservadas
    keywords["state"] = GTokenType::KwState;
    keywords["gate"] = GTokenType::KwGate;
    keywords["optimize"] = GTokenType::KwOptimize;
    keywords["show"] = GTokenType::KwShow;
    keywords["epochs"] = GTokenType::KwEpochs;
    keywords["lr"] = GTokenType::KwLr;
    keywords["opt"] = GTokenType::KwOpt;
    keywords["load"] = GTokenType::KwLoad;
    keywords["activate"] = GTokenType::KwActivate;
    keywords["apply"] = GTokenType::KwApply;
    keywords["relu"] = GTokenType::KwRelu;
    keywords["sigmoid"] = GTokenType::KwSigmoid;
    keywords["sin"] = GTokenType::KwSin;
    keywords["cos"] = GTokenType::KwCos;
    keywords["tan"] = GTokenType::KwTan;
    keywords["transpose"] = GTokenType::KwTranspose;
    keywords["network"] = GTokenType::KwNetwork;
    keywords["hadamard"] = GTokenType::KwHadamard;
    keywords["paulix"] = GTokenType::KwPauliX;
    keywords["pauliy"] = GTokenType::KwPauliY;
    keywords["pauliz"] = GTokenType::KwPauliZ;
    keywords["if"] = GTokenType::KwIf;
    keywords["else"] = GTokenType::KwElse;
    keywords["for"] = GTokenType::KwFor;
    keywords["while"] = GTokenType::KwWhile;
    keywords["do"] = GTokenType::KwDo;
    keywords["foreach"] = GTokenType::KwForeach;
    keywords["in"] = GTokenType::KwIn;
    
    // Switch Statements
    keywords["switch"] = GTokenType::KwSwitch;
    keywords["case"] = GTokenType::KwCase;
    keywords["default"] = GTokenType::KwDefault;
    keywords["break"] = GTokenType::KwBreak;
    
    // Multiverse
    keywords["hypothesis"] = GTokenType::KwHypothesis;
    keywords["collapse_max"] = GTokenType::KwCollapseMax;
    keywords["collapse_min"] = GTokenType::KwCollapseMin;
    
    // Threads
    keywords["spawn"] = GTokenType::KwSpawn;
    keywords["sleep"] = GTokenType::KwSleep;
    keywords["mutex"] = GTokenType::KwMutex;
    keywords["lock"] = GTokenType::KwLock;
    keywords["unlock"] = GTokenType::KwUnlock;

    keywords["var"] = GTokenType::KwVar;
    keywords["print"] = GTokenType::KwPrint;
    keywords["println"] = GTokenType::KwPrint;
    keywords["printf"] = GTokenType::KwPrint;
    keywords["imprimir"] = GTokenType::KwPrint;
    keywords["escreva"] = GTokenType::KwPrint;
    keywords["escrevaln"] = GTokenType::KwPrint;
    keywords["input"] = GTokenType::KwInput;
    keywords["inputln"] = GTokenType::KwInput;
    keywords["function"] = GTokenType::KwFunction;
    keywords["return"] = GTokenType::KwReturn;
    keywords["void"] = GTokenType::KwVoid;
    
    // OOP
    keywords["class"] = GTokenType::KwClass;
    keywords["public"] = GTokenType::KwPublic;
    keywords["private"] = GTokenType::KwPrivate;
    keywords["this"] = GTokenType::KwThis;
    keywords["new"] = GTokenType::KwNew;
    
    keywords["pragma"] = GTokenType::KwPragma;
    keywords["gpu"] = GTokenType::KwGpu;
    keywords["swarm"] = GTokenType::KwSwarm;
    keywords["qreg"] = GTokenType::KwQreg;
    keywords["qcircuit"] = GTokenType::KwQcircuit;
    keywords["qlayer"] = GTokenType::KwQlayer;
    keywords["circuit"] = GTokenType::KwCircuit;
    // Nucleo da linguagem
    keywords["bool"] = GTokenType::KwBool;
    keywords["continue"] = GTokenType::KwContinue;
    keywords["try"] = GTokenType::KwTry;
    keywords["catch"] = GTokenType::KwCatch;
    keywords["throw"] = GTokenType::KwThrow;
    keywords["finally"] = GTokenType::KwFinally;
    keywords["extends"] = GTokenType::KwExtends;
    keywords["super"] = GTokenType::KwSuper;
    keywords["list"] = GTokenType::KwList;
    keywords["map"] = GTokenType::KwMap;
    
    // Tipagem
    keywords["string"] = GTokenType::KwString;
    keywords["date"] = GTokenType::KwDate;
    keywords["time"] = GTokenType::KwTime;
    keywords["datetime"] = GTokenType::KwDatetime;
    keywords["int"] = GTokenType::KwInt;
    keywords["double"] = GTokenType::KwDouble;
    keywords["float"] = GTokenType::KwFloat;
    keywords["complex"] = GTokenType::KwComplex;
    keywords["ghost"] = GTokenType::KwGhost;
    keywords["json"] = GTokenType::KwJson;
    keywords["matrix"] = GTokenType::KwMatrix;
    keywords["GhostMatrix"] = GTokenType::KwMatrix;
    keywords["server"] = GTokenType::KwServer;
    keywords["route"] = GTokenType::KwRoute;
    keywords["listen"] = GTokenType::KwListen;
    keywords["load"] = GTokenType::KwLoad;
    keywords["load_gguf"] = GTokenType::KwLoadGguf;
    keywords["import"] = GTokenType::KwImport;
    keywords["textmodel"] = GTokenType::KwTextModel;
    keywords["llm"] = GTokenType::KwLlm;
    
    keywords["pragma"] = GTokenType::KwPragma;
    keywords["gpu"] = GTokenType::KwGpu;
    keywords["conv"] = GTokenType::KwConv;
}

// Texto de f-string -> literal C++ "...". Mantem os escapes do codigo-fonte (\n, \")
// e, em f`...`, escapa aspas e quebras de linha que la sao permitidas sem barra.
std::string Lexer::cppStringLiteral(const std::string& text, char quote) {
    std::string out = "std::string(\"";
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\\' && i + 1 < text.size()) { out += c; out += text[++i]; continue; }
        if (c == '"') { out += "\\\""; continue; }
        if (c == '\n') { out += "\\n"; continue; }
        if (c == '\r') continue;
        if (c == '`' && quote == '`') continue;
        out += c;
    }
    return out + "\")";
}

// f"a ${expr} b" -> ghost_fstr(std::string("a "), (expr), std::string(" b"))
// A expressao e lexada de novo (modo compilador), como o parser faz com o resto do codigo.
std::string Lexer::fstringToCpp(const std::string& text, char quote) const {
    std::vector<std::string> parts;
    std::string literal;
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '\\' && i + 1 < text.size()) { literal += text[i]; literal += text[i + 1]; i += 2; continue; }
        if (text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{') {
            size_t j = i + 2; int depth = 1;
            while (j < text.size() && depth > 0) {
                if (text[j] == '"') { // pula strings da expressao (podem conter chaves)
                    for (++j; j < text.size() && text[j] != '"'; ++j) if (text[j] == '\\') ++j;
                    ++j;
                    continue;
                }
                if (text[j] == '{') depth++;
                else if (text[j] == '}') depth--;
                if (depth > 0) j++;
            }
            if (depth != 0) { literal += text.substr(i); break; } // ${ sem fechamento: texto literal
            if (!literal.empty()) { parts.push_back(cppStringLiteral(literal, quote)); literal.clear(); }
            std::string inner, spec; // ${x:.2f}: formatação (include/GhostFormat.hpp)
            ghost_split_fmt(text.substr(i + 2, j - (i + 2)), inner, spec);
            // A expressão vai crua entre \x01 e \x02: o Parser traduz com translateExpr (Parser::expandFString),
            // então ${2 ** 8}, ${[x * 2 for x in xs]}, ${maior<int>(a, b)} funcionam como fora da f-string.
            bool blank = true;
            for (char ch : inner) if (!std::isspace(static_cast<unsigned char>(ch))) blank = false;
            const std::string e = blank ? std::string("(\"\")") : "\x01" + inner + "\x02";
            parts.push_back(spec.empty() ? e : "ghost_fmt(" + e + ", \"" + spec + "\")");
            i = j + 1;
            continue;
        }
        literal += text[i++];
    }
    if (!literal.empty() || parts.empty()) parts.push_back(cppStringLiteral(literal, quote));
    if (parts.size() == 1 && parts[0].rfind("std::string(", 0) == 0) return parts[0];
    std::string out = "ghost_fstr(";
    for (size_t k = 0; k < parts.size(); ++k) out += (k ? ", " : "") + parts[k];
    return out + ")";
}

char Lexer::peek(int offset) const {
    if (position + offset >= source.length()) return '\0';
    return source[position + offset];
}

char Lexer::advance() {
    if (position >= source.length()) return '\0';
    char c = source[position++];
    if (c == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }
    return c;
}

Token Lexer::createToken(GTokenType type, const std::string& value) {
    return {type, value, tokenLine, tokenColumn};
}

void Lexer::skipWhitespaceAndComments() {
    while (position < source.length()) {
        char c = peek();
        
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // Ignora comentarios de linha "//"
            while (peek() != '\n' && peek() != '\0') advance();
        } else if (c == '/' && peek(1) == '*') {
            // Ignora comentarios de bloco "/* ... */"
            advance(); advance();
            while (position < source.length() && !(peek() == '*' && peek(1) == '/')) {
                advance();
            }
            if (position < source.length()) {
                advance(); advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::getNextToken() {
    skipWhitespaceAndComments();
    tokenLine = currentLine;     // linha e coluna onde o token começa
    tokenColumn = currentColumn;

    if (position >= source.length()) {
        return createToken(GTokenType::EndOfFile, "EOF");
    }

    char c = peek();


    // F-Strings: f"..." ou f`...`
    if (c == 'f' && (peek(1) == '"' || peek(1) == '`')) {
        advance(); // consome 'f'
        char quote = advance(); // consome a aspa ou crase
        std::string text = "";
        int interp = 0; // profundidade de chaves dentro de ${ ... }
        while (peek() != '\0') {
            if (peek() == '\\') {
                text += advance();
                if (peek() != '\0') text += advance();
                continue;
            }
            if (interp == 0 && peek() == '$' && peek(1) == '{') { text += advance(); text += advance(); interp = 1; continue; }
            if (interp > 0) {
                // Dentro de ${...} aspas abrem uma string da expressao: ${q.expect("ZZ")}
                if (peek() == '"') {
                    text += advance();
                    while (peek() != '\0' && peek() != '"') {
                        if (peek() == '\\') { text += advance(); if (peek() == '\0') break; }
                        text += advance();
                    }
                    if (peek() == '"') text += advance();
                    continue;
                }
                if (peek() == '{') interp++;
                else if (peek() == '}') interp--;
                text += advance();
                continue;
            }
            if (peek() == quote) break;
            text += advance();
        }
        advance(); // consome a aspa ou crase de fechamento
        if (cppLiterals) return createToken(GTokenType::FString, fstringToCpp(text, quote));
        return createToken(GTokenType::FString, text);
    }

    // NOVO: Detecção de Strings (Textos entre aspas)
    if (c == '"') {
        advance(); // Pula aspas de abertura
        std::string text = "";
        while (peek() != '\0') {
            if (peek() == '\\') {
                text += advance(); // pega a barra
                if (peek() != '\0') text += advance(); // pega o caractere escapado (ex: ")
                continue;
            }
            if (peek() == '"') break;
            text += advance();
        }
        advance(); // Pula aspas de fechamento
        return createToken(GTokenType::String, text);
    }

    // 1. Identificadores e Palavras-chave
    if (std::isalpha(c) || c == '_') {
        std::string text = "";
        while (std::isalnum(peek()) || peek() == '_') {
            text += advance();
        }
        // import "x.g" as m;  ->  m.nome vira um nome só (m__nome) quando nome é exportado pelo módulo
        {
            const auto& aliases = ghost_module_aliases();
            auto al = aliases.find(text);
            if (al != aliases.end() && peek() == '.' && (std::isalpha((unsigned char)peek(1)) || peek(1) == '_')) {
                std::string member;
                for (int k = 1; std::isalnum((unsigned char)peek(k)) || peek(k) == '_'; ++k) member += peek(k);
                if (al->second.count(member)) {
                    advance(); // '.'
                    for (size_t k = 0; k < member.size(); ++k) advance();
                    return createToken(GTokenType::Identifier, ghost_module_join(text, member));
                }
            }
        }

        // Verifica se é palavra reservada
        if (keywords.find(text) != keywords.end()) {
            if (text == "this") return createToken(GTokenType::KwThis, "(*this)");
            if (text == "new") return createToken(GTokenType::KwNew, "");
            return createToken(keywords[text], text);
        }
        return createToken(GTokenType::Identifier, text);
    }

    // Operadores compostos: ++ -- += -= *= /= %= (antes dos números negativos)
    {
        const char n1 = peek(1);
        if ((c == '+' || c == '-') && n1 == c) { advance(); advance(); return createToken(c == '+' ? GTokenType::Increment : GTokenType::Decrement, c == '+' ? "++" : "--"); }
        if (c == '*' && n1 == '*') {
            if (peek(2) == '=') {
                advance(); advance(); advance();
                return createToken(GTokenType::StarAssign, "**=");
            }
            advance(); advance();
            return createToken(GTokenType::Power, "**");
        }
        if ((c == '+' || c == '-' || c == '*' || c == '/' || c == '%') && n1 == '=') {
            advance(); advance();
            switch (c) {
                case '+': return createToken(GTokenType::PlusAssign, "+=");
                case '-': return createToken(GTokenType::MinusAssign, "-=");
                case '*': return createToken(GTokenType::StarAssign, "*=");
                case '/': return createToken(GTokenType::SlashAssign, "/=");
                default: return createToken(GTokenType::PercentAssign, "%=");
            }
        }
        if (c == '-' && n1 == '>') { advance(); advance(); return createToken(GTokenType::ThinArrow, "->"); }
    }

    // 2. Números (Inteiros, Decimais e Negativos)
    if (std::isdigit(c) || (c == '-' && std::isdigit(peek(1)))) {
        std::string num = "";
        
        // Se for negativo, consome o sinal de menos primeiro
        if (c == '-') {
            num += advance(); 
        }
        
        // Consome os dígitos e o ponto decimal
        while (std::isdigit(peek()) || peek() == '.') {
            num += advance();
        }
        // Notação científica: 1e20, 2.5e-3, 6.02E+23
        if ((peek() == 'e' || peek() == 'E') &&
            (std::isdigit((unsigned char)peek(1)) || ((peek(1) == '-' || peek(1) == '+') && std::isdigit((unsigned char)peek(2))))) {
            num += advance();
            if (peek() == '-' || peek() == '+') num += advance();
            while (std::isdigit(peek())) num += advance();
        }

        // Literais imaginario (2.0i) e fantasma (3.5g) no modo compilador e REPL.
        // So vale quando o sufixo nao inicia um identificador (ex.: 2.0if nao e literal).
        // 5.0theta == 5.0g (unidade fantasma escrita por extenso)
        if (source.compare(position, 5, "theta") == 0 &&
            !(std::isalnum((unsigned char)peek(5)) || peek(5) == '_')) {
            for (int k = 0; k < 5; ++k) advance();
            if (cppLiterals) {
                if (!num.empty() && num[0] == '-') return createToken(GTokenType::Number, "-ghost_g(" + num.substr(1) + ")");
                return createToken(GTokenType::Number, "ghost_g(" + num + ")");
            }
            return createToken(GTokenType::Number, num + "theta");
        }
        char suffix = peek();
        if ((suffix == 'i' || suffix == 'g') &&
            !(std::isalnum((unsigned char)peek(1)) || peek(1) == '_')) {
            advance();
            if (cppLiterals) {
                std::string fn = (suffix == 'i') ? "ghost_i(" : "ghost_g(";
                // O sinal fica fora da chamada para que "3.0-2.0i" continue sendo uma subtracao.
                if (!num.empty() && num[0] == '-') {
                    return createToken(GTokenType::Number, "-" + fn + num.substr(1) + ")");
                }
                return createToken(GTokenType::Number, fn + num + ")");
            }
            return createToken(GTokenType::Number, num + suffix);
        }
        return createToken(GTokenType::Number, num);
    }

    if (c == ';') { advance(); return createToken(GTokenType::Semicolon, ";"); }
    if (c == '{') { advance(); return createToken(GTokenType::LBrace, "{"); }
    if (c == '}') { advance(); return createToken(GTokenType::RBrace, "}"); }
    
    if ((c=='&' || c=='|') && position+1<source.size() && source[position+1]==c) {
        advance(); advance();
        return createToken(c=='&'?GTokenType::LogicalAnd:GTokenType::LogicalOr, c=='&'?"&&":"||");
    }
    // Tratamento para #pragma
    if (c == '#') { 
        advance(); 
        // Vamos apenas ignorar o '#' e deixar o lexer ler 'pragma' como identifier normal.
        // O parser cuidará de parsear 'pragma gpu'.
        return getNextToken(); 
    }
    
    // Simbolos Compostos (==, !=, <=, >=, =>, >>)
    if (c == '=') {
        advance();
        if (peek() == '>') {
            advance();
            return createToken(GTokenType::Arrow, "=>");
        } else if (peek() == '=') {
            advance();
            return createToken(GTokenType::Eq, "==");
        }
        return createToken(GTokenType::Assign, "=");
    }

    if (c == '!') {
        advance();
        if (peek() == '=') {
            advance();
            return createToken(GTokenType::Neq, "!=");
        }
        return createToken(GTokenType::Not, "!");
    }

    if (c == '<') {
        advance();
        if (peek() == '=') {
            advance();
            return createToken(GTokenType::Leq, "<=");
        }
        if (peek() == '<') {
            advance();
            return createToken(GTokenType::ShiftLeft, "<<");
        }
        return createToken(GTokenType::Lt, "<");
    }

    if (c == '>') {
        advance();
        if (peek() == '>') {
            advance();
            return createToken(GTokenType::Chain, ">>");
        } else if (peek() == '=') {
            advance();
            return createToken(GTokenType::Geq, ">=");
        }
        return createToken(GTokenType::Gt, ">");
    }

    // 4. Símbolos Simples
    advance(); // Consome o caractere
    switch (c) {
        case '+': return createToken(GTokenType::Plus, "+");
        case '-': return createToken(GTokenType::Minus, "-");
        case '*': return createToken(GTokenType::Star, "*");
        case '/': return createToken(GTokenType::Slash, "/");
        case '^': return createToken(GTokenType::Caret, "^");
        case '%': return createToken(GTokenType::Percent, "%");
        case '&': return createToken(GTokenType::BitAnd, "&");
        case '|': return createToken(GTokenType::BitOr, "|");
        case '~': return createToken(GTokenType::Tilde, "~");
        case '?': return createToken(GTokenType::Question, "?");
        case '[': return createToken(GTokenType::LBracket, "[");
        case ']': return createToken(GTokenType::RBracket, "]");
        case '(': return createToken(GTokenType::LParen, "(");
        case ')': return createToken(GTokenType::RParen, ")");
        case ',': return createToken(GTokenType::Comma, ",");
        case ';': return createToken(GTokenType::Semicolon, ";");
        case '{': return createToken(GTokenType::LBrace, "{");
        case '}': return createToken(GTokenType::RBrace, "}");
        case ':': return createToken(GTokenType::Colon, ":");
        case '.': return createToken(GTokenType::Dot, ".");
    }

    return createToken(GTokenType::Unknown, std::string(1, c));
}

std::string Lexer::extractBlock() {
    int depth = 1;
    std::string block = "";
    while (position < source.length()) {
        char c = source[position];
        if (c == '{') depth++;
        if (c == '}') depth--;
        
        if (depth == 0) {
            advance(); // Consume '}'
            break;
        }
        
        block += c;
        advance();
    }
    return block;
}

std::vector<Token> Lexer::peekRest(size_t maxTokens, int startDepth) {
    const size_t oldPos = position;
    const int oldLine = currentLine;
    const int oldColumn = currentColumn;
    std::vector<Token> out;
    int depth = startDepth;
    while (out.size() < maxTokens) {
        Token t = getNextToken();
        out.push_back(t);
        if (t.type == GTokenType::EndOfFile) break;
        if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
        else if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) {
            if (--depth < 0) break;
        } else if (depth == 0 && t.type == GTokenType::Semicolon) break;
    }
    position = oldPos;
    currentLine = oldLine;
    currentColumn = oldColumn;
    return out;
}

Token Lexer::peekNextToken() {
    size_t oldPos = position;
    int oldLine = currentLine;
    int oldColumn = currentColumn;
    Token t = getNextToken();
    position = oldPos;
    currentLine = oldLine;
    currentColumn = oldColumn;
    return t;
}
std::vector<std::string> Lexer::keywordNames() {
    Lexer probe("");
    std::vector<std::string> names;
    names.reserve(probe.keywords.size());
    for (const auto& kv : probe.keywords) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    return names;
}

std::string Lexer::unescape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c != '\\' || i + 1 >= raw.size()) { out += c; continue; }
        const char e = raw[++i];
        switch (e) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case '0': out += '\0'; break;
            case 'a': out += '\a'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'v': out += '\v'; break;
            case '\\': out += '\\'; break;
            case '"': out += '"'; break;
            case '\'': out += '\''; break;
            case 'x': {
                int v = 0, n = 0;
                while (n < 2 && i + 1 < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[i + 1]))) {
                    const char h = raw[++i];
                    v = v * 16 + (std::isdigit(static_cast<unsigned char>(h)) ? h - '0'
                                  : std::tolower(static_cast<unsigned char>(h)) - 'a' + 10);
                    ++n;
                }
                if (n == 0) out += "\\x"; else out += static_cast<char>(v);
                break;
            }
            default: out += '\\'; out += e; break;  // escape desconhecido: fica como está
        }
    }
    return out;
}
