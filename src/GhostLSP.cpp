#include "../include/GhostLSP.hpp"
#include "../include/GhostAst.hpp"
#include "../include/GhostDiag.hpp"
#include "../include/GhostTypes.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace GhostLanguageServer {

GhostLSP::GhostLSP() : running(true) {}

std::string GhostLSP::escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if ((unsigned char)c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
            out += buf;
        } else {
            out += c;
        }
    }
    return out;
}

static std::string extractJsonString(const std::string& json, const std::string& key) {
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t end = pos + 1;
    while (end < json.size()) {
        if (json[end] == '"' && json[end - 1] != '\\') break;
        ++end;
    }
    if (end >= json.size()) return "";
    std::string val = json.substr(pos + 1, end - pos - 1);
    // Desescapar caracteres básicos
    std::string unescaped;
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            char next = val[i + 1];
            if (next == 'n') { unescaped += '\n'; ++i; }
            else if (next == 'r') { unescaped += '\r'; ++i; }
            else if (next == 't') { unescaped += '\t'; ++i; }
            else if (next == '"') { unescaped += '"'; ++i; }
            else if (next == '\\') { unescaped += '\\'; ++i; }
            else { unescaped += next; ++i; }
        } else {
            unescaped += val[i];
        }
    }
    return unescaped;
}

static std::string extractJsonId(const std::string& json) {
    size_t pos = json.find("\"id\"");
    if (pos == std::string::npos) return "null";
    pos = json.find(':', pos + 4);
    if (pos == std::string::npos) return "null";
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "null";
    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        return (end != std::string::npos) ? json.substr(pos, end - pos + 1) : "null";
    }
    size_t end = pos;
    while (end < json.size() && (isdigit((unsigned char)json[end]) || json[end] == '-')) ++end;
    return json.substr(pos, end - pos);
}

static int extractJsonInt(const std::string& json, const std::string& key, int defaultVal = 0) {
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos) return defaultVal;
    while (pos < json.size() && !isdigit((unsigned char)json[pos]) && json[pos] != '-') ++pos;
    if (pos >= json.size()) return defaultVal;
    return std::atoi(json.c_str() + pos);
}

std::string GhostLSP::getWordAt(const std::string& text, int line, int character) {
    std::istringstream stream(text);
    std::string curLine;
    int curLineIdx = 0;
    while (std::getline(stream, curLine)) {
        if (curLineIdx == line) {
            if (character < 0 || character >= (int)curLine.size()) return "";
            int start = character;
            while (start > 0 && (isalnum((unsigned char)curLine[start - 1]) || curLine[start - 1] == '_')) {
                --start;
            }
            int end = character;
            while (end < (int)curLine.size() && (isalnum((unsigned char)curLine[end]) || curLine[end] == '_')) {
                ++end;
            }
            return curLine.substr(start, end - start);
        }
        ++curLineIdx;
    }
    return "";
}

void GhostLSP::sendResponse(const std::string& id, const std::string& result) {
    std::string content = "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}";
    std::cout << "Content-Length: " << content.length() << "\r\n\r\n" << content << std::flush;
}

void GhostLSP::sendNotification(const std::string& method, const std::string& params) {
    std::string content = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method + "\",\"params\":" + params + "}";
    std::cout << "Content-Length: " << content.length() << "\r\n\r\n" << content << std::flush;
}

void GhostLSP::publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics) {
    std::stringstream ss;
    ss << "{\"uri\":\"" << escapeJson(uri) << "\",\"diagnostics\":[";
    for (size_t i = 0; i < diagnostics.size(); ++i) {
        const auto& d = diagnostics[i];
        ss << "{"
           << "\"range\":{"
           << "\"start\":{\"line\":" << d.range.start.line << ",\"character\":" << d.range.start.character << "},"
           << "\"end\":{\"line\":" << d.range.end.line << ",\"character\":" << d.range.end.character << "}"
           << "},"
           << "\"severity\":" << d.severity << ","
           << "\"code\":\"" << escapeJson(d.code) << "\","
           << "\"message\":\"" << escapeJson(d.message) << "\","
           << "\"source\":\"ghost-lsp\""
           << "}";
        if (i + 1 < diagnostics.size()) ss << ",";
    }
    ss << "]}";
    sendNotification("textDocument/publishDiagnostics", ss.str());
}

