#include "../include/Interpreter.hpp"
#include "../include/GhostModules.hpp"
#include "../include/GhostDevice.hpp"
#include <filesystem>
#include <fstream>
#include "../include/GhostFormat.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

using namespace std;

void GhostInterpreter::repl() {
    bool isInteractive = ISATTY(FILENO(stdin));
    
    if (isInteractive) {
        cout << "GhostLang REPL v1.0 (Interactive Mode)" << endl;
        cout << "Digite 'exit' para sair." << endl;
    }
    
    string line;
    string currentBlock = "";
    int braceDepth = 0;
    bool inComment = false;
    while (true) {
        if (isInteractive) {
            if (braceDepth == 0 && !inComment) cout << "ghost> ";
            else cout << ".... > ";
        }
        
        if (!getline(cin, line)) {
            cout << endl; // EOF handled
            break;
        }
        
        if (braceDepth == 0 && !inComment && (line == "exit" || line == "exit()")) {
            break;
        }
        
        if (braceDepth == 0 && !inComment && (line == "clear" || line == "clear()")) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            cout << "GhostLang REPL v1.0 (Interactive Mode)" << endl;
            cout << "Digite 'exit' para sair." << endl;
            continue;
        }
        
        if (braceDepth == 0 && !inComment && (line.empty() || line.substr(0, 2) == "//")) {
            continue;
        }
        
        for (size_t i = 0; i < line.length(); i++) {
            if (!inComment) {
                if (line[i] == '/' && i+1 < line.length() && line[i+1] == '*') {
                    inComment = true;
                    i++;
                } else if (line[i] == '{') {
                    braceDepth++;
                } else if (line[i] == '}') {
                    braceDepth--;
                } else if (line[i] == '/' && i+1 < line.length() && line[i+1] == '/') {
                    break;
                }
            } else {
                if (line[i] == '*' && i+1 < line.length() && line[i+1] == '/') {
                    inComment = false;
                    i++;
                }
            }
        }
        
        currentBlock += line + "\n";
        
        if (braceDepth == 0 && !inComment) {
            if (!currentBlock.empty()) {
                try {
                    executeLine(currentBlock);
                } catch (const exception& e) {
                    cerr << "[Erro de Execucao] " << ghost_module_pretty(e.what()) << endl;
                } catch (const GhostReturnSignal&) {
                    // return fora de funcao: encerra o bloco
                } catch (const GhostBreakSignal&) {
                    cerr << "[Erro de Execucao] break fora de laco ou switch" << endl;
                } catch (const GhostContinueSignal&) {
                    cerr << "[Erro de Execucao] continue fora de laco" << endl;
                }
                currentBlock = "";
            }
        }
    }
}

