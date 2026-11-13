// GhostFormat.hpp - formatação nas f-strings: f"${x:.2f}"  f"${nome:>10}"  f"${n:08,d}"  f"${p:.1%}"
// Mesmo código no programa compilado (ghost_fmt, via GhostLib.hpp) e no REPL (Interpreter.cpp),
// então os dois escrevem exatamente o mesmo texto.
//
// Especificação (subconjunto da do Python):  [[preenchimento]alinhamento][sinal][0][largura][,][.precisão][tipo]
//   alinhamento  <  esquerda (padrão de textos)   >  direita (padrão de números)   ^  centro
//   sinal        +  sempre   (espaço) espaço nos positivos   -  só nos negativos (padrão)
//   0            completa com zeros depois do sinal (números)
//   ,            separa milhares com vírgula
//   tipo         f fixo (6 casas)  e notação científica  g geral  d inteiro  x X o b hexa/octal/binário
//                %  porcentagem (x100, 6 casas)  s texto. Sem tipo: o texto normal do valor
//                (com .N vira o geral com N dígitos; em textos, .N corta em N caracteres).
#ifndef GHOST_FORMAT_HPP
#define GHOST_FORMAT_HPP

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "GhostNumText.hpp"

struct GhostFmtSpec {
    char fill = ' ';
    char align = 0;   // '<', '>', '^' ou 0 (padrão do tipo)
    char sign = '-';
    bool zero = false;
    int width = 0;
    bool comma = false;
    int prec = -1;
    char type = 0;
};

inline bool ghost_parse_fmt(const std::string& s, GhostFmtSpec& f) {
    if (s.empty()) return false;
    if (s == "math" || s == "alg" || s == "matematica" || s == "4d" || s == "4D" || s == "vetorial") {
        f.type = 'm';
        return true;
    }
    auto isAlign = [](char c) { return c == '<' || c == '>' || c == '^'; };
    auto badFill = [](char c) { return c == '"' || c == '\\' || c == '{' || c == '}' || c == '\n'; };
    size_t i = 0;
    if (s.size() >= 2 && isAlign(s[1]) && !badFill(s[0])) { f.fill = s[0]; f.align = s[1]; i = 2; }
    else if (isAlign(s[0])) { f.align = s[0]; i = 1; }
    if (i < s.size() && (s[i] == '+' || s[i] == '-' || s[i] == ' ')) f.sign = s[i++];
    if (i < s.size() && s[i] == '0') { f.zero = true; ++i; }
    int w = 0; bool hasW = false;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) { w = w * 10 + (s[i++] - '0'); hasW = true; if (w > 1000) return false; }
    if (hasW) f.width = w;
    if (i < s.size() && s[i] == ',') { f.comma = true; ++i; }
    if (i < s.size() && s[i] == '.') {
        ++i;
        int p = 0; bool hasP = false;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) { p = p * 10 + (s[i++] - '0'); hasP = true; if (p > 100) return false; }
        if (!hasP) return false;
        f.prec = p;
    }
    if (i < s.size()) {
        const std::string types = "fFeEgGdxXobs%";
        if (types.find(s[i]) == std::string::npos) return false;
        f.type = s[i++];
    }
    return i == s.size();
}

// Separa "expr:spec" no último ':' de fora de parênteses/colchetes/chaves/textos que forma uma
// especificação válida e não fecha um ?: ternário. Sem especificação: spec fica vazio.
inline bool ghost_split_fmt(const std::string& inner, std::string& expr, std::string& spec) {
    int depth = 0, questions = 0, colons = 0;
    size_t cut = std::string::npos;
    int qAtCut = 0, cAtCut = 0;
    for (size_t i = 0; i < inner.size(); ++i) {
        const char c = inner[i];
        if (c == '"' || c == '\'' || c == '`') {
            for (++i; i < inner.size() && inner[i] != c; ++i) if (inner[i] == '\\') ++i;
            continue;
        }
        if (c == '(' || c == '[' || c == '{') depth++;
        else if (c == ')' || c == ']' || c == '}') depth--;
        else if (depth == 0 && c == '?') questions++;
        else if (depth == 0 && c == ':') { cut = i; qAtCut = questions; cAtCut = colons; colons++; }
    }
    expr = inner;
    spec.clear();
    if (cut == std::string::npos || qAtCut != cAtCut) return false;
    GhostFmtSpec f;
    const std::string s = inner.substr(cut + 1);
    if (!ghost_parse_fmt(s, f)) return false;
    expr = inner.substr(0, cut);
    spec = s;
    return true;
}