void GhostLSP::validateDocument(const std::string& uri, const std::string& text) {
    std::vector<Diagnostic> diagnostics;
    try {
        AstParseResult res = ghost_ast_parse(text);
        if (res.errors.empty() && res.program) {
            res.errors = ghost_typecheck(*res.program);
        }
        for (const auto& err : res.errors) {
            Diagnostic diag;
            int ln = (err.line > 0) ? (err.line - 1) : 0;
            int col = (err.col > 0) ? (err.col - 1) : 0;
            diag.range.start = { ln, col };
            diag.range.end = { ln, col + 4 };
            diag.severity = 1; // Error
            diag.code = err.code.empty() ? "GHOST-E1000" : err.code;
            diag.message = err.msg;
            diagnostics.push_back(diag);
        }
    } catch (...) {
        // Falha de parser capturada sem derrubar o servidor
    }
    publishDiagnostics(uri, diagnostics);
}

void GhostLSP::handleInitialize(const std::string& id) {
    std::string res = 
        "{"
          "\"capabilities\":{"
            "\"textDocumentSync\":1,"
            "\"hoverProvider\":true,"
            "\"completionProvider\":{"
              "\"resolveProvider\":false,"
              "\"triggerCharacters\":[\".\",\":\"]"
            "},"
            "\"definitionProvider\":true"
          "},"
          "\"serverInfo\":{"
            "\"name\":\"ghost-lsp\","
            "\"version\":\"1.0.0\""
          "}"
        "}";
    sendResponse(id, res);
}

void GhostLSP::handleShutdown(const std::string& id) {
    running = false;
    sendResponse(id, "null");
}

void GhostLSP::handleDidOpen(const std::string& uri, const std::string& text) {
    documents[uri] = text;
    validateDocument(uri, text);
}

void GhostLSP::handleDidChange(const std::string& uri, const std::string& text) {
    documents[uri] = text;
    validateDocument(uri, text);
}

void GhostLSP::handleDidClose(const std::string& uri) {
    documents.erase(uri);
    // Limpar diagnósticos no cliente
    publishDiagnostics(uri, {});
}

void GhostLSP::handleHover(const std::string& id, const std::string& uri, int line, int col) {
    auto it = documents.find(uri);
    if (it == documents.end()) {
        sendResponse(id, "null");
        return;
    }
    std::string word = getWordAt(it->second, line, col);
    if (word.empty()) {
        sendResponse(id, "null");
        return;
    }

    static const std::map<std::string, std::string> docs = {
        {"fun", "### Palavra-chave: `fun`\nDeclara uma função ou método em GhostLang.\n```ghost\nfun soma(a: int, b: int): int {\n    retorne a + b;\n}\n```"},
        {"retorne", "### Palavra-chave: `retorne`\nFinaliza a execução da função e devolve o valor especificado ao chamador."},
        {"se", "### Controle de Fluxo: `se`\nEstrutura condicional binária.\n```ghost\nse (x > 0) {\n    escreva(\"positivo\");\n} senao {\n    escreva(\"nao positivo\");\n}\n```"},
        {"senao", "### Controle de Fluxo: `senao`\nBloco executado quando a condição do `se` for falsa."},
        {"para", "### Laço: `para`\nIteração por intervalo ou sobre elementos de coleções.\n```ghost\npara (x em lista) { escrevaln(x); }\n```"},
        {"enquanto", "### Laço: `enquanto`\nExecuta o bloco enquanto a condição permanecer verdadeira."},
        {"classe", "### POO: `classe`\nDeclaração de modelo orientado a objetos com propriedades e métodos."},
        {"interface", "### Contrato: `interface`\nDeclaração de contrato de tipos com assinaturas abstratas."},
        {"qubit", "### Quântico: `qubit`\nAloca registradores quânticos ou qubits individuais.\n```ghost\nqubit q[2];\n```"},
        {"hadamard", "### Porta Quântica: `hadamard(q)`\nCria superposição quântica equiprovável $|+\\rangle$ ou $|-\\rangle$."},
        {"cnot", "### Porta Quântica: `cnot(controle, alvo)`\nInverte o estado do qubit alvo condicionado ao controle."},
        {"medir", "### Medição: `medir(qubit)`\nRealiza colapso projetivo na base computacional Z, retornando 0 ou 1."},
        {"pauli_x", "### Porta Quântica: `pauli_x(q)`\nInversão de bit quântico (NOT quântico / rotação $\\pi$ em X)."},
        {"pauli_y", "### Porta Quântica: `pauli_y(q)`\nRotação de bit e fase $\\pi$ em Y."},
        {"pauli_z", "### Porta Quântica: `pauli_z(q)`\nInversão de fase $\\pi$ em Z."},
        {"fase", "### Porta Quântica: `fase(theta, q)`\nAplica rotação de fase parametrizada $\\theta$ em torno de Z."},
        {"swap", "### Porta Quântica: `swap(q1, q2)`\nPermuta os estados quânticos de dois qubits."},
        {"escreva", "### Função Builtin: `escreva(...)`\nEscreve argumentos formatados na saída padrão sem quebra de linha."},
        {"escrevaln", "### Função Builtin: `escrevaln(...)`\nEscreve argumentos na saída padrão seguido de quebra de linha."},
        {"leia", "### Função Builtin: `leia(): string`\nLê uma linha de texto a partir da entrada padrão (stdin)."},
        {"tamanho", "### Função Builtin: `tamanho(colecao): int`\nRetorna o número de elementos de uma lista, texto ou vetor."},
        {"matriz", "### Álgebra Linear: `matriz(linhas, colunas)`\nInstancia matriz bidimensional de alta performance com suporte a CPU/GPU."},
        {"tensor", "### Tensores: `tensor(dimensoes...)`\nEstrutura multidimensional otimizada para modelos de IA e autograd."},
        {"gui_alert", "### GhostGUI: `gui_alert(titulo, mensagem)`\nExibe caixa de diálogo nativa de alerta na interface gráfica desktop."},
        {"gui_confirm", "### GhostGUI: `gui_confirm(titulo, mensagem): bool`\nExibe diálogo nativa de confirmação Sim/Não."},
        {"gui_create_window", "### GhostGUI: `gui_create_window(titulo, larg, alt): int`\nCria janela nativa do GhostGUI e retorna identificador de handle."},
        {"db_abrir", "### GhostDB: `db_abrir(caminho): int`\nAbre conexão com banco de dados SQLite local e retorna handle de conexão."},
        {"db_executar", "### GhostDB: `db_executar(conn, sql): int`\nExecuta comando SQL de criação ou manipulação (INSERT, UPDATE, DELETE)."},
        {"db_consultar", "### GhostDB: `db_consultar(conn, sql): lista`\nExecuta consulta SQL (SELECT) e devolve linhas como matriz/lista."}
    };

    auto docIt = docs.find(word);
    if (docIt != docs.end()) {
        std::string res = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" + escapeJson(docIt->second) + "\"}}";
        sendResponse(id, res);
    } else {
        std::string res = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"### Símbolo GhostLang: `" + escapeJson(word) + "`\\nIdentificador definido no código-fonte.\"}}";
        sendResponse(id, res);
    }
}