void GhostInterpreter::executeLine(const string& line) {
    Lexer lex(line);
    while (true) {
        Token current = lex.getNextToken();
        if (current.type == GTokenType::EndOfFile) break;
        if (current.type == GTokenType::Semicolon) continue;
        
        if (current.type == GTokenType::KwState) {
            parseState(lex);
        } else if (current.type == GTokenType::KwGate) {
            parseGate(lex);
        } else if (current.type == GTokenType::KwShow) {
            parseShow(lex);
        } else if (current.type == GTokenType::KwLoad) {
            parseLoad(lex);
        } else if (current.type == GTokenType::KwActivate) {
            parseActivate(lex);
        } else if (current.type == GTokenType::KwApply) {
            parseApply(lex);
        } else if (current.type == GTokenType::KwNetwork) {
            parseNetwork(lex);
        } else if (current.type == GTokenType::KwPragma) {
            Token devTok = lex.getNextToken();
            std::string dev = devTok.value;
            if (dev == "gpu") dev = "cuda";
            if (dev != "cuda" && dev != "cpu" && dev != "auto") {
                throw std::runtime_error("pragma desconhecido: use 'pragma gpu;', 'pragma cpu;' ou 'pragma auto;'");
            }
            ghost::set_device(dev);
            if (lex.peekNextToken().type == GTokenType::Semicolon) lex.getNextToken();
        } else if (current.type == GTokenType::KwVar || current.type == GTokenType::KwInt || current.type == GTokenType::KwDouble || current.type == GTokenType::KwFloat || current.type == GTokenType::KwString || current.type == GTokenType::KwDate || current.type == GTokenType::KwTime || current.type == GTokenType::KwDatetime || current.type == GTokenType::KwJson || current.type == GTokenType::KwGhost || current.type == GTokenType::KwComplex || current.type == GTokenType::KwLlm || current.type == GTokenType::KwBool || current.type == GTokenType::KwList || current.type == GTokenType::KwMap || current.type == GTokenType::KwTextModel) {
            // var m = img.conv2d(k, 1); / var u = img.transpose(); : o resultado é uma matriz,
            // como no compilador (var vira auto); segue o mesmo caminho de 'gate m = ...'.
            bool matrizVar = false;
            if (current.type == GTokenType::KwVar) {
                const auto ahead = lex.peekRest(8);
                static const std::set<std::string> metodosMatriz = {
                    "transpose", "conv2d", "max_pool2d", "avg_pool2d", "conv2d_grad_kernel", "conv2d_grad_input"};
                matrizVar = ahead.size() >= 5 && ahead[0].type == GTokenType::Identifier &&
                    ahead[1].type == GTokenType::Assign && memory.count(ahead[2].value) &&
                    !type_memory.count(ahead[2].value) && ahead[3].type == GTokenType::Dot &&
                    metodosMatriz.count(ahead[4].value);
            }
            if (current.type == GTokenType::KwTextModel) {
                // textmodel ia = text_model(...): no REPL o objeto nativo fica numa variável comum
                if (!tryDeclaration(GTokenType::KwVar, lex)) parseVar(lex, GTokenType::KwVar);
            } else if (matrizVar) {
                const std::string alvo = lex.getNextToken().value;
                lex.getNextToken(); // '='
                parseMathAssignment(alvo, lex);
            } else if (current.type == GTokenType::KwGhost) {
                parseVar(lex, current.type);
            } else if (!tryDeclaration(current.type, lex)) {
                parseVar(lex, current.type);
            }
        } else if (current.type == GTokenType::KwBreak || current.type == GTokenType::KwContinue) {
            const bool isBreak = current.type == GTokenType::KwBreak;
            if (lex.peekNextToken().type == GTokenType::Semicolon) lex.getNextToken();
            if (isBreak) throw GhostBreakSignal{};
            throw GhostContinueSignal{};
        } else if (current.type == GTokenType::KwTry) {
            parseTryRepl(lex);
        } else if (current.type == GTokenType::KwThrow) {
            parseThrowRepl(lex);
        } else if (current.type == GTokenType::Increment || current.type == GTokenType::Decrement) {
            Token name = lex.getNextToken();
            if (name.type != GTokenType::Identifier || !tryCoreStatement(name, current, lex)) throw std::runtime_error("use ++nome; ou nome++;");
        } else if (current.type == GTokenType::KwImport) {
            importModule(lex);
        } else if (current.type == GTokenType::KwClass) {
            parseClassDef(lex);
        } else if (current.type == GTokenType::KwThis) {
            Token next = lex.getNextToken();
            if (!tryLvalueStatement(current, next, lex)) throw std::runtime_error("use this.campo = valor; ou this.metodo(...); dentro de metodos");
        } else if (current.type == GTokenType::KwSuper || current.type == GTokenType::KwNew ||
                   current.type == GTokenType::LParen || current.type == GTokenType::LBracket) {
            // super.metodo(...);  new Classe(...);  ((x) => { ... })(42);  [&x]() => { ... };  como comando
            vector<Token> expr{current};
            int depth = (current.type == GTokenType::LParen || current.type == GTokenType::LBracket) ? 1 : 0;
            while (true) {
                Token t = lex.peekNextToken();
                if (t.type == GTokenType::EndOfFile) break;
                if (depth == 0 && t.type == GTokenType::Semicolon) { lex.getNextToken(); break; }
                if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
                if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) depth--;
                expr.push_back(lex.getNextToken());
            }
            evalValue(expr);
        } else if (current.type == GTokenType::KwElse) {
            throw std::runtime_error("else sem if correspondente");
        } else if (current.type == GTokenType::KwPrint) {
            parsePrint(lex);
        } else if (current.type == GTokenType::KwIf) {
            parseIf(lex);
        } else if (current.type == GTokenType::KwWhile) {
            parseWhile(lex);
        } else if (current.type == GTokenType::KwDo) {
            parseDoWhile(lex);
        } else if (current.type == GTokenType::KwFor) {
            parseFor(lex);
        } else if (current.type == GTokenType::KwForeach) {
            parseForeach(lex);
        } else if (current.type == GTokenType::KwHypothesis) {
            parseHypothesis(lex);
        } else if (current.type == GTokenType::KwCollapseMax) {
            Token lParen = lex.getNextToken();
            if (lParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos collapse_max");
            vector<Token> exprTokens;
            while(true) {
                Token t = lex.getNextToken();
                if (t.type == GTokenType::RParen) break;
                exprTokens.push_back(t);
            }
            double val = evaluateMath(exprTokens);
            numeric_memory["__collapse_score__"] = val;
            numeric_memory["__collapse_type__"] = 1.0; // max
            Token semi = lex.getNextToken();
            if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos collapse_max");
        } else if (current.type == GTokenType::KwCollapseMin) {
            Token lParen = lex.getNextToken();
            if (lParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos collapse_min");
            vector<Token> exprTokens;
            while(true) {
                Token t = lex.getNextToken();
                if (t.type == GTokenType::RParen) break;
                exprTokens.push_back(t);
            }
            double val = evaluateMath(exprTokens);
            numeric_memory["__collapse_score__"] = val;
            numeric_memory["__collapse_type__"] = -1.0; // min
            Token semi = lex.getNextToken();
            if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos collapse_min");
        } else if (current.type == GTokenType::KwSpawn) {
            parseSpawn(lex);
        } else if (current.type == GTokenType::KwSleep) {
            parseSleep(lex);
        } else if (current.type == GTokenType::KwMutex) {
            parseMutex(lex);
        } else if (current.type == GTokenType::KwLock) {
            parseLock(lex);
        } else if (current.type == GTokenType::KwUnlock) {
            parseUnlock(lex);
        } else if (current.type == GTokenType::KwServer) {
            parseServer(lex);
        } else if (current.type == GTokenType::KwSwitch) {
            parseSwitch(lex);
        } else if (current.type == GTokenType::KwFunction) {
            parseFunction(lex);
        } else if (current.type == GTokenType::KwReturn) {
            parseReturn(lex);
        } else if (current.type == GTokenType::KwQreg || current.type == GTokenType::KwQcircuit || current.type == GTokenType::KwQlayer) {
            parseQuantumDecl(lex, current.type);
        } else if (current.type == GTokenType::KwCircuit) {
            parseCircuitDef(lex);
        } else if (current.type == GTokenType::Identifier || current.type == GTokenType::KwInput) {
            string objName = current.value;
            Token next = lex.getNextToken();
            // interface Nome { function tipo metodo(params); }
            if (objName == "interface" && next.type == GTokenType::Identifier) {
                parseInterfaceDef(next, lex);
                continue;
            }
            auto consumeArraySuffix = [&](std::string& specOut) {
                while (lex.peekNextToken().type == GTokenType::LBracket) {
                    const size_t sv = lex.getPosition();
                    lex.getNextToken();
                    if (lex.getNextToken().type == GTokenType::RBracket) {
                        specOut = "list<" + specOut + ">";
                    } else {
                        lex.setPosition(sv);
                        break;
                    }
                }
            };
            // Pilha<int> p [= ...];  Colecao<int> c [= ...];  Pilha<int>[] ps [= ...];
            // Tambem trata Classe<T>.staticCampo = ...; e Classe<T>.staticMetodo(...);
            if ((generic_memory.count(objName) || generic_iface_memory.count(objName)) && next.type == GTokenType::Lt) {
                std::string spec = readResultSpec(lex, true, objName);
                if (lex.peekNextToken().type == GTokenType::Dot) {
                    ensureTypeReady(spec);
                    std::vector<Token> stmtTokens;
                    stmtTokens.push_back(Token{GTokenType::Identifier, spec, current.line, current.column});
                    stmtTokens.push_back(lex.getNextToken()); // '.'
                    int depth = 0;
                    while (true) {
                        Token t = lex.peekNextToken();
                        if (t.type == GTokenType::EndOfFile) break;
                        if (depth == 0 && t.type == GTokenType::Semicolon) { lex.getNextToken(); break; }
                        if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
                        if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) depth--;
                        stmtTokens.push_back(lex.getNextToken());
                    }
                    // Verifica se eh atribuicao em membro static: Contador<int>.total = ...;
                    auto compoundOp = [](GTokenType t) {
                        switch (t) {
                            case GTokenType::PlusAssign: return GTokenType::Plus;
                            case GTokenType::MinusAssign: return GTokenType::Minus;
                            case GTokenType::StarAssign: return GTokenType::Star;
                            case GTokenType::SlashAssign: return GTokenType::Slash;
                            case GTokenType::PercentAssign: return GTokenType::Percent;
                            default: return GTokenType::Unknown;
                        }
                    };
                    size_t op = std::string::npos;
                    int d = 0;
                    for (size_t k = 2; k < stmtTokens.size(); ++k) {
                        const auto ty = stmtTokens[k].type;
                        if (ty == GTokenType::LParen || ty == GTokenType::LBracket || ty == GTokenType::LBrace) d++;
                        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket || ty == GTokenType::RBrace) d--;
                        else if (d == 0 && (ty == GTokenType::Assign || compoundOp(ty) != GTokenType::Unknown)) { op = k; break; }
                        else if (d == 0 && (ty == GTokenType::Increment || ty == GTokenType::Decrement) && k + 1 == stmtTokens.size()) { op = k; break; }
                    }
                    if (op != std::string::npos && stmtTokens.size() >= 3 && stmtTokens[2].type == GTokenType::Identifier) {
                        const std::string member = stmtTokens[2].value;
                        std::string owner;
                        const GhostFieldDef* sfd = findStatic(spec, member, &owner);
                        if (!sfd) throw std::runtime_error("a classe " + spec + " nao tem o campo static " + member);
                        checkAccess(owner, sfd->priv, sfd->prot, "campo static " + member);
                        const std::string key = owner + "::" + member;
                        GhostValue& target = static_memory[key];
                        const auto opType = stmtTokens[op].type;
                        if (opType == GTokenType::Increment || opType == GTokenType::Decrement) {
                            target = applyBinary(opType == GTokenType::Increment ? GTokenType::Plus : GTokenType::Minus, target, GhostValue::number(1, true));
                        } else {
                            std::vector<Token> rhs(stmtTokens.begin() + static_cast<long long>(op + 1), stmtTokens.end());
                            GhostValue val = evalValue(rhs);
                            if (opType == GTokenType::Assign) {
                                target = coerce(val, sfd->type, sfd->cls, member);
                            } else {
                                target = coerce(applyBinary(compoundOp(opType), target, val), sfd->type, sfd->cls, member);
                            }
                        }
                        continue;
                    }
                    evalValue(stmtTokens);
                    continue;
                }
                ensureTypeReady(spec);
                consumeArraySuffix(spec);
                Token nameTok = lex.getNextToken();
                if (nameTok.type != GTokenType::Identifier) throw std::runtime_error("esperado o nome da variavel depois de " + spec);
                tryClassVarDecl(spec, nameTok, lex);
                continue;
            }
            if (generic_memory.count(objName) && next.type == GTokenType::Identifier)
                throw std::runtime_error(objName + " e uma classe generica: use " + objName + "<tipo> " + next.value);
            if (generic_iface_memory.count(objName) && next.type == GTokenType::Identifier)
                throw std::runtime_error(objName + " e uma interface generica: use " + objName + "<tipo> " + next.value);
            // Classe nome [= valor];   Interface nome [= objeto];   Classe[] xs [= ...];   Interface[] fs [= ...];
            if (class_memory.count(objName) || interface_memory.count(objName)) {
                if (next.type == GTokenType::Identifier) {
                    tryClassVarDecl(objName, next, lex);
                    continue;
                }
                if (next.type == GTokenType::LBracket && lex.peekNextToken().type == GTokenType::RBracket) {
                    lex.getNextToken(); // ']'
                    std::string spec = "list<" + objName + ">";
                    consumeArraySuffix(spec);
                    Token nameTok = lex.getNextToken();
                    if (nameTok.type != GTokenType::Identifier) throw std::runtime_error("esperado o nome da variavel depois de " + objName + "[]");
                    tryClassVarDecl(spec, nameTok, lex);
                    continue;
                }
            }
            // result<T> nome [= ...];  set<T> nome [= ...];  fn<Ret(Args)> nome [= ...];  (+ sufixo [])
            const bool isFnKw = objName == "fn" && !class_memory.count("fn") && !interface_memory.count("fn");
            if ((objName == "result" || objName == "set" || isFnKw) && next.type == GTokenType::Lt) {
                std::string spec = readResultSpec(lex, true, objName);
                consumeArraySuffix(spec);
                Token nameTok = lex.getNextToken();
                if (nameTok.type != GTokenType::Identifier) throw std::runtime_error("esperado o nome da variavel depois de " + spec);
                tryClassVarDecl(spec, nameTok, lex);
                continue;
            }
            if (isFnKw && next.type == GTokenType::Identifier)
                throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)> " + next.value);
            // circuit: bell(q, 0, 1);  e, dentro do corpo de um circuit, h(a); cx(a, b); outro(a, b);
            if (tryCircuitStatement(current, next, lex)) continue;
            // Objeto quantico: q.h(0);  c.fit(60, 0.1);  quantum_device("cuda");
            if ((isQuantum(objName) && next.type == GTokenType::Dot && !fieldRef(objName)) ||
                ((objName == "quantum_device" || objName == "quantum_backend") && next.type == GTokenType::LParen)) {
                vector<Token> call{current, next};
                while (true) {
                    Token t = lex.getNextToken();
                    if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
                    call.push_back(t);
                }
                size_t pos = 0;
                evalQuantumAt(call, pos);
                if (pos + 1 < call.size()) throw std::runtime_error("Esperado ';' apos a chamada de " + objName + ".");
                continue;
            }
            if (tryServerMethod(objName, next, lex)) continue;
            if (tryHybridStatement(objName, next, lex)) continue;
            // Caminhos: obj.campo = e; obj.metodo(); xs[i][j] += 1; campo.push(1); (src/InterpreterCore.cpp)
            if (tryLvalueStatement(current, next, lex)) continue;
            // Nucleo: x = e; x += e; x++; xs[i] = e; xs.push(1); f(1); (src/InterpreterCore.cpp)
            if (tryCoreStatement(current, next, lex)) continue;
            if (next.type == GTokenType::Arrow) {
                parseOptimize(objName, lex);
            } else if (memory.count(objName) && next.type == GTokenType::LParen) {
                // Indexacao 2D ou 1D: X(0, k) = a; ou X(k) = a;
                vector<Token> rTokens, cTokens;
                bool inSecond = false;
                int parenNest = 0;
                while (true) {
                    Token t = lex.getNextToken();
                    if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Indexacao de matriz incompleta");
                    if (t.type == GTokenType::LParen) parenNest++;
                    else if (t.type == GTokenType::RParen) {
                        if (parenNest > 0) parenNest--;
                        else break;
                    } else if (t.type == GTokenType::Comma && parenNest == 0) {
                        inSecond = true;
                        continue;
                    }
                    if (inSecond) cTokens.push_back(t);
                    else rTokens.push_back(t);
                }
                Token assignTok = lex.getNextToken();
                if (assignTok.type != GTokenType::Assign) {
                    throw std::runtime_error("Esperado '=' apos indexacao da matriz " + objName);
                }
                vector<Token> valTokens;
                while (true) {
                    Token t = lex.getNextToken();
                    if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
                    valTokens.push_back(t);
                }
                double val = evaluateMath(valTokens);
                GhostMatrix& mat = memory[objName];
                if (inSecond) {
                    int r = (int)evaluateMath(rTokens);
                    int c = (int)evaluateMath(cTokens);
                    if (r < 0 || r >= (int)mat.getRows() || c < 0 || c >= (int)mat.getCols()) {
                        throw std::runtime_error("Indice (" + std::to_string(r) + ", " + std::to_string(c) + ") fora dos limites da matriz " + objName);
                    }
                    mat(r, c) = GhostNumberG(val, 0.0, 0.0);
                } else {
                    int k = (int)evaluateMath(rTokens);
                    if (k < 0 || k >= (int)mat.getCols()) {
                        throw std::runtime_error("Indice " + std::to_string(k) + " fora dos limites da matriz " + objName);
                    }
                    mat(0, k) = GhostNumberG(val, 0.0, 0.0);
                }
                continue;
            } else if (next.type == GTokenType::LBracket) {
                Token indexToken = lex.getNextToken();
                int idx = 0;
                if (indexToken.type == GTokenType::Number) {
                    idx = std::stoi(indexToken.value);
                } else if (indexToken.type == GTokenType::Identifier && numeric_memory.count(indexToken.value)) {
                    idx = (int)numeric_memory[indexToken.value];
                } else {
                    throw std::runtime_error("Indice de array invalido.");
                }
                Token rBracket = lex.getNextToken();
                if (rBracket.type != GTokenType::RBracket) throw std::runtime_error("Esperado ']' apos indice.");
                
                Token assignToken = lex.getNextToken();
                if (assignToken.type != GTokenType::Assign) throw std::runtime_error("Esperado '=' apos indice do array.");
                
                vector<Token> exprTokens;
                while (true) {
                    Token t = lex.getNextToken();
                    if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
                    exprTokens.push_back(t);
                }
                double val = evaluateMath(exprTokens);
                
                if (memory.count(objName)) {
                    GhostMatrix& arr = memory[objName];
                    if (idx < 0 || idx >= arr.getCols()) throw std::runtime_error("Indice fora dos limites do array.");
                    arr(0, idx) = GhostNumberG(val, 0.0, 0);
                } else {
                    throw std::runtime_error("Array '" + objName + "' nao encontrado.");
                }
            } else if (next.type == GTokenType::Assign) {
                // Check if the identifier is a primitive var assignment
                if (numeric_memory.count(objName) > 0) {
                    vector<Token> exprTokens;
                    bool hasInput = false;
                    string promptStr = "";
                    while (true) {
                        Token t = lex.getNextToken();
                        if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
                        if (t.type == GTokenType::KwInput) {
                            hasInput = true;
                            Token lParen = lex.getNextToken();
                            Token strToken = lex.getNextToken();
                            Token rParen = lex.getNextToken();
                            if (strToken.type == GTokenType::String) promptStr = strToken.value;
                        } else {
                            exprTokens.push_back(t);
                        }
                    }
                    if (hasInput) {
                        cout << promptStr;
                        string buffer;
                        getline(cin, buffer);
                        double val = 0.0;
                        try { val = stod(buffer); } catch (...) {}
                        if (type_memory[objName] == GTokenType::KwInt) numeric_memory[objName] = (int)val;
                        else numeric_memory[objName] = val;
                    } else {
                        double val = evaluateMath(exprTokens);
                        if (type_memory[objName] == GTokenType::KwInt) {
                            numeric_memory[objName] = (int)val;
                        } else {
                            numeric_memory[objName] = val;
                        }
                    }
                } else if (string_memory.count(objName) > 0) {
                    Token t = lex.getNextToken();
                    if (t.type == GTokenType::KwInput) {
                        Token lParen = lex.getNextToken();
                        Token strToken = lex.getNextToken();
                        Token rParen = lex.getNextToken();
                        cout << strToken.value;
                        string buffer;
                        getline(cin, buffer);
                        string_memory[objName] = buffer;
                        lex.getNextToken(); // consume ;
                    } else {
                        string_memory[objName] = t.value;
                        lex.getNextToken(); // consume ;
                    }
                } else {
                    parseMathAssignment(objName, lex);
                }
            } else {
                cerr << "[Erro de Sintaxe] Operacao desconhecida apos '" << objName << "'." << endl;
            }
        } else {
            cerr << "[Erro de Sintaxe] Comando '" << current.value << "' invalido no modo REPL." << endl;
        }
    }
}

void GhostInterpreter::parseState(Lexer& lex) {
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw runtime_error("Esperado nome do state.");
    if (lex.getNextToken().type != GTokenType::Assign) throw runtime_error("Esperado '='.");
    
    Token next = lex.getNextToken();
    if (next.type == GTokenType::LBracket) {
        vector<double> vals;
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::RBracket || t.type == GTokenType::EndOfFile) break;
            if (t.type == GTokenType::Comma) continue;
            if (t.type == GTokenType::Minus) {
                Token numTok = lex.getNextToken();
                vals.push_back(-stod(numTok.value));
            } else if (t.type == GTokenType::Number) {
                vals.push_back(stod(t.value));
            } else {
                throw runtime_error("state precisa de elementos numericos.");
            }
        }
        if (vals.empty()) throw runtime_error("state nao pode ser vazio.");
        const size_t n = vals.size();
        GhostMatrix m(n, 1);
        for (size_t i = 0; i < n; ++i) {
            m(i, 0) = GhostNumberG(vals[i], 0.0, 0.0);
        }
        memory[nameToken.value] = m;
        cout << "[Memoria] State '" << nameToken.value << "' alocado (" << n << "x1)." << endl;
    } else {
        throw runtime_error("Sintaxe invalida para state. Use state nome = [v0, v1, ...];");
    }
}

