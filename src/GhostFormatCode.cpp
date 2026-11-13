#include "../include/GhostFormatCode.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace {

// Remove espaços em branco do início e do fim
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Token simples para formatar uma linha mantendo strings e comentários intactos
struct LineSegment {
    std::string text;
    bool isString;
    bool isComment;
};

std::vector<LineSegment> segmentLine(const std::string& line) {
    std::vector<LineSegment> segs;
    std::string cur;
    size_t i = 0;
    while (i < line.size()) {
        char c = line[i];

        // Comentário de linha //
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            if (!cur.empty()) {
                segs.push_back({cur, false, false});
                cur.clear();
            }
            segs.push_back({line.substr(i), false, true});
            break;
        }

        // Strings normais "..." ou f-strings $"..."
        if (c == '"' || (c == '$' && i + 1 < line.size() && line[i + 1] == '"')) {
            if (!cur.empty()) {
                segs.push_back({cur, false, false});
                cur.clear();
            }
            std::string str;
            if (c == '$') { str += '$'; ++i; }
            str += '"';
            ++i;
            bool escape = false;
            while (i < line.size()) {
                char sc = line[i];
                str += sc;
                if (escape) {
                    escape = false;
                } else if (sc == '\\') {
                    escape = true;
                } else if (sc == '"') {
                    ++i;
                    break;
                }
                ++i;
            }
            segs.push_back({str, true, false});
            continue;
        }

        cur += c;
        ++i;
    }
    if (!cur.empty()) {
        segs.push_back({cur, false, false});
    }
    return segs;
}

// Normaliza operadores e pontuação no código (fora de strings e comentários)
std::string formatCodeSegment(const std::string& code) {
    std::string res;
    size_t i = 0;
    auto isIdent = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

    while (i < code.size()) {
        char c = code[i];

        // Operadores compostos de 2 caracteres
        if (i + 1 < code.size()) {
            std::string op2 = code.substr(i, 2);
            if (op2 == "==" || op2 == "!=" || op2 == "<=" || op2 == ">=" ||
                op2 == "&&" || op2 == "||" || op2 == "->" || op2 == "=>" ||
                op2 == "+=" || op2 == "-=" || op2 == "*=" || op2 == "/=" ||
                op2 == "**") {
                // Adiciona espaço antes se não houver
                if (!res.empty() && res.back() != ' ') res += ' ';
                res += op2;
                res += ' ';
                i += 2;
                while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
                continue;
            }
            // Operadores de incremento e decremento ++ e --
            if (op2 == "++" || op2 == "--") {
                res += op2;
                i += 2;
                continue;
            }
        }

        // Vírgula: sem espaço antes, 1 espaço depois
        if (c == ',') {
            while (!res.empty() && res.back() == ' ') res.pop_back();
            res += ", ";
            ++i;
            while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
            continue;
        }

        // Ponto e vírgula
        if (c == ';') {
            while (!res.empty() && res.back() == ' ') res.pop_back();
            res += ';';
            ++i;
            if (i < code.size() && code[i] != ' ' && code[i] != '\t' && code[i] != '\n') {
                res += ' ';
            }
            continue;
        }

        // Operador = isolado (atribuição)
        if (c == '=' && (i + 1 >= code.size() || code[i + 1] != '=' && code[i + 1] != '>') &&
            (i == 0 || code[i - 1] != '!' && code[i - 1] != '<' && code[i - 1] != '>' && code[i - 1] != '+' && code[i - 1] != '-' && code[i - 1] != '*' && code[i - 1] != '/')) {
            if (!res.empty() && res.back() != ' ') res += ' ';
            res += '=';
            res += ' ';
            ++i;
            while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
            continue;
        }

        // Operadores aritméticos binários (+, -, *, /, %)
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            // Checa se é binário ou unário (unário após '(' ou '[' ou ',' ou no início)
            bool isUnary = false;
            std::string trimmedBack = trim(res);
            if (trimmedBack.empty() || trimmedBack.back() == '(' || trimmedBack.back() == '[' ||
                trimmedBack.back() == ',' || trimmedBack.back() == '=' || trimmedBack.back() == ':') {
                isUnary = true;
            }

            if (isUnary) {
                res += c;
                ++i;
            } else {
                if (!res.empty() && res.back() != ' ') res += ' ';
                res += c;
                res += ' ';
                ++i;
                while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
            }
            continue;
        }

        // Dois pontos : (fora de ::)
        if (c == ':') {
            if (i + 1 < code.size() && code[i + 1] == ':') {
                res += "::";
                i += 2;
                continue;
            }
            while (!res.empty() && res.back() == ' ') res.pop_back();
            res += ": ";
            ++i;
            while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
            continue;
        }

        // Espaços múltiplos viram 1 espaço
        if (c == ' ' || c == '\t') {
            if (!res.empty() && res.back() != ' ') res += ' ';
            ++i;
            while (i < code.size() && (code[i] == ' ' || code[i] == '\t')) ++i;
            continue;
        }

        // Abre chaves { sempre com espaço antes se tiver algo antes que não seja '(' ou '['
        if (c == '{') {
            if (!res.empty() && res.back() != ' ' && res.back() != '(' && res.back() != '[') {
                res += ' ';
            }
            res += '{';
            ++i;
            continue;
        }

        res += c;
        ++i;
    }

    return res;
}

} // namespace

std::string ghost_format_code(const std::string& source) {
    std::stringstream ss(source);
    std::vector<std::string> lines;
    std::string l;
    while (std::getline(ss, l)) {
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines.push_back(l);
    }

    std::vector<std::string> formattedLines;
    int indent = 0;
    int consecutiveEmpty = 0;

    for (size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        std::string raw = trim(lines[lineIdx]);

        if (raw.empty()) {
            ++consecutiveEmpty;
            if (consecutiveEmpty <= 1 && !formattedLines.empty()) {
                formattedLines.push_back("");
            }
            continue;
        }
        consecutiveEmpty = 0;

        // Divide a linha em segmentos (código, strings e comentários)
        std::vector<LineSegment> segs = segmentLine(raw);
        std::string lineContent;
        for (const auto& s : segs) {
            if (s.isString || s.isComment) {
                if (!lineContent.empty() && lineContent.back() != ' ' && s.isComment) {
                    lineContent += "  ";
                }
                lineContent += s.text;
            } else {
                lineContent += formatCodeSegment(s.text);
            }
        }

        // Calcula chaves abertas e fechadas na linha (fora de strings e comentários)
        int openBraces = 0, closeBraces = 0;
        for (const auto& s : segs) {
            if (!s.isString && !s.isComment) {
                for (char c : s.text) {
                    if (c == '{') ++openBraces;
                    if (c == '}') ++closeBraces;
                }
            }
        }

        // Se a linha começa com '}', ela recua antes de imprimir
        bool startsWithClose = (!lineContent.empty() && lineContent[0] == '}');
        bool isCaseOrDefault = (lineContent.rfind("case ", 0) == 0 || lineContent.rfind("default:", 0) == 0);

        int currentLineIndent = indent;
        if (startsWithClose) {
            currentLineIndent = std::max(0, indent - 1);
        } else if (isCaseOrDefault) {
            currentLineIndent = std::max(0, indent - 1);
        }

        std::string pad(currentLineIndent * 4, ' ');
        formattedLines.push_back(pad + trim(lineContent));

        indent += (openBraces - closeBraces);
        if (indent < 0) indent = 0;
    }

    std::string result;
    for (const auto& fl : formattedLines) {
        result += fl + "\n";
    }
    return result;
}
