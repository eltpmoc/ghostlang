// GhostDiag.hpp - diagnósticos do compilador da GhostLang (c42 do dashboard).
//
// Formato (o mesmo do gcc/clang, reconhecido pelos editores):
//
//   tests/ghost/x.g:5:12: erro GHOST-E0102: esperado ';' depois de x
//       5 |     int x = 3 **
//         |            ^
//
// - Os imports são resolvidos juntando o texto dos arquivos; o mapa de linhas leva a linha do texto
//   juntado de volta ao arquivo e à linha certos.
// - Códigos por categoria (tabela em docs/NUCLEO.md, seção "Diagnósticos do compilador"):
//   E01 sintaxe, E02 declarações e tipos, E03 classes, E04 interfaces, E05 genéricos e restrições,
//   E06 funções e lambdas, E07 result e operador ?, E08 coleções e compreensões, E09 quântico,
//   E10 módulos (import), E90 erro no C++ gerado (o g++ recusou o programa traduzido).
#ifndef GHOST_DIAG_HPP
#define GHOST_DIAG_HPP

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "GhostModules.hpp"

struct GhostDiag {
    // linha do texto juntado (1, 2, ...) -> (arquivo, linha no arquivo)
    std::vector<std::pair<std::string, int>> lineMap;
    std::map<std::string, std::vector<std::string>> fileLines;
    int curLine = 0, curCol = 0;   // token atual do Parser (para a coluna do erro)
    std::string cppFile;           // nome do .cpp gerado (para voltar a numeração depois dos #line)
    // Se definido, recebe o erro fatal no lugar da impressão (a AST junta os erros de sintaxe dela)
    std::function<void(int line, int col, const std::string& code, const std::string& msg)> onFatal;
    static GhostDiag& get() {
        static GhostDiag d;
        return d;
    }
    // (arquivo, linha) de uma linha do texto juntado; sem mapa: ("", linha)
    std::pair<std::string, int> locate(int line) const {
        if (line >= 1 && static_cast<size_t>(line) <= lineMap.size()) return lineMap[static_cast<size_t>(line) - 1];
        return {"", line};
    }
    std::string sourceLine(const std::string& file, int line) const {
        auto it = fileLines.find(file);
        if (it == fileLines.end() || line < 1 || static_cast<size_t>(line) > it->second.size()) return "";
        return it->second[static_cast<size_t>(line) - 1];
    }
};

// Categoria do erro pela mensagem (a ordem importa: do mais específico para o geral)
inline std::string ghost_diag_code(const std::string& msgIn) {
    std::string m = msgIn;
    std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto has = [&](const char* k) { return m.find(k) != std::string::npos; };
    if (has("import")) return "GHOST-E1001";
    if (has("operador ?") || has("result<")) return "GHOST-E0701";
    if (has("circuit") || has("qreg") || has("qcircuit") || has("qlayer") || has("qubit") || has("quantic") || has("porta"))
        return "GHOST-E0901";
    if (has("compreensao") || has("lista") || has("mapa") || has("conjunto") || has("set<") || has("map<") || has("list<"))
        return "GHOST-E0801";
    if (has("generic") || has("restricao") || has("entre < >") || has("tipo(s)") || has("depender de")) return "GHOST-E0501";
    if (has("interface") || has("implements")) return "GHOST-E0401";
    if (has("class") || has("super") || has("construtor") || has("static") || has("protected") || has("private") ||
        has("metodo") || has("campo"))
        return "GHOST-E0301";
    if (has("funcao") || has("function") || has("lambda") || has("parametro") || has("captur") || has("retorn") || has("main"))
        return "GHOST-E0601";
    if (has("variavel") || has("tipo") || has("void") || has("var ") || has("declar")) return "GHOST-E0201";
    if (has("token inesperado")) return "GHOST-E0101";
    if (has("esperado") || has("faltou")) return "GHOST-E0102";
    if (has("desconhecid")) return "GHOST-E0103";
    return "GHOST-E0100";
}

// Imprime "arquivo:linha:coluna: erro CODIGO: mensagem" e o trecho com o marcador ^.
inline void ghost_diag_print(const std::string& file, int line, int col, const std::string& code, const std::string& msg,
                             std::ostream& os = std::cerr) {
    os << (file.empty() ? std::string("<entrada>") : file) << ":" << line;
    if (col > 0) os << ":" << col;
    os << ": erro " << code << ": " << ghost_module_pretty(msg) << "\n";
    const std::string src = GhostDiag::get().sourceLine(file, line);
    if (!src.empty()) {
        const std::string num = std::to_string(line);
        os << " " << num << " | " << src << "\n";
        if (col > 0) {
            std::string pad;
            for (int k = 1; k < col && static_cast<size_t>(k - 1) < src.size(); ++k) pad += src[static_cast<size_t>(k - 1)] == '\t' ? '\t' : ' ';
            os << " " << std::string(num.size(), ' ') << " | " << pad << "^\n";
        }
    }
}

// Erro do compilador: line é a linha do texto juntado (-1 = token atual). A coluna vem do token atual
// quando o erro é na mesma linha.
[[noreturn]] inline void ghost_diag_fail(int line, const std::string& msg, const std::string& code = "") {
    GhostDiag& d = GhostDiag::get();
    if (line < 0) line = d.curLine;
    const int col = (line == d.curLine) ? d.curCol : 0;
    const auto loc = d.locate(line);
    const std::string c = code.empty() ? ghost_diag_code(msg) : code;
    if (d.onFatal) {
        auto f = std::move(d.onFatal);
        d.onFatal = nullptr;
        f(line, col, c, msg);
    } else {
        ghost_diag_print(loc.first, loc.second, col, c, msg);
    }
    std::exit(1);
}

// Peças de mensagem montadas com << viram texto
inline std::string ghost_diag_str(const std::string& s) { return s; }
inline std::string ghost_diag_str(const char* s) { return s ? s : ""; }
inline std::string ghost_diag_str(char c) { return std::string(1, c); }
template <class T> inline std::string ghost_diag_str(const T& v) { return std::to_string(v); }

#endif