void GhostInterpreter::parseGate(Lexer& lex) {
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw runtime_error("Esperado nome do gate.");
    
    Token next = lex.getNextToken();
    
    // Novo fluxo: gate nome = operacao
    if (next.type == GTokenType::Assign) {
        Token opToken = lex.getNextToken();
        if (opToken.type == GTokenType::KwHadamard) {
            memory[nameToken.value] = GhostMatrix::Hadamard();
            cout << "[Memoria] Porta Quantica '" << nameToken.value << "' (2x2) instanciada com sucesso." << endl;
        } else if (opToken.type == GTokenType::KwPauliX) {
            memory[nameToken.value] = GhostMatrix::PauliX();
            cout << "[Memoria] Porta Quantica '" << nameToken.value << "' (2x2) instanciada com sucesso." << endl;
        } else if (opToken.type == GTokenType::KwPauliY) {
            memory[nameToken.value] = GhostMatrix::PauliY();
            cout << "[Memoria] Porta Quantica '" << nameToken.value << "' (2x2) instanciada com sucesso." << endl;
        } else if (opToken.type == GTokenType::KwPauliZ) {
            memory[nameToken.value] = GhostMatrix::PauliZ();
            cout << "[Memoria] Porta Quantica '" << nameToken.value << "' (2x2) instanciada com sucesso." << endl;
        } else {
            parseMathAssignmentLeft(nameToken.value, opToken, lex);
        }
        return;
    }

    // Fluxo antigo: gate nome(r, c)
    if (next.type != GTokenType::LParen) throw runtime_error("Esperado '(' ou '='.");
    Token rToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    Token cToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    int rows = stoi(rToken.value);
    int cols = stoi(cToken.value);
    
    GhostMatrix m(rows, cols);
    if (rows == 2 && cols == 2) {
        m(0, 0) = GhostNumberG(0.5, 0.0, 0.1);
        m(0, 1) = GhostNumberG(0.5, 0.0, -0.1);
        m(1, 0) = GhostNumberG(0.5, 0.0, 0.2);
        m(1, 1) = GhostNumberG(0.5, 0.0, -0.2);
    } else {
        double limit = std::sqrt(6.0 / (rows + cols));
        for (size_t r = 0; r < (size_t)rows; ++r) {
            for (size_t c = 0; c < (size_t)cols; ++c) {
                double val = (((double)rand() / RAND_MAX) * 2.0 - 1.0) * limit;
                m(r, c) = GhostNumberG(val, 0.0, 0.0);
            }
        }
    }
    
    memory[nameToken.value] = m;
    cout << "[Memoria] Gate '" << nameToken.value << "' alocado (" << rows << "x" << cols << ")." << endl;
}

void GhostInterpreter::parseShow(Lexer& lex) {
    if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '('.");
    Token nameToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    if (memory.find(nameToken.value) != memory.end()) {
        cout << "\nEstado da estrutura '" << nameToken.value << "':\n";
        auto& mat = memory[nameToken.value];
        if (mat.getRows() == 1 && mat.getCols() == 1 && type_memory[nameToken.value] == GTokenType::KwGhost) {
            mat(0, 0).display();
        } else {
            mat.display();
        }
    } else if (qreg_memory.count(nameToken.value)) {
        cout << "\nEstado da estrutura '" << nameToken.value << "':\n";
        qreg_memory.at(nameToken.value).display();
    } else if (qcircuit_memory.count(nameToken.value)) {
        qcircuit_memory.at(nameToken.value).display();
    } else {
        throw runtime_error("Variavel '" + nameToken.value + "' nao encontrada na memoria.");
    }
}

void GhostInterpreter::parseLoad(Lexer& lex) {
    if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '('.");
    Token nameToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    Token fileToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    if (memory.find(nameToken.value) != memory.end()) {
        memory[nameToken.value].loadCSV(fileToken.value);
        cout << "[Memoria] Dados carregados para '" << nameToken.value << "'." << endl;
    } else {
        throw runtime_error("Variavel '" + nameToken.value + "' nao encontrada na memoria.");
    }
}

void GhostInterpreter::parseActivate(Lexer& lex) {
    if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '('.");
    Token nameToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    Token funcToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    if (memory.find(nameToken.value) != memory.end()) {
        if (funcToken.type == GTokenType::KwRelu) {
            memory[nameToken.value].activateReLU();
            cout << "[Ativacao] ReLU aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.type == GTokenType::KwSigmoid) {
            memory[nameToken.value].activateSigmoid();
            cout << "[Ativacao] Sigmoid aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.type == GTokenType::KwSin) {
            memory[nameToken.value].activateSin();
            cout << "[Ativacao] Sin aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.type == GTokenType::KwCos) {
            memory[nameToken.value].activateCos();
            cout << "[Ativacao] Cos aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.type == GTokenType::KwTan || funcToken.value == "tan") {
            memory[nameToken.value].activateTan();
            cout << "[Ativacao] Tan aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.value == "tanh") {
            memory[nameToken.value].activateTanh();
            cout << "[Ativacao] Tanh aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.value == "leaky_relu") {
            memory[nameToken.value].activateLeakyReLU();
            cout << "[Ativacao] LeakyReLU aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.value == "gelu") {
            memory[nameToken.value].activateGELU();
            cout << "[Ativacao] GELU aplicada a '" << nameToken.value << "'." << endl;
        } else if (funcToken.value == "softmax") {
            memory[nameToken.value].activateSoftmax();
            cout << "[Ativacao] Softmax aplicada a '" << nameToken.value << "'." << endl;
        } else {
            throw runtime_error("Funcao de ativacao desconhecida.");
        }
    } else {
        throw runtime_error("Variavel '" + nameToken.value + "' nao encontrada na memoria.");
    }
}

void GhostInterpreter::parseApply(Lexer& lex) {
    if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '('.");
    
    Token gateToken = lex.getNextToken();
    if (gateToken.type != GTokenType::Identifier) throw runtime_error("Esperado nome do gate.");
    
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    
    Token stateToken = lex.getNextToken();
    if (stateToken.type != GTokenType::Identifier) throw runtime_error("Esperado nome do state.");
    
    if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    if (memory.find(gateToken.value) == memory.end()) {
        throw runtime_error("Porta Quantica '" + gateToken.value + "' nao encontrada.");
    }
    if (memory.find(stateToken.value) == memory.end()) {
        throw runtime_error("Estado '" + stateToken.value + "' nao encontrado.");
    }
    
    // Executando o Colapso / Evolução do Estado
    // estado_novo = Porta * estado_atual
    memory[stateToken.value] = memory[gateToken.value] * memory[stateToken.value];
    cout << "[Colapso Quantico] Porta '" << gateToken.value << "' aplicada sobre '" << stateToken.value << "'." << endl;
}

void GhostInterpreter::parseOptimize(const string& objName, Lexer& lex) {
    if (lex.getNextToken().type != GTokenType::KwOptimize) throw runtime_error("Esperado 'optimize'.");
    if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '('.");
    
    Token initToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    Token targetToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    
    if (lex.getNextToken().type != GTokenType::KwEpochs) throw runtime_error("Esperado 'epochs'.");
    if (lex.getNextToken().type != GTokenType::Assign) throw runtime_error("Esperado '='.");
    Token epochsToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Comma) throw runtime_error("Esperado ','.");
    
    if (lex.getNextToken().type != GTokenType::KwLr) throw runtime_error("Esperado 'lr'.");
    if (lex.getNextToken().type != GTokenType::Assign) throw runtime_error("Esperado '='.");
    Token lrToken = lex.getNextToken();

    // Novo: Parse do Otimizador (Opcional, padrao: "adam")
    string optimizer = "adam";
    Token nextToken = lex.getNextToken();
    if (nextToken.type == GTokenType::Comma) {
        if (lex.getNextToken().type != GTokenType::KwOpt) throw runtime_error("Esperado 'opt'.");
        if (lex.getNextToken().type != GTokenType::Assign) throw runtime_error("Esperado '='.");
        Token optToken = lex.getNextToken();
        if (optToken.type != GTokenType::String) throw runtime_error("Esperado string em 'opt' (ex: \"adam\" ou \"sgd\").");
        optimizer = optToken.value;
        nextToken = lex.getNextToken();
    }
    
    if (nextToken.type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
    
    if (hybrid_memory.count(objName)) {
        if (!memory.count(initToken.value)) throw runtime_error("Entrada '" + initToken.value + "' nao encontrada.");
        if (!memory.count(targetToken.value)) throw runtime_error("Alvo '" + targetToken.value + "' nao encontrado.");
        int epochs = stoi(epochsToken.value);
        double lr = stod(lrToken.value);
        hybrid_memory[objName]->optimize(memory[initToken.value], memory[targetToken.value], epochs, lr, optimizer);
        return;
    }
    
    if (memory.find(objName) == memory.end()) throw runtime_error("Modelo '" + objName + "' nao encontrado.");
    if (memory.find(initToken.value) == memory.end()) throw runtime_error("Estado inicial '" + initToken.value + "' nao encontrado.");
    if (memory.find(targetToken.value) == memory.end()) throw runtime_error("Estado alvo '" + targetToken.value + "' nao encontrado.");
    
    int epochs = stoi(epochsToken.value);
    double lr = stod(lrToken.value);
    
    GhostMatrix& model = memory[objName];
    GhostMatrix& init = memory[initToken.value];
    GhostMatrix& target = memory[targetToken.value];
    
    cout << "[Treino] Otimizando modelo '" << objName << "' com " << optimizer << " (" << epochs << " epocas, lr=" << lr << ")..." << endl;
    
    // Backpropagation Nativo C++ dinâmico
    for (int epoch = 1; epoch <= epochs; ++epoch) {
        // Forward Pass
        GhostMatrix output = model * init;
        
        // Loss & Error = (Output - Target)
        GhostMatrix error = output - target;
        
        // Backward Pass (Gradiente) = Error * Input^T
        GhostMatrix grad = error * init.transpose();
        
        // Weight Update (Otimizadores)
        if (optimizer == "sgd") {
            for (size_t i = 0; i < model.getRows(); ++i) {
                for (size_t j = 0; j < model.getCols(); ++j) {
                    model(i, j).x -= lr * grad(i, j).x;
                }
            }
        } else if (optimizer == "adam") {
            model.adam_t += 1;
            double beta1 = 0.9;
            double beta2 = 0.999;
            double epsilon = 1e-8;
            for (size_t i = 0; i < model.getRows(); ++i) {
                for (size_t j = 0; j < model.getCols(); ++j) {
                    double g = grad(i, j).x;
                    
                    // Momentos
                    model.adam_m[i][j] = beta1 * model.adam_m[i][j] + (1.0 - beta1) * g;
                    model.adam_v[i][j] = beta2 * model.adam_v[i][j] + (1.0 - beta2) * g * g;
                    
                    // Correcao de vies
                    double m_hat = model.adam_m[i][j] / (1.0 - std::pow(beta1, model.adam_t));
                    double v_hat = model.adam_v[i][j] / (1.0 - std::pow(beta2, model.adam_t));
                    
                    // Atualiza pesos
                    model(i, j).x -= lr * m_hat / (std::sqrt(v_hat) + epsilon);
                }
            }
        } else {
            throw runtime_error("Otimizador desconhecido: " + optimizer);
        }
    }
    
    cout << "[Treino] Concluido com sucesso!" << endl;
}


void GhostInterpreter::parseNetwork(Lexer& lex) {
    Token netToken = lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::Assign) throw runtime_error("Esperado '='.");
    
    vector<string> layers;
    layers.push_back(lex.getNextToken().value);
    
    while (true) {
        Token n = lex.getNextToken();
        if (n.type == GTokenType::EndOfFile || n.type == GTokenType::Semicolon) break;
        if (n.type == GTokenType::Chain) {
            layers.push_back(lex.getNextToken().value);
        } else {
            break;
        }
    }
    
    if (layers.empty()) throw runtime_error("Rede nao definida corretamente.");
    
    std::vector<GhostLayerRef> layerRefs;
    for (const auto& l : layers) {
        if (qlayer_memory.count(l)) {
            layerRefs.push_back(qlayer_memory[l].get());
        } else if (memory.count(l)) {
            layerRefs.push_back(&memory[l]);
        }
    }
    
    if (!layerRefs.empty()) {
        auto hnet = std::make_shared<GhostHybridNetwork>(netToken.value, layerRefs);
        hybrid_memory[netToken.value] = hnet;
        cout << "[Ghost OS] Rede hibrida '" << netToken.value << "' montada: " << hnet->summary() << endl;
    } else {
        cout << "[Memoria] Rede Profunda '" << netToken.value << "' estruturada com " << layers.size() << " camadas no REPL." << endl;
    }
}

void GhostInterpreter::parseMathAssignment(const string& targetName, Lexer& lex) {
    Token leftToken = lex.getNextToken();
    parseMathAssignmentLeft(targetName, leftToken, lex);
}

