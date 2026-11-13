// GhostNumText.hpp - texto de números igual no programa compilado e no REPL.
// Reais com valor inteiro saem sem casas (2.0 -> "2"); os demais com 6 algarismos significativos (2.5, 0.333333).
// Usado por print, f-strings, "texto" + numero e pela impressão de listas/mapas.
#ifndef GHOST_NUM_TEXT_HPP
#define GHOST_NUM_TEXT_HPP

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

inline std::string ghost_num_text(double v) {
    if (std::isfinite(v) && v == std::floor(v) && std::fabs(v) < 1e15) return std::to_string(static_cast<long long>(v));
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

// a ** b entre inteiros (o mesmo código no compilado e no REPL): resultado inteiro, como 7 / 2 == 3.
// Expoente negativo: 1 / a^n em divisão inteira (só 1 e -1 dão algo diferente de 0); 0 ** negativo é erro.
// Estouro dá a volta (aritmética sem sinal), igual nos dois caminhos.
inline long long ghost_ipow(long long a, long long b) {
    if (b < 0) {
        if (a == 0) throw std::runtime_error("0 ** expoente negativo: divisao por zero");
        if (a == 1) return 1;
        if (a == -1) return (b % 2 == 0) ? 1 : -1;
        return 0;
    }
    unsigned long long base = static_cast<unsigned long long>(a), res = 1;
    unsigned long long e = static_cast<unsigned long long>(b);
    while (e > 0) {
        if (e & 1ULL) res *= base;
        base *= base;
        e >>= 1;
    }
    return static_cast<long long>(res);
}

// print(x): reais passam por ghost_num_text; o resto vai direto para o stream
template <class T>
inline const T& ghost_show(const T& v) { return v; }
inline std::string ghost_show(double v) { return ghost_num_text(v); }
inline std::string ghost_show(float v) { return ghost_num_text(v); }
inline std::string ghost_show(long double v) { return ghost_num_text(static_cast<double>(v)); }

#endif
