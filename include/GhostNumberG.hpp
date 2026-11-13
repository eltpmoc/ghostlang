#pragma once
#ifndef GHOST_NUMBER_G_HPP
#define GHOST_NUMBER_G_HPP

class GhostNumberG;

#include <vector>
#include <tuple>
#include <cstdint>
#include <complex>
#include <string>
#include <functional>
#include <ostream>
#include <iostream>

// GhostNumberG.hpp - Declaração da classe GhostNumberG
/*
Descreva o propósito desta classe
A classe GhostNumberG representa um número fantasma, que é uma extensão dos números complexos
com uma parte real, uma parte imaginária e um parâmetro de incerteza θ².
A classe permite operações matemáticas como adição, subtração, multiplicação e divisão de números fantasmas.
A classe também permite a criação de números fantasmas a partir de uma semente e um valor inteiro.
A classe GhostNumberG é usada para representar números fantasmas em operações matemáticas
e para manipular esses números de forma eficiente.
A classe também fornece métodos para calcular a norma, conjugado e componentes do número fantasma.

A classe GhostNumberG é uma extensão dos números complexos, permitindo operações matemáticas
com números que possuem uma parte real, uma parte imaginária e um parâmetro de incerteza θ².

A classe é projetada para ser usada em operações matemáticas envolvendo números fantasmas,
como adição, subtração, multiplicação e divisão.

*/
class GhostNumberG {
private:
    uint8_t rawByte;

public:
    double b, x, y, p, theta2, nilpotent, value, theta;
    std::vector<uint8_t> entropy;

    GhostNumberG();

    // BLINDAGEM: explicit impede bool -> int -> GhostNumberG
    explicit GhostNumberG(uint8_t v);
    explicit GhostNumberG(double x, bool is_scalar);
    explicit GhostNumberG(double value);
    explicit GhostNumberG(int v);

    GhostNumberG(double x, double y, double p, double theta2);
    GhostNumberG(double x, double y, double p);
    GhostNumberG(const GhostNumberG& other);
    // Permite: ghost G = 1.5 + 2.0i;  (complexo -> fantasma com p = 0)
    GhostNumberG(const std::complex<double>& c) : GhostNumberG(c.real(), c.imag(), 0.0) {}

    // É Zero se x, y, p são todos zero
    bool isZero() const;

    uint8_t toByte() const;
    int toInt() const;
    void rotate();
    void entropize();