void GhostInterpreter::parseMathAssignmentLeft(const string& targetName, const Token& leftToken, Lexer& lex) {
    // Suporte para Transposição: c = transpose(a)
    if (leftToken.type == GTokenType::KwTranspose) {
        if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos transpose.");
        Token objToken = lex.getNextToken();
        if (objToken.type != GTokenType::Identifier) throw runtime_error("Esperado nome da matriz.");
        if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
        
        if (memory.find(objToken.value) == memory.end()) {
            throw runtime_error("Variavel '" + objToken.value + "' nao encontrada.");
        }
        memory[targetName] = memory[objToken.value].transpose();
        cout << "[Ghost OS] Transposicao de '" << objToken.value << "' calculada com sucesso." << endl;  // mesma mensagem do compilador
        return;
    }

    // Suporte para Convolucao 2D: c = conv(a, k)
    if (leftToken.type == GTokenType::KwConv) {
        if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos conv.");
        Token aTok = lex.getNextToken();
        lex.getNextToken(); // ','
        Token kTok = lex.getNextToken();
        int stride = 1;
        if (lex.peekNextToken().type == GTokenType::Comma) {
            lex.getNextToken();
            stride = std::stoi(lex.getNextToken().value);
        }
        lex.getNextToken(); // ')'
        if (memory.find(aTok.value) == memory.end() || memory.find(kTok.value) == memory.end()) {
            throw runtime_error("Variaveis de matriz nao encontradas em conv().");
        }
        memory[targetName] = memory[aTok.value].conv2d(memory[kTok.value], stride);
        cout << "[Ghost OS] Convolucao ('" << kTok.value << "') aplicada sobre '" << aTok.value << "'." << endl;  // mesma mensagem do compilador
        return;
    }

    if (leftToken.type != GTokenType::Identifier) throw runtime_error("Operando esquerdo esperado.");
    
    // Suporte para m2 = m1.transpose(), m1.conv2d(k), m1.max_pool2d(), m1.avg_pool2d()
    if (lex.peekNextToken().type == GTokenType::Dot) {
        lex.getNextToken(); // consome '.'
        Token mTok = lex.getNextToken();
        if (mTok.value == "transpose") {
            if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos transpose.");
            if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
            if (memory.find(leftToken.value) == memory.end()) {
                throw runtime_error("Variavel '" + leftToken.value + "' nao encontrada.");
            }
            memory[targetName] = memory[leftToken.value].transpose();
            return;
        }
        if (mTok.value == "conv2d") {
            if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos conv2d.");
            Token kTok = lex.getNextToken();
            int stride = 1;
            if (lex.peekNextToken().type == GTokenType::Comma) {
                lex.getNextToken();
                stride = std::stoi(lex.getNextToken().value);
            }
            if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
            if (memory.find(leftToken.value) == memory.end() || memory.find(kTok.value) == memory.end()) {
                throw runtime_error("Variaveis de matriz nao encontradas em conv2d.");
            }
            memory[targetName] = memory[leftToken.value].conv2d(memory[kTok.value], stride);
            return;
        }
        if (mTok.value == "max_pool2d" || mTok.value == "avg_pool2d") {
            const bool isMax = (mTok.value == "max_pool2d");
            if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos " + mTok.value + ".");
            int psize = 2, stride = 2;
            if (lex.peekNextToken().type != GTokenType::RParen) {
                psize = std::stoi(lex.getNextToken().value);
                if (lex.peekNextToken().type == GTokenType::Comma) {
                    lex.getNextToken();
                    stride = std::stoi(lex.getNextToken().value);
                }
            }
            if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
            if (memory.find(leftToken.value) == memory.end()) {
                throw runtime_error("Variavel '" + leftToken.value + "' nao encontrada.");
            }
            memory[targetName] = isMax ? memory[leftToken.value].max_pool2d(psize, stride)
                                       : memory[leftToken.value].avg_pool2d(psize, stride);
            return;
        }
        if (mTok.value == "conv2d_grad_kernel") {
            if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos conv2d_grad_kernel.");
            Token inTok = lex.getNextToken();
            lex.getNextToken(); // ','
            Token gradTok = lex.getNextToken();
            int stride = 1;
            if (lex.peekNextToken().type == GTokenType::Comma) {
                lex.getNextToken();
                stride = std::stoi(lex.getNextToken().value);
            }
            if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
            memory[targetName] = memory[leftToken.value].conv2d_grad_kernel(memory[inTok.value], memory[gradTok.value], stride);
            return;
        }
        if (mTok.value == "conv2d_grad_input") {
            if (lex.getNextToken().type != GTokenType::LParen) throw runtime_error("Esperado '(' apos conv2d_grad_input.");
            Token gradTok = lex.getNextToken();
            lex.getNextToken(); // ','
            Token kTok = lex.getNextToken();
            int stride = 1;
            if (lex.peekNextToken().type == GTokenType::Comma) {
                lex.getNextToken();
                stride = std::stoi(lex.getNextToken().value);
            }
            if (lex.getNextToken().type != GTokenType::RParen) throw runtime_error("Esperado ')'.");
            memory[targetName] = memory[leftToken.value].conv2d_grad_input(memory[gradTok.value], memory[kTok.value], stride);
            return;
        }
    }
    
    Token opToken = lex.getNextToken();
    if (opToken.type != GTokenType::Plus && opToken.type != GTokenType::Minus && opToken.type != GTokenType::Star && opToken.type != GTokenType::Slash) {
        throw runtime_error("Operador matematico esperado (+, -, *, /).");
    }
    
    Token rightToken = lex.getNextToken();
    
    if (memory.find(leftToken.value) == memory.end()) {
        throw runtime_error("Variavel '" + leftToken.value + "' nao encontrada.");
    }
    
    GhostMatrix leftMat = memory[leftToken.value];
    
    if (rightToken.type == GTokenType::Identifier) {
        if (memory.find(rightToken.value) == memory.end()) {
            throw runtime_error("Variavel '" + rightToken.value + "' nao encontrada.");
        }
        GhostMatrix rightMat = memory[rightToken.value];
        
        if (opToken.type == GTokenType::Plus) {
            memory[targetName] = leftMat + rightMat;
        } else if (opToken.type == GTokenType::Minus) {
            memory[targetName] = leftMat - rightMat;
        } else if (opToken.type == GTokenType::Star) {
            memory[targetName] = leftMat * rightMat;
        } else if (opToken.type == GTokenType::Slash) {
            throw runtime_error("Divisao matricial por outra matriz nao suportada (apenas por escalar).");
        }
    } else if (rightToken.type == GTokenType::Number) {
        if (opToken.type != GTokenType::Star && opToken.type != GTokenType::Slash) {
            throw runtime_error("Apenas multiplicacao e divisao por escalar sao suportadas no momento.");
        }
        double scalar = stod(rightToken.value);
        if (opToken.type == GTokenType::Star) {
            memory[targetName] = leftMat * scalar;
        } else if (opToken.type == GTokenType::Slash) {
            memory[targetName] = leftMat / scalar;
        }
    } else {
        throw runtime_error("Operando direito invalido.");
    }
}

// Avaliação numérica: usa o avaliador do núcleo (precedência, inteiros, %, bitwise...).
double GhostInterpreter::evaluateMath(std::vector<Token>& tokens) {
    if (tokens.empty()) return 0.0;
    return toNum(evalValue(tokens), "expressao");
}

bool GhostInterpreter::evaluateCondition(std::vector<Token>& tokens) {
    if (tokens.empty()) return false;
    return truthy(evalValue(tokens));
}

