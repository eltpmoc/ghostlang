import sys

content = open("src/Interpreter.cpp").read()

# We need to find the place where evaluateStringExpression should end.
# It currently ends around:
#        } else if (t.type == TokenType::Comma) {
#            flushMath();
# void GhostInterpreter::parsePrint(Lexer& lex) {
#    Token nameToken = lex.getNextToken();

idx1 = content.find("        } else if (t.type == TokenType::Comma) {")
idx2 = content.find("    Token nameToken = lex.getNextToken();", idx1)

if idx1 == -1 or idx2 == -1:
    print("Could not find anchor points")
    sys.exit(1)

new_content = content[:idx1] + """        } else if (t.type == TokenType::Comma) {
            flushMath();
        } else {
            mathTokens.push_back(t);
        }
    }
    flushMath();
    return result;
}

void GhostInterpreter::parsePrint(Lexer& lex) {
    Token openParen = lex.getNextToken();
    if (openParen.type != TokenType::LParen) throw std::runtime_error("Esperado '(' apos print.");
    
    std::vector<Token> exprTokens;
    int pDepth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == TokenType::EndOfFile) throw std::runtime_error("Faltando ')'.");
        
        if (t.type == TokenType::LParen) pDepth++;
        if (t.type == TokenType::RParen) {
            pDepth--;
            if (pDepth == 0) break;
        }
        
        exprTokens.push_back(t);
    }
    
    std::cout << evaluateStringExpression(exprTokens) << std::endl;
    
    Token semi = lex.getNextToken();
    if (semi.type != TokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
}

void GhostInterpreter::parseIf(Lexer& lex) {
    Token openParen = lex.getNextToken();
    if (openParen.type != TokenType::LParen) throw std::runtime_error("Esperado '(' apos if.");
    
    std::vector<Token> condTokens;
    int parenDepth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == TokenType::LParen) parenDepth++;
        if (t.type == TokenType::RParen) parenDepth--;
        if (parenDepth == 0) break;
        condTokens.push_back(t);
    }
    
    Token openBrace = lex.getNextToken();
    if (openBrace.type != TokenType::LBrace) throw std::runtime_error("Esperado '{' apos condicao if.");
    
    std::string blockCode = lex.extractBlock();
    
    bool conditionMet = evaluateCondition(condTokens);
    if (conditionMet) {
        executeLine(blockCode);
    }
    
    Token next = lex.peekNextToken();
    if (next.type == TokenType::KwElse) {
        lex.getNextToken(); // consume 'else'
        Token openElseBrace = lex.getNextToken();
        if (openElseBrace.type != TokenType::LBrace) throw std::runtime_error("Esperado '{' apos else.");
        std::string elseBlock = lex.extractBlock();
        if (!conditionMet) {
            executeLine(elseBlock);
        }
    }
}

void GhostInterpreter::parseWhile(Lexer& lex) {
    Token openParen = lex.getNextToken();
    if (openParen.type != TokenType::LParen) throw std::runtime_error("Esperado '(' apos while.");
    
    std::vector<Token> condTokens;
    int parenDepth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == TokenType::LParen) parenDepth++;
        if (t.type == TokenType::RParen) parenDepth--;
        if (parenDepth == 0) break;
        condTokens.push_back(t);
    }
    
    Token openBrace = lex.getNextToken();
    if (openBrace.type != TokenType::LBrace) throw std::runtime_error("Esperado '{' apos condicao while.");
    
    std::string blockCode = lex.extractBlock();
    
    while (evaluateCondition(condTokens)) {
        executeLine(blockCode);
    }
}

void GhostInterpreter::parseFor(Lexer& lex) {
    Token openParen = lex.getNextToken();
    if (openParen.type != TokenType::LParen) throw std::runtime_error("Esperado '(' apos for.");
    
    std::vector<Token> initTokens;
    std::vector<Token> condTokens;
    std::vector<Token> incTokens;
    
    int stage = 0;
    int parenDepth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == TokenType::LParen) parenDepth++;
        if (t.type == TokenType::RParen) parenDepth--;
        if (parenDepth == 0) break;
        
        if (t.type == TokenType::Semicolon && stage < 2) {
            stage++;
            continue;
        }
        
        if (stage == 0) initTokens.push_back(t);
        else if (stage == 1) condTokens.push_back(t);
        else if (stage == 2) incTokens.push_back(t);
    }
    
    Token openBrace = lex.getNextToken();
    if (openBrace.type != TokenType::LBrace) throw std::runtime_error("Esperado '{' apos condicao for.");
    
    std::string blockCode = lex.extractBlock();
    
    std::string initCode = "";
    for (auto& t : initTokens) {
        if (t.type == TokenType::String) initCode += "\"" + t.value + "\" ";
        else initCode += t.value + " ";
    }
    initCode += ";";
    
    std::string incCode = "";
    for (auto& t : incTokens) {
        if (t.type == TokenType::String) incCode += "\"" + t.value + "\" ";
        else incCode += t.value + " ";
    }
    incCode += ";";
    
    executeLine(initCode);
    
    while (evaluateCondition(condTokens)) {
        executeLine(blockCode);
        executeLine(incCode);
    }
}

void GhostInterpreter::parseFunction(Lexer& lex) {
    Token retType = lex.getNextToken();
""" + content[idx2:]

with open("src/Interpreter.cpp", "w") as f:
    f.write(new_content)

print("Interpreter.cpp fixed.")