void GhostLSP::handleCompletion(const std::string& id, const std::string& uri, int line, int col) {
    (void)uri; (void)line; (void)col;
    static const std::vector<CompletionItem> items = {
        {"fun", "fun nome(): tipo", "Declaração de função", 14},
        {"retorne", "retorne valor", "Retorna valor da função", 14},
        {"se", "se (cond) { }", "Estrutura condicional", 14},
        {"senao", "senao { }", "Bloco alternativo", 14},
        {"para", "para (item em lista)", "Laço de repetição", 14},
        {"enquanto", "enquanto (cond)", "Laço condicional", 14},
        {"classe", "classe Nome { }", "Declaração de classe", 7},
        {"interface", "interface Nome { }", "Contrato de interface", 7},
        {"qubit", "qubit q[n]", "Alocação de registradores quânticos", 14},
        {"hadamard", "hadamard(q)", "Porta quântica de superposição Hadamard", 3},
        {"cnot", "cnot(ctrl, alvo)", "Porta quântica NOT-controlada", 3},
        {"medir", "medir(q)", "Medição quântica de qubit", 3},
        {"pauli_x", "pauli_x(q)", "Porta Pauli-X (NOT)", 3},
        {"pauli_y", "pauli_y(q)", "Porta Pauli-Y", 3},
        {"pauli_z", "pauli_z(q)", "Porta Pauli-Z", 3},
        {"fase", "fase(theta, q)", "Rotação de fase quântica", 3},
        {"swap", "swap(q1, q2)", "Permutação quântica de qubits", 3},
        {"escreva", "escreva(args...)", "Impressão sem nova linha", 3},
        {"escrevaln", "escrevaln(args...)", "Impressão com nova linha", 3},
        {"leia", "leia()", "Leitura da entrada padrão", 3},
        {"tamanho", "tamanho(colecao)", "Retorna cardinalidade da coleção", 3},
        {"matriz", "matriz(linhas, cols)", "Cria matriz bidimensional", 3},
        {"gui_alert", "gui_alert(titulo, msg)", "Alerta nativo desktop", 3},
        {"gui_confirm", "gui_confirm(titulo, msg)", "Confirmação nativa desktop", 3},
        {"db_abrir", "db_abrir(caminho)", "Abre banco SQLite local", 3},
        {"db_executar", "db_executar(conn, sql)", "Executa comando SQL", 3},
        {"db_consultar", "db_consultar(conn, sql)", "Consulta SQL com retorno", 3}
    };

    std::stringstream ss;
    ss << "{\"isIncomplete\":false,\"items\":[";
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& it = items[i];
        ss << "{"
           << "\"label\":\"" << escapeJson(it.label) << "\","
           << "\"kind\":" << it.kind << ","
           << "\"detail\":\"" << escapeJson(it.detail) << "\","
           << "\"documentation\":\"" << escapeJson(it.documentation) << "\""
           << "}";
        if (i + 1 < items.size()) ss << ",";
    }
    ss << "]}";
    sendResponse(id, ss.str());
}