void GhostInterpreter::parseVar(Lexer& lex, GTokenType type) {
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Nome de variavel esperado.");
    declareLocal(nameToken.value); // dentro de funcao, a variavel e local
    
    Token assignToken = lex.getNextToken();
    bool isArray = false;
    int arraySize = 0;
    
    if (assignToken.type == GTokenType::LBracket) {
        isArray = true;
        Token sizeToken = lex.getNextToken();
        if (sizeToken.type == GTokenType::Number) {
            arraySize = std::stoi(sizeToken.value);
            Token rBracket = lex.getNextToken();
            if (rBracket.type != GTokenType::RBracket) throw std::runtime_error("Esperado ']' apos tamanho do array.");
        } else if (sizeToken.type == GTokenType::RBracket) {
            arraySize = -1;
        } else {
            throw std::runtime_error("Tamanho do array invalido.");
        }
        assignToken = lex.getNextToken();
    }
    
    if (assignToken.type != GTokenType::Assign && assignToken.type != GTokenType::Semicolon) {
        throw std::runtime_error("Esperado '=' ou ';'.");
    }
    
    if (isArray) {
        if (assignToken.type == GTokenType::Semicolon) {
            if (arraySize == -1) throw std::runtime_error("Array sem tamanho deve ser inicializado.");
            memory[nameToken.value] = GhostMatrix(1, arraySize);
            type_memory[nameToken.value] = type;
            return;
        } else if (assignToken.type == GTokenType::Assign) {
            Token lBrace = lex.getNextToken();
            if (lBrace.type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' para inicializar array.");
            
            std::vector<double> values;
            while (true) {
                Token t = lex.getNextToken();
                if (t.type == GTokenType::RBrace) break;
                if (t.type == GTokenType::Comma) continue;
                
                std::vector<Token> exprTokens;
                exprTokens.push_back(t);
                while (true) {
                    Token peek = lex.peekNextToken();
                    if (peek.type == GTokenType::Comma || peek.type == GTokenType::RBrace) break;
                    exprTokens.push_back(lex.getNextToken());
                }
                values.push_back(evaluateMath(exprTokens));
            }
            
            if (arraySize != -1 && arraySize != (int)values.size()) {
                throw std::runtime_error("Tamanho da inicializacao nao bate com o tamanho do array.");
            }
            arraySize = values.size();
            GhostMatrix arr(1, arraySize);
            for (int i = 0; i < arraySize; i++) {
                if (type == GTokenType::KwInt) arr(0, i) = GhostNumberG((int)values[i], 0.0, 0);
                else arr(0, i) = GhostNumberG(values[i], 0.0, 0);
            }
            memory[nameToken.value] = arr;
            type_memory[nameToken.value] = type;
            
            Token semi = lex.getNextToken();
            if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos array.");
            return;
        }
    }
    
    type_memory[nameToken.value] = type;
    
    if (assignToken.type == GTokenType::Semicolon) {
        if (type == GTokenType::KwString) string_memory[nameToken.value] = "";
        else if (type == GTokenType::KwLlm) llm_memory[nameToken.value] = GhostLLM();
        else numeric_memory[nameToken.value] = 0.0;
        return;
    }
    
    if (type == GTokenType::KwLlm) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::KwLoadGguf) {
            lex.getNextToken(); // '('
            Token pathToken = lex.getNextToken();
            lex.getNextToken(); // ')'
            llm_memory[nameToken.value] = GhostLLM(pathToken.value);
        } else {
            throw std::runtime_error("Esperado 'load_gguf' para tipo llm.");
        }
        Token next = lex.getNextToken();
        if (next.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
        return;
    }
    
    if (type == GTokenType::KwString || type == GTokenType::KwDate || type == GTokenType::KwTime || type == GTokenType::KwDatetime || type == GTokenType::KwJson) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::KwInput) {
            Token lParen = lex.getNextToken();
            Token strToken = lex.getNextToken();
            Token rParen = lex.getNextToken();
            std::cout << strToken.value;
            std::string buffer;
            std::getline(std::cin, buffer);
            string_memory[nameToken.value] = buffer;
        } else if (t.type == GTokenType::String) {
            string_memory[nameToken.value] = Lexer::unescape(t.value);
        } else if (t.type == GTokenType::Identifier && t.value == "now") {
            lex.getNextToken(); // consume '('
            lex.getNextToken(); // consume ')'
            std::time_t rawtime;
            std::time(&rawtime);
            std::tm* timeinfo = std::localtime(&rawtime);
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            string_memory[nameToken.value] = std::string(buffer);
        } else if (t.type == GTokenType::Identifier && (isQuantum(t.value) || t.value == "quantum_backend")) {
            // string s = q.draw();  string k = q;  string b = quantum_backend();
            vector<Token> expr{t};
            while (lex.peekNextToken().type != GTokenType::Semicolon && lex.peekNextToken().type != GTokenType::EndOfFile) expr.push_back(lex.getNextToken());
            string_memory[nameToken.value] = evaluateStringExpression(expr);
        } else if (t.type == GTokenType::Identifier && llm_memory.count(t.value)) {
            Token dotToken = lex.getNextToken(); // '.'
            Token funcToken = lex.getNextToken(); // 'prompt'
            if (dotToken.type == GTokenType::Dot && funcToken.type == GTokenType::Identifier && funcToken.value == "prompt") {
                lex.getNextToken(); // '('
                Token strToken = lex.getNextToken();
                lex.getNextToken(); // ')'
                // The prompt string might be an expression, but right now we only handle a simple string token.
                // To be robust, let's assume it's just a String token for now, as in test_llama.g
                std::string inputPrompt = strToken.value;
                if (strToken.type == GTokenType::Identifier && string_memory.count(strToken.value)) {
                    inputPrompt = string_memory[strToken.value];
                }
                string_memory[nameToken.value] = llm_memory[t.value].prompt(inputPrompt);
            } else {
                throw std::runtime_error("Metodo invalido para objeto llm.");
            }
        } else if (t.type == GTokenType::Identifier && t.value == "parse_date") {
            lex.getNextToken(); // '('
            Token strTok = lex.getNextToken();
            lex.getNextToken(); // ','
            Token fmtTok = lex.getNextToken();
            lex.getNextToken(); // ')'
            std::tm tm = {};
            std::istringstream ss(strTok.value);
            ss >> std::get_time(&tm, fmtTok.value.c_str());
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
            string_memory[nameToken.value] = std::string(buffer);
        } else if (t.type == GTokenType::Identifier && t.value == "format_date") {
            lex.getNextToken(); // '('
            Token varTok = lex.getNextToken();
            lex.getNextToken(); // ','
            Token fmtTok = lex.getNextToken();
            lex.getNextToken(); // ')'
            std::tm tm = {};
            std::string d_str = string_memory.count(varTok.value) ? string_memory[varTok.value] : varTok.value;
            std::istringstream ss(d_str);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), fmtTok.value.c_str(), &tm);
            string_memory[nameToken.value] = std::string(buffer);
        } else if (t.type == GTokenType::Identifier && t.value == "format_number") {
            lex.getNextToken(); // '('
            Token varTok = lex.getNextToken();
            lex.getNextToken(); // ','
            Token decTok = lex.getNextToken();
            lex.getNextToken(); // ')'
            double val = numeric_memory.count(varTok.value) ? numeric_memory[varTok.value] : std::stod(varTok.value);
            int dec = std::stoi(decTok.value);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(dec) << val;
            string_memory[nameToken.value] = ss.str();
        } else if (t.type == GTokenType::LBrace) {
            std::string jsonStr = "{";
            int bDepth = 1;
            while (true) {
                Token jt = lex.getNextToken();
                if (jt.type == GTokenType::EndOfFile) throw std::runtime_error("JSON nao fechado.");
                if (jt.type == GTokenType::LBrace) {
                    bDepth++;
                    jsonStr += "{";
                } else if (jt.type == GTokenType::RBrace) {
                    bDepth--;
                    jsonStr += "}";
                    if (bDepth == 0) break;
                } else if (jt.type == GTokenType::String) {
                    jsonStr += "\"" + jt.value + "\"";
                } else if (jt.type == GTokenType::Identifier && jt.value == "true") {
                    jsonStr += "true";
                } else if (jt.type == GTokenType::Identifier && jt.value == "false") {
                    jsonStr += "false";
                } else if (jt.type == GTokenType::Identifier) {
                    if (string_memory.count(jt.value)) {
                        jsonStr += "\"" + string_memory[jt.value] + "\"";
                    } else if (numeric_memory.count(jt.value)) {
                        std::ostringstream ss; ss << numeric_memory[jt.value];
                        jsonStr += ss.str();
                    } else {
                        jsonStr += "\"" + jt.value + "\"";
                    }
                } else {
                    jsonStr += jt.value;
                }
            }
            string_memory[nameToken.value] = jsonStr;
        } else {
            throw std::runtime_error("Esperado valor compativel com o tipo.");
        }
        
        Token next = lex.getNextToken();
        if (next.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
    } else {
        Token peek = lex.peekNextToken();
        if (peek.type == GTokenType::String) {
            lex.getNextToken();
            string_memory[nameToken.value] = Lexer::unescape(peek.value);
            Token next = lex.getNextToken();
            if (next.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
            return;
        } else if (peek.type == GTokenType::LBrace) {
            lex.getNextToken(); // '{'
            std::string jsonStr = "{";
            int bDepth = 1;
            while (true) {
                Token t = lex.getNextToken();
                if (t.type == GTokenType::EndOfFile) throw std::runtime_error("JSON nao fechado.");
                if (t.type == GTokenType::LBrace) {
                    bDepth++;
                    jsonStr += "{";
                } else if (t.type == GTokenType::RBrace) {
                    bDepth--;
                    jsonStr += "}";
                    if (bDepth == 0) break;
                } else if (t.type == GTokenType::String) {
                    jsonStr += "\"" + t.value + "\"";
                } else if (t.type == GTokenType::Identifier && t.value == "true") {
                    jsonStr += "true";
                } else if (t.type == GTokenType::Identifier && t.value == "false") {
                    jsonStr += "false";
                } else if (t.type == GTokenType::Identifier) {
                    if (string_memory.count(t.value)) {
                        jsonStr += "\"" + string_memory[t.value] + "\"";
                    } else if (numeric_memory.count(t.value)) {
                        std::ostringstream ss; ss << numeric_memory[t.value];
                        jsonStr += ss.str();
                    } else {
                        jsonStr += "\"" + t.value + "\"";
                    }
                } else {
                    jsonStr += t.value;
                }
            }
            string_memory[nameToken.value] = jsonStr;
            Token next = lex.getNextToken();
            if (next.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
            return;
        } else if (peek.type == GTokenType::Identifier && peek.value == "parse_number") {
            lex.getNextToken(); // parse_number
            lex.getNextToken(); // (
            Token strTok = lex.getNextToken();
            lex.getNextToken(); // )
            numeric_memory[nameToken.value] = std::stod(strTok.value);
            Token next = lex.getNextToken();
            if (next.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
            return;
        } else if (type == GTokenType::KwGhost) {
            std::vector<Token> exprTokens;
            while (true) {
                Token t = lex.getNextToken();
                if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
                exprTokens.push_back(t);
            }
            // ghost g = ghost(x, y, p, theta2); (ou outra expressao que de um Numero G)
            if (exprTokens.size() >= 2 && exprTokens[0].type == GTokenType::KwGhost && exprTokens[1].type == GTokenType::LParen) {
                GhostValue gv = evalValue(exprTokens);
                memory[nameToken.value] = GhostMatrix(1, 1);
                memory[nameToken.value](0, 0) = parse_ghost_number(gv.str);
                type_memory[nameToken.value] = GTokenType::KwGhost;
                return;
            }
            
            double real_part = 0, imag_part = 0, ghost_part = 0;
            std::vector<std::vector<Token>> terms;
            std::vector<Token> currentTerm;
            int depth = 0;
            for (size_t i = 0; i < exprTokens.size(); i++) {
                Token t = exprTokens[i];
                if (t.type == GTokenType::LParen) depth++;
                else if (t.type == GTokenType::RParen) depth--;
                
                if (depth == 0 && (t.type == GTokenType::Plus || t.type == GTokenType::Minus)) {
                    if (!currentTerm.empty()) terms.push_back(currentTerm);
                    currentTerm.clear();
                    currentTerm.push_back(t);
                } else {
                    currentTerm.push_back(t);
                }
            }
            if (!currentTerm.empty()) terms.push_back(currentTerm);
            
            for (auto& term : terms) {
                std::string comp = "real";
                int comp_idx = -1;
                int d = 0;
                for (int j = term.size() - 1; j >= 0; j--) {
                    if (term[j].type == GTokenType::RParen) d++;
                    else if (term[j].type == GTokenType::LParen) d--;
                    else if (d == 0 && term[j].type == GTokenType::Identifier) {
                        if (term[j].value == "i") { comp = "imag"; comp_idx = j; break; }
                        else if (term[j].value == "g" || term[j].value == "theta") { comp = "ghost"; comp_idx = j; break; }
                    } else if (d == 0 && term[j].type == GTokenType::Number) {
                        const std::string& val = term[j].value;
                        if (!val.empty() && val.back() == 'i') {
                            comp = "imag";
                            comp_idx = j;
                            break;
                        } else if (!val.empty() && (val.back() == 'g' || (val.size() >= 5 && val.substr(val.size() - 5) == "theta"))) {
                            comp = "ghost";
                            comp_idx = j;
                            break;
                        }
                    }
                }
                
                double coeff = 1.0;
                std::vector<Token> mathToks;
                
                if (comp_idx != -1) {
                    if (term[comp_idx].type == GTokenType::Number) {
                        // O número já tem sufixo 'i', 'g' ou 'theta' (ex: "2.0i", "3.0g", "5.0theta")
                        for (int j = 0; j < (int)term.size(); j++) {
                            if (j == comp_idx) {
                                std::string stripped = term[j].value;
                                if (stripped.back() == 'i' || stripped.back() == 'g') stripped.pop_back();
                                else if (stripped.size() >= 5 && stripped.substr(stripped.size() - 5) == "theta") stripped = stripped.substr(0, stripped.size() - 5);
                                if (stripped.empty() || stripped == "+" || stripped == "-") stripped += "1.0";
                                mathToks.push_back(Token{GTokenType::Number, stripped});
                            } else {
                                mathToks.push_back(term[j]);
                            }
                        }
                    } else {
                        for (int j = 0; j < (int)term.size(); j++) {
                            if (j == comp_idx) continue;
                            if (j == comp_idx - 1 && term[j].type == GTokenType::Star) continue;
                            mathToks.push_back(term[j]);
                        }
                    }
                } else {
                    mathToks = term;
                }
                
                if (mathToks.empty() || (mathToks.size() == 1 && (mathToks[0].type == GTokenType::Plus || mathToks[0].type == GTokenType::Minus))) {
                    coeff = (mathToks.empty() || mathToks[0].type == GTokenType::Plus) ? 1.0 : -1.0;
                } else {
                    coeff = evaluateMath(mathToks);
                }
                
                if (comp == "real") real_part += coeff;
                else if (comp == "imag") imag_part += coeff;
                else if (comp == "ghost") ghost_part += coeff;
            }
            
            memory[nameToken.value] = GhostMatrix(1, 1);
            memory[nameToken.value](0, 0) = GhostNumberG(real_part, imag_part, ghost_part);
            type_memory[nameToken.value] = type;
            return;
        }

        std::vector<Token> exprTokens;
        bool hasInput = false;
        std::string promptStr = "";
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile) break;
            if (t.type == GTokenType::KwInput) {
                hasInput = true;
                Token lParen = lex.getNextToken();
                Token strToken = lex.getNextToken();
                Token rParen = lex.getNextToken();
                if (strToken.type == GTokenType::String) promptStr = strToken.value;
            } else {
                exprTokens.push_back(t);
            }
        }
        if (hasInput) {
            std::cout << promptStr;
            std::string buffer;
            std::getline(std::cin, buffer);
            double val = 0.0;
            try { val = std::stod(buffer); } catch (...) {}
            if (type == GTokenType::KwInt) numeric_memory[nameToken.value] = (int)val;
            else numeric_memory[nameToken.value] = val;
        } else {
            double val = evaluateMath(exprTokens);
            if (type == GTokenType::KwInt) {
                numeric_memory[nameToken.value] = (int)val;
            } else {
                numeric_memory[nameToken.value] = val;
            }
        }
    }
}
std::string GhostInterpreter::evaluateFString(const std::string& fstr) {
    std::string result = "";
    for (size_t i = 0; i < fstr.length(); i++) {
        if (fstr[i] == '{' && i + 1 < fstr.length() && fstr[i+1] == '{') {
            result += '{';
            i++;
            continue;
        }
        if (fstr[i] == '}' && i + 1 < fstr.length() && fstr[i+1] == '}') {
            result += '}';
            i++;
            continue;
        }
        const bool hasDollar = (fstr[i] == '$' && i + 1 < fstr.length() && fstr[i+1] == '{');
        if (hasDollar || fstr[i] == '{') {
            i += (hasDollar ? 2 : 1);
            std::string expr = "";
            int braces = 1;
            while (i < fstr.length()) {
                if (fstr[i] == '{') braces++;
                else if (fstr[i] == '}') {
                    braces--;
                    if (braces == 0) break;
                }
                expr += fstr[i];
                i++;
            }
            std::string inner, spec; // {x:.2f} ou ${x:.2f}: formatação (include/GhostFormat.hpp, igual ao compilado)
            ghost_split_fmt(expr, inner, spec);
            Lexer tempLexer(inner);
            std::vector<Token> tokens;
            while (true) {
                Token t = tempLexer.getNextToken();
                if (t.type == GTokenType::EndOfFile || t.type == GTokenType::Semicolon) break;
                tokens.push_back(t);
            }
            if (!spec.empty()) {
                const GhostValue v = evalValue(tokens);
                if (v.kind == GhostValue::Num) result += ghost_fmt_num(v.num, v.isInt, spec);
                else if (v.kind == GhostValue::Bool) {
                    GhostFmtSpec f;
                    ghost_parse_fmt(spec, f);
                    result += (f.type && f.type != 's') ? ghost_fmt_num(v.num, true, spec) : ghost_fmt_text(v.num != 0.0 ? "true" : "false", spec);
                } else result += ghost_fmt_text(toText(v), spec);
                continue;
            }
            // Mesma avaliacao do print: numeros, variaveis string e objetos quanticos (${q}, ${q.prob(0)})
            result += evaluateStringExpression(tokens);
        } else {
            result += fstr[i];
        }
    }
    return result;
}

// Texto de uma expressão (print, f-strings, string s = ...): cada argumento separado por vírgula
// é avaliado inteiro e os textos são concatenados. "a" + 1 concatena; 2 + 3 soma.
std::string GhostInterpreter::evaluateStringExpression(std::vector<Token>& tokens) {
    std::string result;
    std::vector<Token> piece;
    int depth = 0;
    auto flush = [&]() {
        if (!piece.empty()) result += toText(evalValue(piece));
        piece.clear();
    };
    int angle = 0; // < > de tipos genéricos: print(Par<string, int>("a", 1).descrever())
    for (auto& t : tokens) {
        if (t.type == GTokenType::Lt && (angle > 0 || (!piece.empty() && opensTypeAngle(piece.back())))) {
            angle++;
            piece.push_back(t);
            continue;
        }
        if (angle > 0 && (t.type == GTokenType::Gt || t.type == GTokenType::Chain)) {
            angle = std::max(0, angle - (t.type == GTokenType::Chain ? 2 : 1));
            piece.push_back(t);
            continue;
        }
        if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
        if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) depth--;
        if (depth == 0 && angle == 0 && t.type == GTokenType::Comma) { flush(); continue; }
        piece.push_back(t);
    }
    flush();
    return result;
}

void GhostInterpreter::parsePrint(Lexer& lex) {
    Token openParen = lex.getNextToken();
    if (openParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos print.");
    
    std::vector<Token> exprTokens;
    int pDepth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Faltando ')'.");
        
        if (t.type == GTokenType::LParen) pDepth++;
        if (t.type == GTokenType::RParen) {
            pDepth--;
            if (pDepth == 0) break;
        }
        
        exprTokens.push_back(t);
    }
    
    std::cout << evaluateStringExpression(exprTokens) << std::endl;
    
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';'.");
}