    static GhostNumberG from_bytes(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> to_bytes() const;

    static GhostNumberG from_seed(int val, const std::string& seed, const std::vector<uint8_t>& context);

    double norm_squared() const;
    GhostNumberG norm() const;
    GhostNumberG normSquared() const;
    GhostNumberG abs();
    GhostNumberG abs2();
    GhostNumberG arg(const GhostNumberG& g);
    GhostNumberG sqrt(const GhostNumberG& g);
    GhostNumberG log(const GhostNumberG& g);
    GhostNumberG exp(const GhostNumberG& g);
    GhostNumberG pow(double exponent);

    
    GhostNumberG relu() const;
    GhostNumberG sigmoid() const;
    GhostNumberG sin() const;
    GhostNumberG cos() const;
    GhostNumberG tan() const;
    GhostNumberG tanh() const;
    GhostNumberG leaky_relu(double alpha = 0.01) const;
    GhostNumberG gelu() const;

    bool isValid() const;
    GhostNumberG sanitize() const;
    GhostNumberG conjugate() const;
    bool isInvertible() const;
    GhostNumberG inverse() const;

    GhostNumberG set_theta2(double new_theta2) const;
    GhostNumberG set_real(double new_x) const;
    GhostNumberG set_imag(double new_y) const;
    GhostNumberG set_ghost(double new_p) const;
    GhostNumberG set_components(double new_x, double new_y, double new_p) const;

    double get_real() const;
    double get_imag() const;
    double get_ghost() const;
    double get_theta2() const;
    double magnitude() const;
    
    operator unsigned char() const;

    std::tuple<double, double, double> get_components() const;
    std::tuple<double, double, double, double> get_all_components() const;
    std::complex<double> ghostNumberTo_complex() const;

    GhostNumberG operator+(const GhostNumberG& other) const;
    GhostNumberG operator-(const GhostNumberG& other) const;
    GhostNumberG operator*(const GhostNumberG& other) const;
    GhostNumberG operator/(const GhostNumberG& other) const;
    GhostNumberG& operator+=(const GhostNumberG& other);
    GhostNumberG& operator-=(const GhostNumberG& other);
    GhostNumberG& operator*=(const GhostNumberG& other);
    GhostNumberG& operator/=(const GhostNumberG& other);

    GhostNumberG operator*(double scalar) const;
    GhostNumberG operator/(double scalar) const;
    GhostNumberG operator+(double scalar) const;
    GhostNumberG operator-(double scalar) const;

    GhostNumberG operator-();
    GhostNumberG operator+();
    GhostNumberG operator-() const;

    GhostNumberG operator++();
    GhostNumberG operator++(int);
    GhostNumberG operator--();
    GhostNumberG operator--(int);

    bool operator==(const GhostNumberG& other) const;
    bool operator!=(const GhostNumberG& other) const;
    bool operator>(const GhostNumberG& other) const;
    bool operator<(const GhostNumberG& other) const;
    bool operator>=(const GhostNumberG& other) const;
    bool operator<=(const GhostNumberG& other) const;
    GhostNumberG operator^(uint8_t rhs) const;
    GhostNumberG operator^(const GhostNumberG& rhs) const;
    friend GhostNumberG operator^(uint8_t lhs, const GhostNumberG& rhs);

    double transform_theta(double theta) const;
    GhostNumberG transform_theta(const GhostNumberG& g, std::function<double(double)> transformer);

    int get_int() const;
    double get_double() const;
    int get_gnumber() const;
    std::string to_string() const;
    std::string to_string(const std::string& fmt) const;
    std::string to_4d(bool with_prefix = false) const;
    std::string to_math(bool with_prefix = false) const;
    std::string to_algebraic(bool with_prefix = false) const { return to_math(with_prefix); }
    std::string to_string_4d(bool with_prefix = false) const { return to_4d(with_prefix); }
    std::string to_string_math(bool with_prefix = false) const { return to_math(with_prefix); }
    void print() const;
    void print_4d() const;
    void print_math() const;
    // show(G) na GhostLang gera G.display(), igual a gate/state
    void display() const { std::cout << to_string() << std::endl; }
    double get_double_real() const;
    //operator uint8_t() const;
    operator double() const;
    operator int() const;
    operator std::string() const;
    operator std::complex<double>() const;
    operator bool() const;


    static GhostNumberG zero(double theta2 = 1.0);
    static GhostNumberG one(double theta2 = 1.0);
    static GhostNumberG i();
    static GhostNumberG pi();
    static GhostNumberG e();
    static GhostNumberG phi();

    std::vector<std::string> get_vector_string();
    std::vector<double> get_vector_double();
    std::vector<int> get_vector_int();
    std::vector<GhostNumberG> get_vector_gnumber();
    std::vector<std::complex<double>> get_vector_complex();

    GhostNumberG& operator=(const GhostNumberG& other);
    GhostNumberG& operator=(int v);
    GhostNumberG& operator=(double v);
    GhostNumberG harmonize_theta(const GhostNumberG& other);

    // Retorna a norma "real" (só x e y) para cálculo de probabilidade
    double real_norm_squared() const;

};

inline GhostNumberG number_g(double real, double imag, double theta) {
    return GhostNumberG(real, imag, theta);
}

// Parser universal para formatos 4D (ex.: "(3.0, 4.0, 1.7, 2.0)") e matematico (ex.: "3.0 + 4.0i - 1.7p + 2.0θ²")
GhostNumberG parse_ghost_number(const std::string& str);

inline std::string to_4d(const GhostNumberG& g, bool with_prefix = false) { return g.to_4d(with_prefix); }
inline std::string to_math(const GhostNumberG& g, bool with_prefix = false) { return g.to_math(with_prefix); }
inline std::string to_algebraic(const GhostNumberG& g, bool with_prefix = false) { return g.to_math(with_prefix); }

inline std::istream& operator>>(std::istream& is, GhostNumberG& g) {
    std::string s;
    if (std::getline(is, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        g = parse_ghost_number(s);
    }
    return is;
}

// ---------------------------------------------------------------------------
// Literais da GhostLang gerados pelo compilador:
//   2.0i -> ghost_i(2.0)  (std::complex, para servir tanto a complex quanto a ghost)
//   3.5g -> ghost_g(3.5)  (componente fantasma p)
// ---------------------------------------------------------------------------
inline std::complex<double> ghost_i(double y) { return std::complex<double>(0.0, y); }
inline GhostNumberG ghost_g(double p) { return GhostNumberG(0.0, 0.0, p); }

// Mistura complexo/escalar com GhostNumberG. Sem estas sobrecargas exatas,
// as conversoes implicitas da classe (double, int, bool, ...) tornam a expressao
// ambigua. O resultado segue a mesma regra dos operadores membros.
inline GhostNumberG operator+(const std::complex<double>& a, const GhostNumberG& b) { return GhostNumberG(a) + b; }
inline GhostNumberG operator-(const std::complex<double>& a, const GhostNumberG& b) { return GhostNumberG(a) - b; }
inline GhostNumberG operator*(const std::complex<double>& a, const GhostNumberG& b) { return GhostNumberG(a) * b; }
inline GhostNumberG operator+(const GhostNumberG& a, const std::complex<double>& b) { return a + GhostNumberG(b); }
inline GhostNumberG operator-(const GhostNumberG& a, const std::complex<double>& b) { return a - GhostNumberG(b); }
inline GhostNumberG operator*(const GhostNumberG& a, const std::complex<double>& b) { return a * GhostNumberG(b); }
inline GhostNumberG operator+(double a, const GhostNumberG& b) { return GhostNumberG(a, 0.0, 0.0) + b; }
inline GhostNumberG operator-(double a, const GhostNumberG& b) { return GhostNumberG(a, 0.0, 0.0) - b; }
inline GhostNumberG operator*(double a, const GhostNumberG& b) { return GhostNumberG(a, 0.0, 0.0) * b; }
inline GhostNumberG operator+(int a, const GhostNumberG& b) { return static_cast<double>(a) + b; }
inline GhostNumberG operator-(int a, const GhostNumberG& b) { return static_cast<double>(a) - b; }
inline GhostNumberG operator*(int a, const GhostNumberG& b) { return static_cast<double>(a) * b; }

// print(G) / std::cout << G: correspondencia exata, evita a ambiguidade entre
// as conversoes para double, int, bool, string e complex.
inline std::ostream& operator<<(std::ostream& os, const GhostNumberG& g) { return os << g.to_string(); }

#endif // GHOST_NUMBER_G_HPP