namespace ghost_fmt_detail {
inline size_t u8len(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return n;
}
inline std::string u8prefix(const std::string& s, size_t chars) {
    size_t n = 0, i = 0;
    for (; i < s.size(); ++i) {
        if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) { if (n == chars) break; ++n; }
    }
    return s.substr(0, i);
}
inline std::string pad(const std::string& body, const GhostFmtSpec& f, char defAlign) {
    const size_t len = u8len(body);
    if (f.width <= 0 || len >= static_cast<size_t>(f.width)) return body;
    const size_t total = static_cast<size_t>(f.width) - len;
    const char a = f.align ? f.align : defAlign;
    const std::string fill(1, f.fill);
    auto rep = [&](size_t k) { std::string r; for (size_t j = 0; j < k; ++j) r += fill; return r; };
    if (a == '<') return body + rep(total);
    if (a == '^') return rep(total / 2) + body + rep(total - total / 2);
    return rep(total) + body;
}
inline std::string group(const std::string& digits) { // "1234567.5" -> "1,234,567.5"
    size_t end = digits.find_first_of(".eE");
    if (end == std::string::npos) end = digits.size();
    std::string intPart = digits.substr(0, end), out;
    int k = 0;
    for (size_t i = intPart.size(); i-- > 0;) {
        out.insert(out.begin(), intPart[i]);
        if (++k % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    return out + digits.substr(end);
}
inline std::string radix(unsigned long long v, int base, bool upper) {
    if (v == 0) return "0";
    const char* d = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    std::string s;
    while (v) { s.insert(s.begin(), d[v % static_cast<unsigned>(base)]); v /= static_cast<unsigned>(base); }
    return s;
}
} // namespace ghost_fmt_detail

inline std::string ghost_fmt_text(const std::string& text, const std::string& specText) {
    GhostFmtSpec f;
    if (!ghost_parse_fmt(specText, f)) throw std::runtime_error("formato invalido ':" + specText + "'");
    if (f.type && f.type != 's') throw std::runtime_error("formato ':" + specText + "' precisa de numero, recebido texto \"" + text + "\"");
    const std::string body = f.prec >= 0 ? ghost_fmt_detail::u8prefix(text, static_cast<size_t>(f.prec)) : text;
    return ghost_fmt_detail::pad(body, f, '<');
}

inline std::string ghost_fmt_num(double v, bool isInt, const std::string& specText) {
    GhostFmtSpec f;
    if (!ghost_parse_fmt(specText, f)) throw std::runtime_error("formato invalido ':" + specText + "'");
    if (f.type == 's') {
        GhostFmtSpec t = f;
        t.type = 0;
        const std::string txt = isInt ? std::to_string(static_cast<long long>(v)) : ghost_num_text(v);
        return ghost_fmt_detail::pad(f.prec >= 0 ? ghost_fmt_detail::u8prefix(txt, static_cast<size_t>(f.prec)) : txt, t, '<');
    }
    std::string digits;
    bool neg = std::signbit(v) && !(v == 0.0 && (f.type == 'd' || f.type == 'x' || f.type == 'X' || f.type == 'o' || f.type == 'b'));
    const double a = std::fabs(v);
    std::ostringstream os;
    if (std::isnan(v)) { digits = "nan"; neg = false; }
    else if (std::isinf(v)) digits = "inf";
    else {
        switch (f.type) {
            case 'f': case 'F': os << std::fixed << std::setprecision(f.prec >= 0 ? f.prec : 6) << a; digits = os.str(); break;
            case 'e': case 'E':
                os << std::scientific << std::setprecision(f.prec >= 0 ? f.prec : 6) << a; digits = os.str();
                if (f.type == 'E') for (auto& c : digits) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                break;
            case 'g': case 'G':
                os << std::setprecision(f.prec > 0 ? f.prec : (f.prec == 0 ? 1 : 6)) << a; digits = os.str();
                if (f.type == 'G') for (auto& c : digits) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                break;
            case '%': os << std::fixed << std::setprecision(f.prec >= 0 ? f.prec : 6) << a * 100.0; digits = os.str() + "%"; break;
            case 'd': { const long long r = std::llround(v); neg = r < 0; digits = std::to_string(r < 0 ? -r : r); break; }
            case 'x': case 'X': case 'o': case 'b': {
                const long long r = std::llround(v);
                neg = r < 0;
                const unsigned long long u = static_cast<unsigned long long>(r < 0 ? -r : r);
                digits = ghost_fmt_detail::radix(u, f.type == 'o' ? 8 : f.type == 'b' ? 2 : 16, f.type == 'X');
                break;
            }
            default: // sem tipo: texto normal; com .N, geral com N dígitos
                if (f.prec >= 0) { os << std::setprecision(f.prec > 0 ? f.prec : 1) << a; digits = os.str(); }
                else digits = isInt ? std::to_string(static_cast<long long>(std::llround(a))) : ghost_num_text(a);
                break;
        }
    }
    if (f.comma && digits.find_first_not_of("0123456789.eE+-%") == std::string::npos) digits = ghost_fmt_detail::group(digits);
    const std::string sign = neg ? "-" : f.sign == '+' ? "+" : f.sign == ' ' ? " " : "";
    if (f.zero && !f.align && f.width > 0) { // zeros entre o sinal e os dígitos
        while (sign.size() + digits.size() < static_cast<size_t>(f.width)) digits.insert(digits.begin(), '0');
        return sign + digits;
    }
    return ghost_fmt_detail::pad(sign + digits, f, '>');
}

#endif
