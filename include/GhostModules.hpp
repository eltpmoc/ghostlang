// GhostModules.hpp - import "arquivo.g" as m;  (c44 do dashboard). Mesmo código no compilador e no REPL.
//
// O módulo importado com "as m" tem suas declarações de nível superior (funções, classes, interfaces,
// circuits e variáveis globais) renomeadas para m__nome dentro do próprio arquivo; quem importa escreve
// m.nome, e o Lexer junta "m.nome" num único nome m__nome (só quando nome é exportado pelo módulo, então
// uma variável comum chamada m continua funcionando: m.keys()). Nas mensagens de erro, m__nome volta a
// aparecer como m.nome.
//
// Nomes de campos e métodos (depois de '.', ou declarados dentro de classes e interfaces) não mudam.
#ifndef GHOST_MODULES_HPP
#define GHOST_MODULES_HPP

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Lexer.hpp"

// alias -> nomes exportados pelo módulo (registrados ao importar)
inline std::map<std::string, std::set<std::string>>& ghost_module_aliases() {
    static std::map<std::string, std::set<std::string>> aliases;
    return aliases;
}

inline std::string ghost_module_join(const std::string& alias, const std::string& name) { return alias + "__" + name; }

// m__nome -> m.nome (mensagens de erro, typeof)
inline std::string ghost_module_pretty(std::string s) {
    for (const auto& a : ghost_module_aliases()) {
        const std::string from = a.first + "__";
        for (size_t p = s.find(from); p != std::string::npos; p = s.find(from, p + 1)) {
            const bool startOk = p == 0 || !(std::isalnum(static_cast<unsigned char>(s[p - 1])) || s[p - 1] == '_');
            if (startOk) s.replace(p, from.size(), a.first + ".");
        }
    }
    return s;
}

namespace ghost_modules_detail {
inline bool isTypeKw(GTokenType t) {
    switch (t) {
        case GTokenType::KwInt: case GTokenType::KwDouble: case GTokenType::KwFloat: case GTokenType::KwString:
        case GTokenType::KwBool: case GTokenType::KwVar: case GTokenType::KwList: case GTokenType::KwMap:
        case GTokenType::KwComplex: case GTokenType::KwGhost: case GTokenType::KwQreg: case GTokenType::KwQcircuit:
        case GTokenType::KwJson:
            return true;
        default:
            return false;
    }
}
} // namespace ghost_modules_detail

// Nomes declarados no nível superior do texto de um arquivo
inline std::set<std::string> ghost_module_exports(const std::string& src) {
    using namespace ghost_modules_detail;
    std::vector<Token> t;
    Lexer lx(src);
    for (Token k = lx.getNextToken(); k.type != GTokenType::EndOfFile; k = lx.getNextToken()) t.push_back(k);
    std::set<std::string> names;
    int depth = 0;
    bool stmtStart = true;
    for (size_t i = 0; i < t.size(); ++i) {
        const auto ty = t[i].type;
        if (ty == GTokenType::LBrace) { depth++; stmtStart = false; continue; }
        if (ty == GTokenType::RBrace) { depth--; stmtStart = depth == 0; continue; }
        if (depth != 0) continue;
        if (ty == GTokenType::Semicolon) { stmtStart = true; continue; }
        if (!stmtStart) continue;
        stmtStart = false;
        if (ty == GTokenType::KwImport) continue;
        if ((ty == GTokenType::KwClass || ty == GTokenType::KwCircuit || (ty == GTokenType::Identifier && t[i].value == "interface")) &&
            i + 1 < t.size() && t[i + 1].type == GTokenType::Identifier) {
            names.insert(t[i + 1].value);
            continue;
        }
        if (ty == GTokenType::KwFunction) { // nome antes do '(' (ou antes de <...>( nas genéricas)
            int angle = 0;
            size_t k = i + 1;
            for (; k < t.size(); ++k) {
                if (t[k].type == GTokenType::Lt) angle++;
                else if (t[k].type == GTokenType::Gt) angle--;
                else if (t[k].type == GTokenType::Chain) angle -= 2;
                else if (t[k].type == GTokenType::LParen && angle <= 0) break;
            }
            if (k >= t.size()) continue;
            size_t n = k - 1;
            if (t[n].type == GTokenType::Gt) { // ... nome < T > (
                int d = 0;
                for (size_t q = n + 1; q-- > i + 1;) {
                    if (t[q].type == GTokenType::Gt) d++;
                    else if (t[q].type == GTokenType::Lt && --d == 0) { n = q - 1; break; }
                }
            }
            if (n > i && t[n].type == GTokenType::Identifier) names.insert(t[n].value);
            continue;
        }
        // variável global: TIPO ... nome (= | ; | '(' de qreg)
        if (isTypeKw(ty) || ty == GTokenType::Identifier) {
            int angle = 0;
            size_t k = i;
            for (; k < t.size(); ++k) {
                const auto tk = t[k].type;
                if (tk == GTokenType::Lt) angle++;
                else if (tk == GTokenType::Gt) angle--;
                else if (tk == GTokenType::Chain) angle -= 2;
                else if (angle <= 0 && (tk == GTokenType::Assign || tk == GTokenType::Semicolon || tk == GTokenType::LParen ||
                                        tk == GTokenType::LBrace || tk == GTokenType::Dot || tk == GTokenType::LBracket))
                    break;
            }
            if (k < t.size() && k >= i + 2 && t[k - 1].type == GTokenType::Identifier &&
                (t[k].type == GTokenType::Assign || t[k].type == GTokenType::Semicolon ||
                 (t[k].type == GTokenType::LParen && (ty == GTokenType::KwQreg || ty == GTokenType::KwQcircuit))))
                names.insert(t[k - 1].value);
        }
    }
    return names;
}