namespace {
// Lê "( ... )" (o '(' ainda não consumido) e devolve os tokens de dentro.
std::vector<Token> readParenGroup(Lexer& lex, const char* what) {
    if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error(std::string("Esperado '(' apos ") + what + ".");
    std::vector<Token> out;
    int depth = 1;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::EndOfFile) throw std::runtime_error(std::string("Faltou ')' em ") + what + ".");
        if (t.type == GTokenType::LParen) depth++;
        if (t.type == GTokenType::RParen && --depth == 0) break;
        out.push_back(t);
    }
    return out;
}
std::string readBlock(Lexer& lex, const char* what) {
    if (lex.getNextToken().type != GTokenType::LBrace) throw std::runtime_error(std::string("Esperado '{' apos ") + what + ".");
    return lex.extractBlock();
}
} // namespace

// if (c) { } else if (c2) { } else { }
void GhostInterpreter::parseIf(Lexer& lex) {
    std::vector<std::pair<std::vector<Token>, std::string>> branches;
    std::string elseBlock;
    bool hasElse = false;
    auto cond = readParenGroup(lex, "if");
    branches.push_back({cond, readBlock(lex, "condicao if")});
    while (lex.peekNextToken().type == GTokenType::KwElse) {
        lex.getNextToken();
        if (lex.peekNextToken().type == GTokenType::KwIf) {
            lex.getNextToken();
            auto c = readParenGroup(lex, "else if");
            branches.push_back({c, readBlock(lex, "condicao else if")});
            continue;
        }
        elseBlock = readBlock(lex, "else");
        hasElse = true;
        break;
    }
    for (auto& b : branches) {
        if (evaluateCondition(b.first)) { executeLine(b.second); return; }
    }
    if (hasElse) executeLine(elseBlock);
}

void GhostInterpreter::parseWhile(Lexer& lex) {
    auto cond = readParenGroup(lex, "while");
    const std::string blockCode = readBlock(lex, "condicao while");
    while (evaluateCondition(cond)) {
        try { executeLine(blockCode); }
        catch (GhostBreakSignal&) { break; }
        catch (GhostContinueSignal&) { continue; }
    }
}

// for (init; cond; passo) { }   e   for (x in xs) { }  /  for (k, v in mapa) { }
void GhostInterpreter::parseFor(Lexer& lex) {
    std::vector<Token> all;
    if (lex.peekNextToken().type != GTokenType::LParen) {
        // Forma do Python, sem parênteses: for x in range(10) { ... } / for k, v in mapa { ... }
        int depth = 0;
        while (true) {
            const Token t = lex.peekNextToken();
            if (t.type == GTokenType::EndOfFile) throw std::runtime_error("for x in colecao: faltou '{'");
            if (depth == 0 && t.type == GTokenType::LBrace) break;
            lex.getNextToken();
            if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket) depth++;
            if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket) depth--;
            all.push_back(t);
        }
    } else {
        all = readParenGroup(lex, "for");
    }
    const std::string blockCode = readBlock(lex, "condicao for");
    // for ([tipo] x in colecao): tem 'in' fora de parênteses e nenhum ';'
    bool temIn = false, temPontoVirgula = false;
    {
        int depth = 0;
        for (const auto& t : all) {
            if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket) depth++;
            if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket) depth--;
            if (depth == 0 && t.type == GTokenType::KwIn) temIn = true;
            if (depth == 0 && t.type == GTokenType::Semicolon) temPontoVirgula = true;
        }
    }
    if (temIn && !temPontoVirgula) {
        foreachFromHead(all, blockCode);
        return;
    }
    std::vector<Token> parts[3];
    int stage = 0, depth = 0;
    for (auto& t : all) {
        if (t.type == GTokenType::LParen) depth++;
        if (t.type == GTokenType::RParen) depth--;
        if (depth == 0 && t.type == GTokenType::Semicolon && stage < 2) { stage++; continue; }
        parts[stage].push_back(t);
    }
    const std::string initCode = tokensToSource(parts[0]) + ";";
    const std::string incCode = tokensToSource(parts[2]) + ";";
    // for (int i = ...): i existe só dentro do laço (como em C/C++)
    std::string loopVar;
    if (parts[0].size() >= 2 && parts[0][0].type != GTokenType::Identifier && parts[0][1].type == GTokenType::Identifier) loopVar = parts[0][1].value;
    const SavedVar before = loopVar.empty() ? SavedVar{} : saveVar(loopVar);
    try {
        executeLine(initCode);
        while (parts[1].empty() || evaluateCondition(parts[1])) {
            try { executeLine(blockCode); }
            catch (GhostBreakSignal&) { break; }
            catch (GhostContinueSignal&) {}
            executeLine(incCode);
        }
    } catch (...) {
        if (!loopVar.empty()) restoreVar(loopVar, before);
        throw;
    }
    if (!loopVar.empty()) restoreVar(loopVar, before);
}

// function [tipo] nome(tipo a, tipo b = padrao) { ... }   (sem tipo: void)
// import "arquivo.g" [as m];  Procura na pasta de quem importa, na pasta atual e na raiz da GhostLang
// (a pasta com include/GhostNumberG.hpp, ao lado do ghost ou acima dele), como o compilador.
void GhostInterpreter::importModule(Lexer& lex) {
    namespace fs = std::filesystem;
    const Token file = lex.getNextToken();
    if (file.type != GTokenType::String) throw std::runtime_error("use import \"arquivo.g\"; ou import \"arquivo.g\" as nome;");
    std::string alias;
    Token t = lex.getNextToken();
    if (t.type == GTokenType::Identifier && t.value == "as") {
        const Token a = lex.getNextToken();
        if (a.type != GTokenType::Identifier) throw std::runtime_error("import ... as: esperado um nome depois de as");
        alias = a.value;
        t = lex.getNextToken();
    }
    if (t.type != GTokenType::Semicolon && t.type != GTokenType::EndOfFile) throw std::runtime_error("esperado ';' depois do import");
    std::error_code ec;
    std::vector<fs::path> dirs;
    if (!import_stack.empty()) dirs.push_back(fs::path(import_stack.back()).parent_path());
    dirs.push_back(fs::current_path(ec));
#ifdef _WIN32
    char exeBuf[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    const fs::path exeDir = fs::path(exeBuf).parent_path();
#else
    char exeBuf[4096] = {0};
    const ssize_t n = readlink("/proc/self/exe", exeBuf, sizeof(exeBuf) - 1);
    const fs::path exeDir = n > 0 ? fs::path(std::string(exeBuf, static_cast<size_t>(n))).parent_path() : fs::current_path(ec);
#endif
    for (const fs::path& c : {exeDir, exeDir.parent_path()})
        if (fs::exists(c / "include" / "GhostNumberG.hpp", ec)) dirs.push_back(c);
    fs::path found;
    for (const auto& d : dirs) {
        const fs::path cand = d / file.value;
        if (fs::exists(cand, ec)) { found = cand; break; }
    }
    if (found.empty()) throw std::runtime_error("nao foi possivel importar o arquivo " + file.value);
    const std::string canon = fs::weakly_canonical(found, ec).string();
    if (std::find(import_stack.begin(), import_stack.end(), canon) != import_stack.end()) {
        std::string chain;
        bool on = false;
        for (const auto& f : import_stack) {
            if (f == canon) on = true;
            if (on) chain += fs::path(f).filename().string() + " -> ";
        }
        std::cerr << "[Ghost] aviso: import circular: " << chain << fs::path(canon).filename().string()
                  << " (o import repetido foi ignorado)" << std::endl;
        return;
    }
    if (!imported_modules.insert(canon + "|" + alias).second) return; // já importado (com este apelido)
    std::ifstream in(found, std::ios::binary);
    std::stringstream buf;
    buf << in.rdbuf();
    std::string text = buf.str();
    if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) text.erase(0, 3);
    {
        std::string t2;
        for (char c : text) if (c != '\r') t2 += c;
        text.swap(t2);
    }
    if (!alias.empty()) {
        const std::set<std::string> names = ghost_module_exports(text);
        ghost_module_aliases()[alias].insert(names.begin(), names.end());
        text = ghost_mangle_module(text, alias, names);
    }
    import_stack.push_back(canon);
    try {
        executeLine(text);
    } catch (...) {
        import_stack.pop_back();
        throw;
    }
    import_stack.pop_back();
}

void GhostInterpreter::parseFunction(Lexer& lex) {
    if (tryGenericFunctionDef(lex)) return; // function T maior<T>(...)
    std::string name;
    GhostFunction func = readFunctionDef(lex, name);
    if (class_memory.count(name)) throw std::runtime_error("function " + name + ": ja existe uma classe com esse nome");
    functions_memory[name] = func;
    auto& ovs = function_overloads[name];
    bool replaced = false;
    for (auto& f : ovs) {
        if (f.paramSpecs == func.paramSpecs) {
            f = func;
            replaced = true;
            break;
        }
    }
    if (!replaced) ovs.push_back(func);
}

