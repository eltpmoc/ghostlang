#ifndef GHOST_LSP_HPP
#define GHOST_LSP_HPP

#include <string>
#include <vector>
#include <map>

namespace GhostLanguageServer {

struct Position {
    int line = 0;       // 0-based
    int character = 0;  // 0-based
};

struct Range {
    Position start;
    Position end;
};

struct Diagnostic {
    Range range;
    int severity = 1; // 1 = Erro, 2 = Aviso, 3 = Info, 4 = Dica
    std::string code;
    std::string message;
};

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string documentation;
    int kind = 1; // 1=Text, 3=Function, 7=Class, 14=Keyword, 15=Snippet
};

class GhostLSP {
public:
    GhostLSP();
    void run();
    int runTestMode();
    // Raiz da GhostLang: o auto-completar lista as funções declaradas em <raiz>/libs/*.g
    void setRoot(const std::string& root);

private:
    bool running = true;
    std::map<std::string, std::string> documents;
    std::vector<CompletionItem> libItems;        // funções das libs/*.g
    std::vector<std::string>* capture = nullptr; // modo de teste: guarda as mensagens em vez de escrever

    void processMessage(const std::string& content);
    void handleInitialize(const std::string& id);
    void handleShutdown(const std::string& id);
    void handleDidOpen(const std::string& uri, const std::string& text);
    void handleDidChange(const std::string& uri, const std::string& text);
    void handleDidClose(const std::string& uri);
    void handleHover(const std::string& id, const std::string& uri, int line, int col);
    void handleCompletion(const std::string& id, const std::string& uri, int line, int col);
    void handleDefinition(const std::string& id, const std::string& uri, int line, int col);

    void validateDocument(const std::string& uri, const std::string& text);
    void publishDiagnostics(const std::string& uri, const std::vector<Diagnostic>& diagnostics);

    void sendResponse(const std::string& id, const std::string& result);
    void sendNotification(const std::string& method, const std::string& params);

    std::string getWordAt(const std::string& text, int line, int character);
    std::string escapeJson(const std::string& s);
};

} // namespace GhostLanguageServer

#endif // GHOST_LSP_HPP