// Renomeia, no texto do módulo, cada uso dos nomes exportados para alias__nome. Mantém as linhas
// (os diagnósticos continuam apontando para a linha certa do arquivo do módulo).
inline std::string ghost_mangle_module(const std::string& src, const std::string& alias, const std::set<std::string>& names) {
    std::vector<Token> t;
    {
        Lexer lx(src);
        for (Token k = lx.getNextToken(); k.type != GTokenType::EndOfFile; k = lx.getNextToken()) t.push_back(k);
    }
    // linhas do texto (para aplicar as trocas por linha/coluna)
    std::vector<std::string> lines;
    {
        std::string cur;
        for (char c : src) { if (c == '\n') { lines.push_back(cur); cur.clear(); } else cur += c; }
        lines.push_back(cur);
    }
    struct Edit { int line, col; size_t len; std::string repl; };
    std::vector<Edit> edits;
    auto renameInText = [&](const std::string& text, int line, int col) { // ${...} de f-strings
        size_t i = 0;
        while ((i = text.find("${", i)) != std::string::npos) {
            size_t j = i + 2;
            int d = 1;
            for (; j < text.size() && d > 0; ++j) { if (text[j] == '{') d++; else if (text[j] == '}') d--; }
            for (size_t k = i + 2; k + 1 < j;) {
                if (std::isalpha(static_cast<unsigned char>(text[k])) || text[k] == '_') {
                    size_t e = k;
                    while (e < j && (std::isalnum(static_cast<unsigned char>(text[e])) || text[e] == '_')) ++e;
                    const std::string w = text.substr(k, e - k);
                    const bool afterDot = k > 0 && text[k - 1] == '.';
                    if (!afterDot && names.count(w)) edits.push_back({line, col + static_cast<int>(k), w.size(), ghost_module_join(alias, w)});
                    k = e;
                } else ++k;
            }
            i = j;
        }
    };
    // 1ª passada: membros (campos e métodos) de cada classe/interface e a base (extends), para que dentro da
    // classe um campo ou método com o mesmo nome de uma declaração do módulo continue sendo o membro (como no C++).
    std::map<std::string, std::set<std::string>> members;
    std::map<std::string, std::string> baseOf;
    {
        std::string pending;
        int d = 0, bodyDepth = -1;
        std::string cur;
        for (size_t i = 0; i < t.size(); ++i) {
            const Token& k = t[i];
            if (k.type == GTokenType::KwClass || (k.type == GTokenType::Identifier && k.value == "interface" && i + 1 < t.size() &&
                                                  t[i + 1].type == GTokenType::Identifier)) {
                pending = i + 1 < t.size() ? t[i + 1].value : "";
                for (size_t q = i + 2; q < t.size() && t[q].type != GTokenType::LBrace; ++q)
                    if (t[q].type == GTokenType::KwExtends && q + 1 < t.size()) baseOf[pending] = t[q + 1].value;
            }
            if (k.type == GTokenType::LBrace) {
                d++;
                if (!pending.empty() && bodyDepth < 0) { bodyDepth = d; cur = pending; pending.clear(); }
                continue;
            }
            if (k.type == GTokenType::RBrace) {
                if (d == bodyDepth) { bodyDepth = -1; cur.clear(); }
                d--;
                continue;
            }
            if (bodyDepth > 0 && d == bodyDepth && k.type == GTokenType::Identifier && k.value != cur) {
                const GTokenType nx = i + 1 < t.size() ? t[i + 1].type : GTokenType::EndOfFile;
                if (nx == GTokenType::LParen || nx == GTokenType::Lt || nx == GTokenType::Semicolon || nx == GTokenType::Assign ||
                    nx == GTokenType::LBracket)
                    members[cur].insert(k.value);
            }
        }
    }
    auto isMember = [&](const std::string& cls, const std::string& name) {
        std::string c = cls;
        for (int guard = 0; !c.empty() && guard < 64; ++guard) {
            auto m = members.find(c);
            if (m != members.end() && m->second.count(name)) return true;
            auto b = baseOf.find(c);
            c = b == baseOf.end() ? std::string() : b->second;
        }
        return false;
    };
    // 2ª passada: renomeia. Dentro de uma classe (corpo e métodos), nomes de membros não mudam.
    struct Body { int depth; std::string cls; };
    std::vector<Body> bodies;
    int depth = 0;
    std::string pendingClass;
    for (size_t i = 0; i < t.size(); ++i) {
        const Token& k = t[i];
        if (k.type == GTokenType::KwClass || (k.type == GTokenType::Identifier && k.value == "interface" && i + 1 < t.size() &&
                                              t[i + 1].type == GTokenType::Identifier))
            pendingClass = (i + 1 < t.size()) ? t[i + 1].value : "";
        if (k.type == GTokenType::LBrace) {
            depth++;
            if (!pendingClass.empty()) { bodies.push_back({depth, pendingClass}); pendingClass.clear(); }
            continue;
        }
        if (k.type == GTokenType::RBrace) {
            if (!bodies.empty() && bodies.back().depth == depth) bodies.pop_back();
            depth--;
            continue;
        }
        if (k.type == GTokenType::FString) {
            // a coluna do token aponta para o 'f'; o texto começa depois de f" (ou f`)
            renameInText(k.value, k.line, k.column + 2);
            continue;
        }
        if (k.type != GTokenType::Identifier || !names.count(k.value)) continue;
        if (i > 0 && t[i - 1].type == GTokenType::Dot) continue;  // obj.nome: membro
        if (!bodies.empty() && k.value != bodies.back().cls && isMember(bodies.back().cls, k.value)) continue;
        edits.push_back({k.line, k.column, k.value.size(), ghost_module_join(alias, k.value)});
    }
    // aplica da direita para a esquerda em cada linha
    std::sort(edits.begin(), edits.end(), [](const Edit& a, const Edit& b) { return a.line != b.line ? a.line < b.line : a.col > b.col; });
    for (const auto& e : edits) {
        if (e.line < 1 || static_cast<size_t>(e.line) > lines.size()) continue;
        std::string& L = lines[static_cast<size_t>(e.line) - 1];
        const size_t c = static_cast<size_t>(e.col - 1);
        if (c + e.len <= L.size()) L.replace(c, e.len, e.repl);
    }
    std::string out;
    for (size_t k = 0; k < lines.size(); ++k) out += lines[k] + (k + 1 < lines.size() ? "\n" : "");
    return out;
}

// Linha "import "arquivo.g" [as m];" ou "load "arquivo.g";" -> (arquivo, alias). false se a linha não for um import.
inline bool ghost_parse_import_line(const std::string& line, std::string& file, std::string& alias) {
    size_t p = line.find_first_not_of(" \t");
    if (p == std::string::npos) return false;
    size_t kwLen = 0;
    if (line.compare(p, 7, "import ") == 0) kwLen = 7;
    else if (line.compare(p, 5, "load ") == 0) kwLen = 5;
    else return false;
    const size_t a = line.find('"', p + kwLen);
    const size_t b = a == std::string::npos ? std::string::npos : line.find('"', a + 1);
    if (b == std::string::npos) return false;
    file = line.substr(a + 1, b - a - 1);
    alias.clear();
    size_t q = line.find_first_not_of(" \t", b + 1);
    if (q != std::string::npos && line.compare(q, 3, "as ") == 0) {
        q = line.find_first_not_of(" \t", q + 3);
        size_t e = q;
        while (e < line.size() && (std::isalnum(static_cast<unsigned char>(line[e])) || line[e] == '_')) ++e;
        if (q != std::string::npos && e > q) alias = line.substr(q, e - q);
    }
    return true;
}

#endif