// Lê assinatura e corpo (o 'function' já foi consumido). Também usada pelos métodos de classe.
GhostFunction GhostInterpreter::readFunctionDef(Lexer& lex, std::string& outName, bool signatureOnly, bool allowInterfaceDefault) {
    auto isType = [](GTokenType t) {
        return t == GTokenType::KwInt || t == GTokenType::KwDouble || t == GTokenType::KwFloat || t == GTokenType::KwString ||
               t == GTokenType::KwBool || t == GTokenType::KwVoid || t == GTokenType::KwVar || t == GTokenType::KwList ||
               t == GTokenType::KwMap || t == GTokenType::KwGhost || t == GTokenType::KwComplex || t == GTokenType::KwQreg ||
               t == GTokenType::KwQcircuit;
    };
    auto readGenericSuffix = [&]() -> std::string {
        if (lex.peekNextToken().type != GTokenType::Lt) return "";
        std::string s;
        int depth = 0;
        while (true) {
            Token g = lex.getNextToken();
            if (g.type == GTokenType::EndOfFile) throw std::runtime_error("tipo generico sem '>'");
            if (g.type == GTokenType::Lt) depth++;
            else if (g.type == GTokenType::Gt) depth--;
            else if (g.type == GTokenType::Chain) depth -= 2;
            if (g.type == GTokenType::Comma) s += ",";
            else {
                if (!s.empty() && !g.value.empty() &&
                    (std::isalnum(static_cast<unsigned char>(s.back())) || s.back() == '_') &&
                    (std::isalnum(static_cast<unsigned char>(g.value.front())) || g.value.front() == '_')) s += " ";
                s += g.value;
            }
            if (depth <= 0) break;
        }
        return s;
    };
    auto isFnTypeWord = [&](const std::string& w) {
        return w == "fn" && !class_memory.count("fn") && !interface_memory.count("fn");
    };
    Token first = lex.getNextToken();
    GTokenType retType = GTokenType::KwVoid;
    Token nameToken = first;
    std::string retClass;
    std::string retSpec = "void";
    if (isType(first.type)) {
        retType = first.type;
        const std::string suf = readGenericSuffix();
        retSpec = first.value + suf;
        if (!suf.empty()) {
            try { retSpec = validateAndNormalizeTypeSpec(retSpec, true); ensureTypeReady(retSpec); } catch (...) {}
        }
        while (lex.peekNextToken().type == GTokenType::LBracket) {
            const size_t save = lex.getPosition();
            lex.getNextToken();
            if (lex.getNextToken().type == GTokenType::RBracket &&
                (lex.peekNextToken().type == GTokenType::Identifier || lex.peekNextToken().type == GTokenType::LBracket)) {
                retType = GTokenType::KwList;
                retSpec = "list<" + retSpec + ">";
            } else {
                lex.setPosition(save);
                break;
            }
        }
        if (isListSpec(retSpec)) retClass = retSpec;
        nameToken = lex.getNextToken();
    } else if (first.type == GTokenType::Identifier &&
               (first.value == "result" || first.value == "set" || isFnTypeWord(first.value) ||
                generic_memory.count(first.value) || generic_iface_memory.count(first.value)) &&
               lex.peekNextToken().type == GTokenType::Lt) {
        retType = GTokenType::KwVar;
        retClass = readResultSpec(lex, false, first.value);
        ensureTypeReady(retClass);
        retSpec = retClass;
        while (lex.peekNextToken().type == GTokenType::LBracket) {
            const size_t save = lex.getPosition();
            lex.getNextToken();
            if (lex.getNextToken().type == GTokenType::RBracket &&
                (lex.peekNextToken().type == GTokenType::Identifier || lex.peekNextToken().type == GTokenType::LBracket)) {
                retType = GTokenType::KwList;
                retSpec = "list<" + retSpec + ">";
                retClass = retSpec;
            } else {
                lex.setPosition(save);
                break;
            }
        }
        nameToken = lex.getNextToken();
    } else if (first.type == GTokenType::Identifier && isFnTypeWord(first.value) && lex.peekNextToken().type == GTokenType::Identifier) {
        throw std::runtime_error("use fn<retorno(parametros)>, ex.: function fn<int(int)> " + lex.peekNextToken().value + "(...)");
    } else if (first.type == GTokenType::Identifier && lex.peekNextToken().type == GTokenType::Identifier) {
        if (generic_memory.count(first.value))
            throw std::runtime_error(first.value + " e uma classe generica: use " + first.value + "<tipo>");
        if (generic_iface_memory.count(first.value))
            throw std::runtime_error(first.value + " e uma interface generica: use " + first.value + "<tipo>");
        retType = GTokenType::KwVar; // tipo de classe ou interface
        retClass = first.value;
        retSpec = first.value;
        nameToken = lex.getNextToken();
    } else if (first.type == GTokenType::Identifier && lex.peekNextToken().type == GTokenType::LBracket) {
        const size_t save = lex.getPosition();
        lex.getNextToken();
        if (lex.getNextToken().type == GTokenType::RBracket &&
            (lex.peekNextToken().type == GTokenType::Identifier || lex.peekNextToken().type == GTokenType::LBracket)) {
            retType = GTokenType::KwList;
            retSpec = "list<" + first.value + ">";
            while (lex.peekNextToken().type == GTokenType::LBracket) {
                const size_t sv2 = lex.getPosition();
                lex.getNextToken();
                if (lex.getNextToken().type == GTokenType::RBracket) retSpec = "list<" + retSpec + ">";
                else { lex.setPosition(sv2); break; }
            }
            retClass = retSpec;
            nameToken = lex.getNextToken();
        } else {
            lex.setPosition(save);
        }
    }
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Nome de funcao esperado.");
    if (lex.peekNextToken().type == GTokenType::Lt) // só chega aqui dentro de classe (as genéricas de fora são lidas antes)
        throw std::runtime_error("metodos genericos ainda nao existem; use uma funcao generica fora da classe ou uma classe generica");
    if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos nome da funcao.");

    GhostFunction func;
    func.returnType = retType;
    func.returnClass = retClass;
    func.returnSpec = retSpec;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::RParen) break;
        if (t.type == GTokenType::Comma) continue;
        if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Faltou ')' nos parametros de " + nameToken.value);
        GTokenType pType = t.type;
        std::string pClass;
        std::string pSpec;
        if (isType(pType)) {
            const std::string suf = readGenericSuffix();
            pSpec = t.value + suf;
            if (!suf.empty()) {
                try { pSpec = validateAndNormalizeTypeSpec(pSpec, false); ensureTypeReady(pSpec); } catch (...) {}
            }
        } else if (t.type == GTokenType::Identifier) {
            pType = GTokenType::KwVar;
        }
        if (t.type == GTokenType::Identifier &&
            (t.value == "result" || t.value == "set" || isFnTypeWord(t.value) ||
             generic_memory.count(t.value) || generic_iface_memory.count(t.value)) &&
            lex.peekNextToken().type == GTokenType::Lt) {
            pClass = readResultSpec(lex, false, t.value);
            ensureTypeReady(pClass);
            pSpec = pClass;
        } else if (t.type == GTokenType::Identifier && isFnTypeWord(t.value) && lex.peekNextToken().type == GTokenType::Identifier) {
            throw std::runtime_error("use fn<retorno(parametros)>, ex.: fn<int(int)> " + lex.peekNextToken().value);
        } else if (t.type == GTokenType::Identifier && lex.peekNextToken().type == GTokenType::Identifier) {
            if (generic_memory.count(t.value))
                throw std::runtime_error(t.value + " e uma classe generica: use " + t.value + "<tipo>");
            if (generic_iface_memory.count(t.value))
                throw std::runtime_error(t.value + " e uma interface generica: use " + t.value + "<tipo>");
            pClass = t.value; // Animal a / Forma f
            pSpec = t.value;
        }
        bool arrType = false; // int[] xs  /  Ponto[] pts  /  Forma[] fs
        while (lex.peekNextToken().type == GTokenType::LBracket) {
            const size_t save = lex.getPosition();
            lex.getNextToken();
            const Token rb = lex.getNextToken();
            if (rb.type == GTokenType::RBracket &&
                (lex.peekNextToken().type == GTokenType::Identifier || lex.peekNextToken().type == GTokenType::LBracket)) {
                arrType = true;
                pType = GTokenType::KwList;
                pSpec = "list<" + (pSpec.empty() ? t.value : pSpec) + ">";
            } else {
                lex.setPosition(save);
                break;
            }
        }
        Token pName = (arrType || isType(t.type) || !pClass.empty() || lex.peekNextToken().type == GTokenType::Identifier) ? lex.getNextToken() : t;
        if (pName.type != GTokenType::Identifier) throw std::runtime_error("Esperado identificador de parametro.");
        std::vector<Token> def;
        if (lex.peekNextToken().type == GTokenType::LBracket) {
            lex.getNextToken();
            lex.getNextToken();
            pType = GTokenType::KwList;
            pSpec = "list<" + (pSpec.empty() ? std::string("var") : pSpec) + ">";
        } // nome[]
        if (lex.peekNextToken().type == GTokenType::Assign) {
            lex.getNextToken();
            int depth = 0;
            while (true) {
                Token d = lex.peekNextToken();
                if (d.type == GTokenType::EndOfFile) break;
                if (depth == 0 && (d.type == GTokenType::Comma || d.type == GTokenType::RParen)) break;
                if (d.type == GTokenType::LParen || d.type == GTokenType::LBracket) depth++;
                if (d.type == GTokenType::RParen || d.type == GTokenType::RBracket) depth--;
                def.push_back(lex.getNextToken());
            }
        }
        if (pType == GTokenType::KwList || arrType) {
            pClass = isListSpec(pSpec) ? pSpec : "";
        }
        if (pSpec.empty()) pSpec = "var";
        func.parameters.push_back({pType, pName.value});
        func.defaults.push_back(def);
        func.paramClasses.push_back(pClass);
        func.paramSpecs.push_back(pSpec);
    }
    if (signatureOnly) {
        const Token end = lex.getNextToken();
        if (end.type == GTokenType::LBrace) {
            if (!allowInterfaceDefault)
                throw std::runtime_error(nameToken.value + "(): metodo de interface so declara a assinatura; o corpo fica na classe (termine com ';')");
            func.body = lex.extractBlock();
            func.hasDefault = true;
            outName = nameToken.value;
            return func;
        }
        if (end.type != GTokenType::Semicolon && end.type != GTokenType::EndOfFile) throw std::runtime_error("Esperado ';' depois de " + nameToken.value + "(...)");
        outName = nameToken.value;
        return func;
    }
    if (lex.getNextToken().type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' apos declaracao de parametros.");
    func.body = lex.extractBlock();
    outName = nameToken.value;
    return func;
}

// return [expr];  encerra a função (o valor vai para quem chamou)
void GhostInterpreter::parseReturn(Lexer& lex) {
    std::vector<Token> expr;
    int depth = 0;
    while (true) {
        Token t = lex.getNextToken();
        if (t.type == GTokenType::EndOfFile || (depth == 0 && t.type == GTokenType::Semicolon)) break;
        if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket || t.type == GTokenType::LBrace) depth++;
        else if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket || t.type == GTokenType::RBrace) depth--;
        expr.push_back(t);
    }
    GhostReturnSignal r;
    if (!expr.empty()) { r.value = evalValue(expr); r.hasValue = true; }
    throw r;
}

void GhostInterpreter::parseDoWhile(Lexer& lex) {
    const std::string blockCode = readBlock(lex, "do");
    if (lex.getNextToken().type != GTokenType::KwWhile) throw std::runtime_error("Esperado 'while' apos bloco 'do'.");
    auto cond = readParenGroup(lex, "while");
    if (lex.peekNextToken().type == GTokenType::Semicolon) lex.getNextToken();
    do {
        try { executeLine(blockCode); }
        catch (GhostBreakSignal&) { break; }
        catch (GhostContinueSignal&) {}
    } while (evaluateCondition(cond));
}

// switch (x) { case 1, 2: ...; break; case "a": ...; default: ... }  (queda entre cases como em C)
void GhostInterpreter::parseSwitch(Lexer& lex) {
    auto subjectTokens = readParenGroup(lex, "switch");
    const GhostValue subject = evalValue(subjectTokens);
    const std::string block = readBlock(lex, "switch");

    struct Section { bool isDefault = false; std::vector<std::vector<Token>> values; std::vector<Token> body; };
    std::vector<Section> sections;
    Lexer sl(block);
    int depth = 0;
    for (Token t = sl.getNextToken(); t.type != GTokenType::EndOfFile; t = sl.getNextToken()) {
        if (depth == 0 && (t.type == GTokenType::KwCase || t.type == GTokenType::KwDefault)) {
            Section s;
            if (t.type == GTokenType::KwDefault) {
                s.isDefault = true;
                if (sl.getNextToken().type != GTokenType::Colon) throw std::runtime_error("Esperado ':' apos default.");
            } else {
                std::vector<Token> v;
                int d = 0;
                while (true) {
                    Token c = sl.getNextToken();
                    if (c.type == GTokenType::EndOfFile) throw std::runtime_error("case sem ':'");
                    if (c.type == GTokenType::LParen) d++;
                    if (c.type == GTokenType::RParen) d--;
                    if (d == 0 && c.type == GTokenType::Colon) break;
                    if (d == 0 && c.type == GTokenType::Comma) { s.values.push_back(v); v.clear(); continue; }
                    v.push_back(c);
                }
                s.values.push_back(v);
            }
            sections.push_back(s);
            continue;
        }
        if (sections.empty()) throw std::runtime_error("switch: comandos antes do primeiro case");
        if (t.type == GTokenType::LBrace) depth++;
        if (t.type == GTokenType::RBrace) depth--;
        sections.back().body.push_back(t);
    }
    size_t start = sections.size();
    for (size_t k = 0; k < sections.size() && start == sections.size(); ++k) {
        for (auto& v : sections[k].values) {
            GhostValue cv = evalValue(v);
            const bool eq = (cv.kind == GhostValue::Str || subject.kind == GhostValue::Str) ? (cv.kind == subject.kind && cv.str == subject.str) : cv.num == subject.num;
            if (eq) { start = k; break; }
        }
    }
    if (start == sections.size()) {
        for (size_t k = 0; k < sections.size(); ++k) if (sections[k].isDefault) { start = k; break; }
    }
    for (size_t k = start; k < sections.size(); ++k) {
        try { executeLine(tokensToSource(sections[k].body)); }
        catch (GhostBreakSignal&) { return; }
    }
}

// foreach (x in colecao) { }   foreach (int x in xs) { }   foreach (k, v in mapa) { }
void GhostInterpreter::parseForeach(Lexer& lex) {
    auto head = readParenGroup(lex, "foreach");
    const std::string body = readBlock(lex, "cabecalho do foreach");
    foreachFromHead(head, body);
}

// Cabeçalho "[tipo] x[, y] in colecao" já lido (foreach, for (... in ...) e for x in ... { })
void GhostInterpreter::foreachFromHead(std::vector<Token>& head, const std::string& body) {
    size_t k = 0;
    const bool hasExplicitType = k < head.size() &&
                                 (head[k].type != GTokenType::Identifier ||
                                  (k + 1 < head.size() && (head[k + 1].type == GTokenType::Identifier ||
                                                           head[k + 1].type == GTokenType::Lt ||
                                                           head[k + 1].type == GTokenType::LBracket)));
    std::string foreachElemSpec;
    if (hasExplicitType) {
        const size_t typeStart = k;
        k++; // tipo (var, int, double, Ponto, Forma, Colecao<int>, fn<int(int)>, Pilha<int>...)
        if (k < head.size() && head[k].type == GTokenType::Lt) {
            int d = 0;
            for (; k < head.size(); ++k) {
                if (head[k].type == GTokenType::Lt) d++;
                else if (head[k].type == GTokenType::Gt) d--;
                else if (head[k].type == GTokenType::Chain) d -= 2;
                if (d <= 0) { ++k; break; }
            }
        }
        while (k + 1 < head.size() && head[k].type == GTokenType::LBracket && head[k + 1].type == GTokenType::RBracket) k += 2;
        if (!(k == typeStart + 1 && head[typeStart].type == GTokenType::KwVar)) {
            std::string rawTy;
            for (size_t ti = typeStart; ti < k; ++ti) {
                if (head[ti].type == GTokenType::Comma) { rawTy += ","; continue; }
                if (!rawTy.empty() && !head[ti].value.empty() &&
                    (std::isalnum(static_cast<unsigned char>(rawTy.back())) || rawTy.back() == '_') &&
                    (std::isalnum(static_cast<unsigned char>(head[ti].value.front())) || head[ti].value.front() == '_')) rawTy += " ";
                rawTy += head[ti].value;
            }
            foreachElemSpec = validateAndNormalizeTypeSpec(rawTy, false);
            ensureTypeReady(foreachElemSpec);
        }
    }
    std::vector<std::string> vars;
    if (k < head.size() && head[k].type == GTokenType::Identifier) vars.push_back(head[k++].value);
    if (k < head.size() && head[k].type == GTokenType::Comma && k + 1 < head.size()) { vars.push_back(head[k + 1].value); k += 2; }
    if (vars.empty() || k >= head.size() || head[k].type != GTokenType::KwIn) throw std::runtime_error("foreach: use foreach (x in colecao) { ... }");
    std::vector<Token> coll(head.begin() + static_cast<long long>(k + 1), head.end());
    runForeach(vars, coll, body, foreachElemSpec);
}