void GhostLSP::handleDefinition(const std::string& id, const std::string& uri, int line, int col) {
    auto it = documents.find(uri);
    if (it == documents.end()) {
        sendResponse(id, "null");
        return;
    }
    std::string word = getWordAt(it->second, line, col);
    if (word.empty()) {
        sendResponse(id, "null");
        return;
    }

    std::istringstream stream(it->second);
    std::string curLine;
    int curLineIdx = 0;
    while (std::getline(stream, curLine)) {
        size_t pFun = curLine.find("fun " + word);
        size_t pCls = curLine.find("classe " + word);
        if (pFun != std::string::npos || pCls != std::string::npos) {
            size_t p = (pFun != std::string::npos) ? (pFun + 4) : (pCls + 7);
            std::stringstream ss;
            ss << "{\"uri\":\"" << escapeJson(uri) << "\","
               << "\"range\":{"
               << "\"start\":{\"line\":" << curLineIdx << ",\"character\":" << p << "},"
               << "\"end\":{\"line\":" << curLineIdx << ",\"character\":" << (p + word.size()) << "}"
               << "}}";
            sendResponse(id, ss.str());
            return;
        }
        ++curLineIdx;
    }
    sendResponse(id, "null");
}

void GhostLSP::processMessage(const std::string& content) {
    std::string method = extractJsonString(content, "method");
    std::string id = extractJsonId(content);

    if (method == "initialize") {
        handleInitialize(id);
    } else if (method == "initialized") {
        // Notificação de cliente pronto
    } else if (method == "shutdown") {
        handleShutdown(id);
    } else if (method == "exit") {
        running = false;
    } else if (method == "textDocument/didOpen") {
        std::string uri = extractJsonString(content, "uri");
        std::string text = extractJsonString(content, "text");
        handleDidOpen(uri, text);
    } else if (method == "textDocument/didChange") {
        std::string uri = extractJsonString(content, "uri");
        std::string text = extractJsonString(content, "text");
        handleDidChange(uri, text);
    } else if (method == "textDocument/didClose") {
        std::string uri = extractJsonString(content, "uri");
        handleDidClose(uri);
    } else if (method == "textDocument/hover") {
        std::string uri = extractJsonString(content, "uri");
        int line = extractJsonInt(content, "line", 0);
        int col = extractJsonInt(content, "character", 0);
        handleHover(id, uri, line, col);
    } else if (method == "textDocument/completion") {
        std::string uri = extractJsonString(content, "uri");
        int line = extractJsonInt(content, "line", 0);
        int col = extractJsonInt(content, "character", 0);
        handleCompletion(id, uri, line, col);
    } else if (method == "textDocument/definition") {
        std::string uri = extractJsonString(content, "uri");
        int line = extractJsonInt(content, "line", 0);
        int col = extractJsonInt(content, "character", 0);
        handleDefinition(id, uri, line, col);
    } else {
        if (id != "null") {
            sendResponse(id, "null");
        }
    }
}

void GhostLSP::run() {
    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (line.substr(0, 16) == "Content-Length: ") {
            int length = std::stoi(line.substr(16));
            // Lê linha em branco restante (\r\n)
            std::getline(std::cin, line);

            std::vector<char> buffer(length);
            std::cin.read(buffer.data(), length);
            std::string content(buffer.data(), length);

            processMessage(content);
        }
    }
}

int GhostLSP::runTestMode() {
    // Teste sintético de conformidade JSON-RPC 2.0 e AST Diagnostics
    std::string initMsg = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";
    processMessage(initMsg);

    // Documento com código válido
    std::string openMsgValid = "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///teste.g\",\"text\":\"fun ola(): texto { retorne \\\"mundo\\\"; }\"}}}";
    processMessage(openMsgValid);

    // Hover sobre 'fun'
    std::string hoverMsg = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///teste.g\"},\"position\":{\"line\":0,\"character\":2}}}";
    processMessage(hoverMsg);

    // Completion
    std::string compMsg = "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///teste.g\"},\"position\":{\"line\":0,\"character\":2}}}";
    processMessage(compMsg);

    // Documento com erro de sintaxe para validar diagnósticos
    std::string openMsgError = "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///erro.g\",\"text\":\"fun quebrado( { retorne; }\"}}}";
    processMessage(openMsgError);

    // Shutdown
    std::string shutMsg = "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"shutdown\",\"params\":{}}";
    processMessage(shutMsg);

    return 0;
}

} // namespace GhostLanguageServer
