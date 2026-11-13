#ifndef LEXER_HPP
#define LEXER_HPP

#include "Token.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
private:
    std::string source;
    size_t position;
    int currentLine;
    int currentColumn;
    int tokenLine = 1, tokenColumn = 1;   // onde começa o token atual (diagnósticos)
    std::unordered_map<std::string, GTokenType> keywords;
    // Modo compilador: literais 2.0i / 3.5g viram chamadas C++ (ghost_i / ghost_g).
    // O REPL deixa desligado e continua recebendo os tokens originais.
    bool cppLiterals = false;

    char peek(int offset = 0) const;
    char advance();
    void skipWhitespaceAndComments();
    Token createToken(GTokenType type, const std::string& value);
    std::string fstringToCpp(const std::string& text, char quote) const;
    static std::string cppStringLiteral(const std::string& text, char quote);

public:
    Lexer(const std::string& sourceCode);
    Token getNextToken();
    Token peekNextToken();
    // Próximos tokens (sem consumir) até ';' fora de parênteses, um fechamento sem par ou o fim
    std::vector<Token> peekRest(size_t maxTokens = 512, int startDepth = 0);
    std::string extractBlock();
    size_t getPosition() const { return position; }
    void setPosition(size_t pos) { position = pos; }
    std::string getSource() const { return source; }
    void setCppLiterals(bool enabled) { cppLiterals = enabled; }
    void setLine(int line) { currentLine = line; }   // trecho de ${...}: linhas contadas a partir da f-string
};

#endif