// hypothesis (var x in valores) { ...; collapse_max(score); }
// valores: matriz/array antigo, lista, range(...) ou qualquer expressão que dê lista.
// Cada valor roda o bloco numa cópia do estado (linha do tempo); a de melhor score vira a realidade.
void GhostInterpreter::parseHypothesis(Lexer& lex) {
    std::vector<Token> head;
    {
        if (lex.getNextToken().type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos hypothesis.");
        int depth = 1;
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::EndOfFile) throw std::runtime_error("Faltou ')' em hypothesis.");
            if (t.type == GTokenType::LParen) depth++;
            if (t.type == GTokenType::RParen && --depth == 0) break;
            head.push_back(t);
        }
    }
    size_t k = 0;
    if (k < head.size() && head[k].type != GTokenType::Identifier) k++; // tipo opcional (var, int, double)
    if (k + 1 >= head.size() || head[k].type != GTokenType::Identifier || head[k + 1].type != GTokenType::KwIn) {
        throw std::runtime_error("hypothesis: use hypothesis (var x in valores) { ... collapse_max(score); }");
    }
    const std::string var = head[k].value;
    std::vector<Token> coll(head.begin() + static_cast<long long>(k + 2), head.end());
    if (lex.getNextToken().type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' apos a condicao de hypothesis.");
    const std::string block = lex.extractBlock();

    std::vector<GhostValue> values;
    if (coll.size() == 1 && coll[0].type == GTokenType::Identifier && memory.count(coll[0].value) && !coll_memory.count(coll[0].value)) {
        GhostMatrix& arr = memory[coll[0].value];
        for (size_t r = 0; r < arr.getRows(); ++r)
            for (size_t c = 0; c < arr.getCols(); ++c) values.push_back(GhostValue::number(arr(r, c).x));
    } else {
        GhostValue v = evalValue(coll);
        if (v.kind != GhostValue::List) throw std::runtime_error("hypothesis: '" + tokensToSource(coll) + "' nao e lista, range() nem matriz");
        values = v.list;
    }

    struct State {
        std::map<std::string, double> num;
        std::map<std::string, std::string> str;
        std::map<std::string, GhostMatrix> mem;
        std::map<std::string, GTokenType> type;
        std::map<std::string, GhostValue> coll;
    };
    auto snap = [&]() { return State{numeric_memory, string_memory, memory, type_memory, coll_memory}; };
    auto load = [&](const State& s) { numeric_memory = s.num; string_memory = s.str; memory = s.mem; type_memory = s.type; coll_memory = s.coll; };

    const State base = snap();
    State best;
    double bestScore = 0.0;
    bool has = false;
    for (const auto& val : values) {
        load(base);
        storeVar(var, val, GTokenType::KwVar);
        numeric_memory.erase("__collapse_score__");
        executeLine(block);
        if (!numeric_memory.count("__collapse_score__")) continue;
        const double score = numeric_memory["__collapse_score__"];
        const bool isMax = numeric_memory["__collapse_type__"] > 0;
        if (!has || (isMax ? score > bestScore : score < bestScore)) {
            has = true;
            bestScore = score;
            best = snap();
        }
    }
    if (has) {
        load(best);
        numeric_memory.erase("__collapse_score__");
        numeric_memory.erase("__collapse_type__");
        // a variável da hipótese não sobrevive ao bloco (como no compilador)
        if (!base.num.count(var)) numeric_memory.erase(var);
        if (!base.str.count(var)) string_memory.erase(var);
        if (!base.coll.count(var)) coll_memory.erase(var);
        std::cout << "[Multiverso] Colapso temporal resolvido. Linha do tempo de maior score (" << bestScore << ") assumida." << std::endl;
    } else {
        load(base);
    }
}

void GhostInterpreter::parseSpawn(Lexer& lex) {
    Token lBrace = lex.getNextToken();
    if (lBrace.type != GTokenType::LBrace) throw std::runtime_error("Esperado '{' apos spawn");
    std::string block = lex.extractBlock();
    
    std::thread([this, block]() {
        try {
            this->executeLine(block);
        } catch (const exception& e) {
            cerr << "[Erro de Execucao na Thread] " << e.what() << endl;
        }
    }).detach();
}

void GhostInterpreter::parseSleep(Lexer& lex) {
    Token lParen = lex.getNextToken();
    if (lParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos sleep");
    Token val = lex.getNextToken();
    int ms = 0;
    if (val.type == GTokenType::Number) {
        ms = std::stoi(val.value);
    } else if (val.type == GTokenType::Identifier && numeric_memory.count(val.value)) {
        ms = (int)numeric_memory[val.value];
    } else {
        throw std::runtime_error("Esperado valor numerico no sleep");
    }
    Token rParen = lex.getNextToken();
    if (rParen.type != GTokenType::RParen) throw std::runtime_error("Esperado ')' apos sleep");
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos sleep");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void GhostInterpreter::parseMutex(Lexer& lex) {
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Esperado nome para o mutex");
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos declaracao do mutex");
    
    mutex_memory[nameToken.value] = std::make_shared<std::mutex>();
}

void GhostInterpreter::parseLock(Lexer& lex) {
    Token lParen = lex.getNextToken();
    if (lParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos lock");
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Esperado nome do mutex");
    Token rParen = lex.getNextToken();
    if (rParen.type != GTokenType::RParen) throw std::runtime_error("Esperado ')' apos lock");
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos lock");
    
    if (mutex_memory.count(nameToken.value)) {
        mutex_memory[nameToken.value]->lock();
    } else {
        throw std::runtime_error("Mutex nao encontrado: " + nameToken.value);
    }
}

void GhostInterpreter::parseUnlock(Lexer& lex) {
    Token lParen = lex.getNextToken();
    if (lParen.type != GTokenType::LParen) throw std::runtime_error("Esperado '(' apos unlock");
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Esperado nome do mutex");
    Token rParen = lex.getNextToken();
    if (rParen.type != GTokenType::RParen) throw std::runtime_error("Esperado ')' apos unlock");
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) throw std::runtime_error("Esperado ';' apos unlock");
    
    if (mutex_memory.count(nameToken.value)) {
        mutex_memory[nameToken.value]->unlock();
    } else {
        throw std::runtime_error("Mutex nao encontrado: " + nameToken.value);
    }
}

void GhostInterpreter::parseServer(Lexer& lex) {
    Token nameToken = lex.getNextToken();
    if (nameToken.type != GTokenType::Identifier) throw std::runtime_error("Esperado nome para o servidor HTTP");
    Token eqToken = lex.getNextToken();
    if (eqToken.type != GTokenType::Assign) throw std::runtime_error("Esperado '=' apos nome do servidor HTTP");
    
    std::vector<Token> portTokens;
    Token t = lex.getNextToken();
    while (t.type != GTokenType::Semicolon && t.type != GTokenType::EndOfFile) {
        portTokens.push_back(t);
        t = lex.getNextToken();
    }
    if (portTokens.empty()) throw std::runtime_error("Esperado porta para o servidor HTTP");
    
    GhostValue portVal = evalValue(portTokens);
    int port = (int)portVal.num;
    server_memory[nameToken.value] = std::make_shared<GhostServer>(port);
}

bool GhostInterpreter::tryServerMethod(const std::string& objName, const Token& next, Lexer& lex) {
    if (!server_memory.count(objName) || next.type != GTokenType::Dot) {
        return false;
    }
    
    Token methodTok = lex.getNextToken();
    // 'route' e 'listen' sao palavras-chave no lexer, mas aqui sao nomes de metodo (app.route(...))
    const bool nomeValido = methodTok.type == GTokenType::Identifier ||
        (!methodTok.value.empty() && (std::isalpha(static_cast<unsigned char>(methodTok.value[0])) || methodTok.value[0] == '_'));
    if (!nomeValido) {
        throw std::runtime_error("Esperado nome do metodo apos '" + objName + ".'");
    }
    std::string method = methodTok.value;
    
    Token lParen = lex.getNextToken();
    if (lParen.type != GTokenType::LParen) {
        throw std::runtime_error("Esperado '(' apos '" + objName + "." + method + "'");
    }
    
    std::vector<Token> callTokens;
    callTokens.push_back(lParen);
    int parenDepth = 1;
    while (parenDepth > 0) {
        Token argTok = lex.getNextToken();
        if (argTok.type == GTokenType::EndOfFile) {
            throw std::runtime_error("Chamada de metodo HTTP incompleta: esperado ')'");
        }
        if (argTok.type == GTokenType::LParen) parenDepth++;
        else if (argTok.type == GTokenType::RParen) parenDepth--;
        callTokens.push_back(argTok);
    }
    
    Token semi = lex.getNextToken();
    if (semi.type != GTokenType::Semicolon) {
        throw std::runtime_error("Esperado ';' apos a chamada de " + objName + "." + method);
    }
    
    size_t idx = 0;
    auto args = splitArgs(callTokens, idx);
    auto srv = server_memory[objName];
    
    if (method == "route" || method == "get" || method == "post" || method == "put" || method == "del") {
        if (args.size() != 2) {
            throw std::runtime_error(objName + "." + method + " espera 2 argumentos: (rota, funcao_callback)");
        }
        std::string path = evalValue(args[0]).str;
        std::string handlerName = "";
        if (args[1].size() == 1 && args[1][0].type == GTokenType::Identifier) {
            handlerName = args[1][0].value;
        } else {
            GhostValue hv = evalValue(args[1]);
            if (hv.kind == GhostValue::Str) handlerName = hv.str;
        }
        
        auto handler = [this, handlerName](std::string reqBody) -> std::string {
            if (functions_memory.count(handlerName)) {
                const GhostFunction f = functions_memory.at(handlerName);
                std::vector<GhostValue> vals;
                GhostValue arg;
                arg.kind = GhostValue::Str;
                arg.str = reqBody;
                vals.push_back(arg);
                GhostValue res = runFunction(f, handlerName, vals, nullptr, "");
                if (res.kind == GhostValue::Str) return res.str;
                if (res.kind == GhostValue::Num) {
                    if (res.isInt) return std::to_string((long long)res.num);
                    return std::to_string(res.num);
                }
                return "";
            }
            return "";
        };
        
        if (method == "route") srv->route(path, handler);
        else if (method == "get") srv->get(path, handler);
        else if (method == "post") srv->post(path, handler);
        else if (method == "put") srv->put(path, handler);
        else if (method == "del") srv->del(path, handler);
    } else if (method == "serve") {
        if (args.size() != 2) {
            throw std::runtime_error(objName + ".serve espera 2 argumentos: (prefixo_url, diretorio_local)");
        }
        std::string prefix = evalValue(args[0]).str;
        std::string dir = evalValue(args[1]).str;
        srv->serve(prefix, dir);
    } else if (method == "listen_step") {
        int timeout = 0;
        if (!args.empty()) {
            timeout = (int)evalValue(args[0]).num;
        }
        srv->listen_step(timeout);
    } else if (method == "listen") {
        srv->listen();
    } else {
        throw std::runtime_error("Metodo de servidor HTTP desconhecido: " + method);
    }
    
    return true;
}

bool GhostInterpreter::tryHybridStatement(const std::string& objName, const Token& next, Lexer& lex) {
    if (next.type != GTokenType::Dot) return false;
    
    if (qlayer_memory.count(objName)) {
        Token mTok = lex.getNextToken();
        if (mTok.type != GTokenType::Identifier) throw runtime_error("Esperado nome do metodo de qlayer");
        Token lp = lex.getNextToken();
        if (lp.type != GTokenType::LParen) throw runtime_error("Esperado '('");
        vector<Token> argToks;
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::RParen || t.type == GTokenType::EndOfFile) break;
            argToks.push_back(t);
        }
        Token semi = lex.getNextToken();
        if (semi.type != GTokenType::Semicolon) throw runtime_error("Esperado ';' apos chamada de qlayer");
        
        if (mTok.value == "seed") {
            unsigned long long s = (unsigned long long)evaluateMath(argToks);
            qlayer_memory[objName]->seed(s);
            return true;
        }
        throw runtime_error("Metodo desconhecido de qlayer: " + mTok.value);
    }
    
    if (hybrid_memory.count(objName)) {
        Token mTok = lex.getNextToken();
        if (mTok.type != GTokenType::Identifier) throw runtime_error("Esperado nome do metodo de rede hibrida");
        Token lp = lex.getNextToken();
        if (lp.type != GTokenType::LParen) throw runtime_error("Esperado '('");
        vector<Token> argToks;
        while (true) {
            Token t = lex.getNextToken();
            if (t.type == GTokenType::RParen || t.type == GTokenType::EndOfFile) break;
            argToks.push_back(t);
        }
        Token semi = lex.getNextToken();
        if (semi.type != GTokenType::Semicolon) throw runtime_error("Esperado ';' apos chamada de rede hibrida");
        
        if (mTok.value == "set_verbose") {
            int v = (int)evaluateMath(argToks);
            hybrid_memory[objName]->set_verbose(v);
            return true;
        }
        throw runtime_error("Metodo de instrucao desconhecido de rede hibrida: " + mTok.value);
    }
    
    return false;
}
