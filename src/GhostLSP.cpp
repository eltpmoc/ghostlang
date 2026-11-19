#include "../include/GhostLSP.hpp"
#include "../include/GhostAst.hpp"
#include "../include/GhostDiag.hpp"
#include "../include/GhostTypes.hpp"
#include "../include/Lexer.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>

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
    if (capture) { capture->push_back(content); return; }
    std::cout << "Content-Length: " << content.length() << "\r\n\r\n" << content << std::flush;
}

void GhostLSP::sendNotification(const std::string& method, const std::string& params) {
    std::string content = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method + "\",\"params\":" + params + "}";
    if (capture) { capture->push_back(content); return; }
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

    // Documentação das palavras-chave mais usadas, com a sintaxe real (conferida nos testes)
    static const std::map<std::string, std::string> docs = {
        {"function", "### `function`\nDeclara uma função. O tipo de retorno é opcional.\n```ghost\nfunction int soma(int a, int b) {\n    return a + b;\n}\n```"},
        {"return", "### `return`\nEncerra a função e devolve o valor ao chamador."},
        {"var", "### `var`\nDeclara uma variável com tipo inferido.\n```ghost\nvar total = 0;\n```"},
        {"if", "### `if` / `else`\n```ghost\nif (x > 0) {\n    print(\"positivo\");\n} else {\n    print(\"nao positivo\");\n}\n```"},
        {"else", "### `else`\nBloco executado quando a condição do `if` é falsa."},
        {"for", "### `for`\n```ghost\nfor (int i = 0; i < 10; i++) {\n    print(i);\n}\n```"},
        {"foreach", "### `foreach`\nPercorre os elementos de uma coleção.\n```ghost\nforeach (x in lista) {\n    print(x);\n}\n```"},
        {"while", "### `while`\nRepete o bloco enquanto a condição for verdadeira."},
        {"switch", "### `switch` / `case` / `default`\nEscolhe um bloco pelo valor da expressão."},
        {"class", "### `class`\nDeclara uma classe com campos, construtor e métodos; `extends` herda de outra classe."},
        {"import", "### `import`\nImporta outro arquivo .g.\n```ghost\nimport \"libs/tensor.g\" as tn;\n```"},
        {"try", "### `try` / `catch` / `finally`\n```ghost\ntry {\n    use_device(\"vulkan\");\n} catch (e) {\n    print(\"erro: \", e);\n}\n```"},
        {"print", "### `print(...)`\nEscreve os argumentos na saída padrão, seguidos de quebra de linha."},
        {"show", "### `show(x)`\nMostra o valor de um estado, matriz ou número G."},
        {"state", "### `state`\nVetor de estado quântico.\n```ghost\nstate psi = [1.0, 0.0];\n```"},
        {"gate", "### `gate`\nMatriz (linhas x colunas).\n```ghost\ngate m(2, 3);\n```"},
        {"qreg", "### `qreg`\nRegistrador quântico simulado.\n```ghost\nqreg q(2);\n```"},
        {"qcircuit", "### `qcircuit`\nCircuito quântico parametrizado (QML)."},
        {"ghost", "### `ghost`\nNúmero G (álgebra hipercomplexa de 4 componentes).\n```ghost\nghost g = ghost(1.0, 0.0, 0.0, 0.0);\n```"},
        {"hypothesis", "### `hypothesis`\nAvalia o bloco para cada valor e escolhe o melhor com `collapse_max`/`collapse_min`.\n```ghost\nhypothesis (var t in [7, 3, 9]) { ... }\n```"},
        {"spawn", "### `spawn`\nExecuta o bloco em outra thread."}
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
    // Palavras-chave reais do lexer + funções declaradas nas libs/*.g
    static const std::vector<CompletionItem> keywordItems = [] {
        std::vector<CompletionItem> v;
        for (const auto& k : Lexer::keywordNames()) v.push_back({k, "palavra-chave", "", 14});
        return v;
    }();
    std::vector<CompletionItem> items = keywordItems;
    items.insert(items.end(), libItems.begin(), libItems.end());

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

    // Declarações reais: function [tipo] nome(, class/interface nome, var nome
    const std::regex decl("(^|[^\\w])(?:function\\s+(?:[A-Za-z_][\\w<>,]*\\s+)?|class\\s+|interface\\s+|var\\s+)(" + word + ")\\b");
    std::istringstream stream(it->second);
    std::string curLine;
    int curLineIdx = 0;
    while (std::getline(stream, curLine)) {
        std::smatch m;
        if (std::regex_search(curLine, m, decl)) {
            const size_t p = static_cast<size_t>(m.position(2));
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

void GhostLSP::setRoot(const std::string& root) {
    namespace fs = std::filesystem;
    libItems.clear();
    std::error_code ec;
    const fs::path libs = fs::path(root) / "libs";
    if (!fs::is_directory(libs, ec)) return;
    static const std::regex decl(R"(^\s*function\s+(?:[A-Za-z_][\w<>,\s]*?\s+)?([A-Za-z_]\w*)\s*\()");
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(libs, ec))
        if (e.path().extension() == ".g") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    std::set<std::string> seen;
    for (const auto& f : files) {
        std::ifstream in(f);
        std::string line;
        while (std::getline(in, line)) {
            std::smatch m;
            if (std::regex_search(line, m, decl) && seen.insert(m[1]).second)
                libItems.push_back({m[1], "libs/" + f.filename().string(), "", 3});
        }
    }
}

int GhostLSP::runTestMode() {
    // Teste de conformidade: confere as respostas, não só se o servidor roda.
    std::vector<std::string> out;
    capture = &out;
    int falhas = 0;
    auto confere = [&](bool ok, const std::string& o_que) {
        std::cout << (ok ? "[ok]    " : "[FALHA] ") << o_que << std::endl;
        if (!ok) ++falhas;
    };
    auto ultima = [&]() { return out.empty() ? std::string() : out.back(); };

    processMessage("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}");
    confere(ultima().find("\"capabilities\"") != std::string::npos, "initialize devolve capabilities");

    // documento válido: nenhum diagnóstico
    processMessage("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///ok.g\",\"text\":\"function int soma(int a, int b) {\\n    return a + b;\\n}\\nfunction main() {\\n    print(soma(1, 2));\\n}\\n\"}}}");
    confere(ultima().find("\"diagnostics\":[]") != std::string::npos, "codigo valido sem diagnosticos");

    // hover em 'function' (linha 0, coluna 3)
    processMessage("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"textDocument/hover\",\"params\":{\"textDocument\":{\"uri\":\"file:///ok.g\"},\"position\":{\"line\":0,\"character\":3}}}");
    confere(ultima().find("### `function`") != std::string::npos, "hover de function");

    // completion: palavras reais, sem o vocabulário inventado
    processMessage("{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"file:///ok.g\"},\"position\":{\"line\":0,\"character\":0}}}");
    const std::string comp = ultima();
    for (const char* k : {"function", "while", "foreach", "qreg", "hypothesis"})
        confere(comp.find(std::string("\"label\":\"") + k + "\"") != std::string::npos, std::string("completion tem '") + k + "'");
    for (const char* k : {"fun", "retorne", "senao", "enquanto", "db_abrir"})
        confere(comp.find(std::string("\"label\":\"") + k + "\"") == std::string::npos, std::string("completion nao tem '") + k + "'");
    confere(!libItems.empty() && comp.find("\"label\":\"tensor_matmul\"") != std::string::npos, "completion tem tensor_matmul (libs/tensor.g)");

    // documento com erro: diagnóstico com código e linha
    processMessage("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"file:///erro.g\",\"text\":\"function main() {\\n    int 10 = 3;\\n}\\n\"}}}");
    const std::string diag = ultima();
    confere(diag.find("GHOST-E") != std::string::npos && diag.find("\"line\":1") != std::string::npos, "erro de sintaxe vira diagnostico na linha 2");

    // definition: soma (linha 4) -> declaração na linha 0
    processMessage("{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"textDocument/definition\",\"params\":{\"textDocument\":{\"uri\":\"file:///ok.g\"},\"position\":{\"line\":4,\"character\":12}}}");
    confere(ultima().find("\"line\":0") != std::string::npos, "definition de soma aponta a linha 1");

    processMessage("{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"shutdown\",\"params\":{}}");
    capture = nullptr;
    std::cout << (falhas ? "LSP: " + std::to_string(falhas) + " falha(s)" : std::string("LSP: tudo certo")) << std::endl;
    return falhas ? 1 : 0;
}

} // namespace GhostLanguageServer
