
#include "GhostNumberG.hpp"
#include <iostream>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <numeric>
#include <cstdint>
#include <sstream>
#include <tuple>
#include <iomanip>
#include <algorithm>
#include <complex>
#include <stdexcept>
#include <functional>

// Limits para acessar infinity
#include <limits>

GhostNumberG::GhostNumberG() 
    : rawByte(0), b(0), x(0), y(0), p(0), theta2(0.0), nilpotent(0), value(0), theta(0) {}

GhostNumberG::GhostNumberG(uint8_t v)
    : rawByte(v), b(0), x(0), y(0), p(0), theta2(0.0), nilpotent(0), value(static_cast<double>(v)), theta(0) {}

GhostNumberG::GhostNumberG(double x, bool is_scalar)
    : rawByte(0), b(0), x(x), y(0), p(0), theta2(0.0), nilpotent(0), value(x), theta(0) {}

GhostNumberG::GhostNumberG(double value)
    : rawByte(0), b(0), x(value), y(0), p(0), theta2(0.0), nilpotent(0), value(value), theta(0) {}

GhostNumberG::GhostNumberG(int v)
    : rawByte(static_cast<uint8_t>(v)), b(0), x(0), y(0), p(0), theta2(0.0), nilpotent(0), value(static_cast<double>(v)), theta(0) {}

GhostNumberG::GhostNumberG(double x, double y, double p, double theta2)
    : rawByte(0), b(0), x(x), y(y), p(p), theta2(theta2), nilpotent(0), value(0), theta(0) {}

GhostNumberG::GhostNumberG(double x, double y, double p) 
    : rawByte(0), b(0), x(x), y(y), p(p), theta2(0.0), nilpotent(0), value(0), theta(0) {}

GhostNumberG::GhostNumberG(const GhostNumberG& other) = default;

int GhostNumberG::toInt() const {
    return static_cast<int>(std::round(x));
}

// Verifica se o GhostNumber é zero
bool GhostNumberG::isZero() const {
    // Considera zero se todos os componentes principais forem próximos de zero.
    constexpr double EPS = 1e-9;
    return std::abs(x) < EPS && std::abs(y) < EPS && std::abs(p) < EPS;
}

uint8_t GhostNumberG::toByte() const {
    double val = std::round(this->x);
    return static_cast<uint8_t>(val < 0 ? 0 : (val > 255 ? 255 : val));
}

GhostNumberG GhostNumberG::operator^(uint8_t rhs) const {
    return GhostNumberG(static_cast<uint8_t>(this->toByte() ^ rhs));
}

GhostNumberG GhostNumberG::operator^(const GhostNumberG& rhs) const {
    return GhostNumberG(static_cast<uint8_t>(this->toByte() ^ rhs.toByte()));
}

GhostNumberG operator^(uint8_t lhs, const GhostNumberG& rhs) {
    return GhostNumberG(static_cast<uint8_t>(lhs ^ rhs.toByte()));
}

std::tuple<double, double, double> GhostNumberG::get_components() const {
    return std::make_tuple(x, y, p);
}

std::tuple<double, double, double, double> GhostNumberG::get_all_components() const {
    return std::make_tuple(x, y, p, theta2);
}

GhostNumberG GhostNumberG::set_theta2(double new_theta2) const {
    return GhostNumberG(x, y, p, new_theta2);
}

GhostNumberG GhostNumberG::set_real(double new_x) const {
    return GhostNumberG(new_x, y, p, theta2);
}

GhostNumberG GhostNumberG::set_imag(double new_y) const {
    return GhostNumberG(x, new_y, p, theta2);
}

GhostNumberG GhostNumberG::set_ghost(double new_p) const {
    return GhostNumberG(x, y, new_p, theta2);
}

GhostNumberG GhostNumberG::conjugate() const {
    // A conjugação em si não precisa transformar theta, apenas a parte 'imaginária' y
    return GhostNumberG(x, -y, p, theta2);
}

GhostNumberG GhostNumberG::normSquared() const {
    return GhostNumberG(norm_squared(), 0, 0, theta2);
}

GhostNumberG GhostNumberG::norm() const {
    return GhostNumberG(std::sqrt(norm_squared()), 0, 0, theta2);
}

double GhostNumberG::norm_squared() const {
    // A norma deve considerar a parte fantasma p e o fator theta2
    return x * x + y * y + (p * p * theta2);
}

// Retorna a norma "real" (só x e y) para cálculo de probabilidade
double GhostNumberG::real_norm_squared() const {
    return x * x + y * y;
}

bool GhostNumberG::isValid() const {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(p) || !std::isfinite(theta2)) return false;
    // theta2 agora é validamente 0.0 na álgebra nilpotente
    return true;
}

GhostNumberG GhostNumberG::abs2(){
    return GhostNumberG(x * x + y * y + (p * p * theta2), 0, 0, theta2);
}

GhostNumberG GhostNumberG::arg(const GhostNumberG& g){
    return GhostNumberG(std::arg(std::complex<double>(g.x, g.y)), 0, 0, g.theta2);
}

GhostNumberG GhostNumberG::sanitize() const {
    double safeTheta = !std::isfinite(theta2) ? 0.0 : theta2;
    return GhostNumberG(x, y, p, safeTheta);
}

double GhostNumberG::transform_theta(double theta) const {
    if (theta == 0.0) return 0.0;
    return 1.0 / (theta * theta);
}

GhostNumberG GhostNumberG::transform_theta(const GhostNumberG& g, std::function<double(double)> transformer) {
    double new_theta2 = transformer(g.get_theta2());
    if (!std::isfinite(new_theta2) || new_theta2 <= 0.0)
        new_theta2 = 1.0;
    return g.set_theta2(new_theta2);
}

// A função foi atualizada para o novo formato de exibição
std::string GhostNumberG::to_string() const {
    char buffer[256];
    // Formatação estrita conforme exigido pelo interpretador Phantom
    // Evita -0.0000 usando verificação de proximidade com zero
    std::sprintf(buffer, "(x, yi, p, θ²) = (%.4f, %.4f, %.4f, %.4f)",
                  (std::abs(x) < 1e-9 ? 0.0 : x),
                  (std::abs(y) < 1e-9 ? 0.0 : y),
                  (std::abs(p) < 1e-9 ? 0.0 : p),
                  (std::abs(theta2) < 1e-9 ? 0.0 : theta2));
    return std::string(buffer);
}

static std::string format_ghost_component(double val) {
    if (std::abs(val) < 1e-9) return "0.0";
    char buf[64];
    if (std::abs(val - std::round(val)) < 1e-9 && std::abs(val) < 1e12) {
        std::snprintf(buf, sizeof(buf), "%.1f", val);
        return std::string(buf);
    }
    std::snprintf(buf, sizeof(buf), "%.6g", val);
    std::string s(buf);
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
        s += ".0";
    }
    return s;
}

std::string GhostNumberG::to_4d(bool with_prefix) const {
    std::string res = "(" + format_ghost_component(x) + ", " +
                            format_ghost_component(y) + ", " +
                            format_ghost_component(p) + ", " +
                            format_ghost_component(theta2) + ")";
    if (with_prefix) return "G = " + res;
    return res;
}

std::string GhostNumberG::to_math(bool with_prefix) const {
    std::string res;
    // Termo real: x
    if (x < -1e-9) {
        res += "-" + format_ghost_component(-x);
    } else {
        res += format_ghost_component(x);
    }

    // Termo imaginário: y*i
    if (y < -1e-9) {
        res += " - " + format_ghost_component(-y) + "i";
    } else {
        res += " + " + format_ghost_component(y) + "i";
    }

    // Termo fantasma: p*p (notação fantasma)
    if (p < -1e-9) {
        res += " - " + format_ghost_component(-p) + "p";
    } else {
        res += " + " + format_ghost_component(p) + "p";
    }

    // Termo nilpotente: theta2*θ²
    if (theta2 < -1e-9) {
        res += " - " + format_ghost_component(-theta2) + "θ²";
    } else {
        res += " + " + format_ghost_component(theta2) + "θ²";
    }

    if (with_prefix) return "G = " + res;
    return res;
}

std::string GhostNumberG::to_string(const std::string& fmt) const {
    if (fmt == "4d" || fmt == "4D" || fmt == "vetorial") return to_4d(false);
    if (fmt == "G=4d" || fmt == "G=4D") return to_4d(true);
    if (fmt == "math" || fmt == "alg" || fmt == "matematica") return to_math(false);
    if (fmt == "G=math" || fmt == "G=alg") return to_math(true);
    return to_string();
}

void GhostNumberG::print() const {
    std::printf("(x, yi, p, θ²) = (%.4f, %.4f, %.4f, %.4f)\n", 
                (std::abs(x) < 1e-9 ? 0.0 : x),
                (std::abs(y) < 1e-9 ? 0.0 : y),
                (std::abs(p) < 1e-9 ? 0.0 : p),
                (std::abs(theta2) < 1e-9 ? 0.0 : theta2));
}

void GhostNumberG::print_4d() const {
    std::cout << to_4d(true) << std::endl;
}

void GhostNumberG::print_math() const {
    std::cout << to_math(true) << std::endl;
}

GhostNumberG parse_ghost_number(const std::string& input_str) {
    std::string s = input_str;
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s = s.substr(3);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    if (s.empty()) return GhostNumberG();

    if (s.rfind("(x, yi, p, θ²) =", 0) == 0 || s.rfind("(x, yi, p, theta2) =", 0) == 0) {
        size_t eq = s.find('=');
        s = s.substr(eq + 1);
    } else if ((s.size() >= 2 && (s[0] == 'G' || s[0] == 'g') && (s[1] == '=' || (s.size() >= 3 && s[1] == ' ' && s[2] == '=')))) {
        size_t eq = s.find('=');
        s = s.substr(eq + 1);
    }
    while (!s.empty() && (std::isspace(static_cast<unsigned char>(s.front())) || s.front() == '(' || s.front() == '[')) s.erase(s.begin());
    while (!s.empty() && (std::isspace(static_cast<unsigned char>(s.back())) || s.back() == ')' || s.back() == ']' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    if (s.empty()) return GhostNumberG();

    if (s.find(',') != std::string::npos) {
        std::vector<std::string> parts;
        std::stringstream ss(s);
        std::string part;
        while (std::getline(ss, part, ',')) {
            while (!part.empty() && (std::isspace(static_cast<unsigned char>(part.front())) || part.front() == '(' || part.front() == '[')) part.erase(part.begin());
            while (!part.empty() && (std::isspace(static_cast<unsigned char>(part.back())) || part.back() == ')' || part.back() == ']' || part.back() == '\r' || part.back() == '\n')) part.pop_back();
            if (!part.empty()) parts.push_back(part);
        }
        auto clean_num = [](std::string p, const std::vector<std::string>& suffixes) {
            for (const auto& suf : suffixes) {
                if (p.size() >= suf.size() && p.compare(p.size() - suf.size(), suf.size(), suf) == 0) {
                    p = p.substr(0, p.size() - suf.size());
                    break;
                }
            }
            while (!p.empty() && (std::isspace(static_cast<unsigned char>(p.front())) || p.front() == '(' || p.front() == '[')) p.erase(p.begin());
            while (!p.empty() && (std::isspace(static_cast<unsigned char>(p.back())) || p.back() == ')' || p.back() == ']' || p.back() == '\r' || p.back() == '\n')) p.pop_back();
            try { return std::stod(p); } catch (...) { return 0.0; }
        };
        double rx = parts.size() > 0 ? clean_num(parts[0], {"x"}) : 0.0;
        double ry = parts.size() > 1 ? clean_num(parts[1], {"yi", "i", "y"}) : 0.0;
        double rp = parts.size() > 2 ? clean_num(parts[2], {"p", "g"}) : 0.0;
        double rth = parts.size() > 3 ? clean_num(parts[3], {"θ²", "theta2", "theta^2", "θ^2", "θ", "theta"}) : 0.0;
        return GhostNumberG(rx, ry, rp, rth);
    }

    double rx = 0.0, ry = 0.0, rp = 0.0, rth = 0.0;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (i >= s.size()) break;
        double sign = 1.0;
        if (s[i] == '+') { sign = 1.0; ++i; }
        else if (s[i] == '-') { sign = -1.0; ++i; }
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (i >= s.size()) break;

        size_t start = i;
        while (i < s.size()) {
            if (s[i] == '+' || s[i] == '-') {
                if (i > start && (s[i-1] == 'e' || s[i-1] == 'E')) {
                    ++i;
                    continue;
                }
                break;
            }
            ++i;
        }
        std::string term = s.substr(start, i - start);
        while (!term.empty() && std::isspace(static_cast<unsigned char>(term.front()))) term.erase(term.begin());
        while (!term.empty() && std::isspace(static_cast<unsigned char>(term.back()))) term.pop_back();
        if (term.empty()) continue;

        int comp = 0;
        std::string num_str;
        std::string u8_theta2 = "\xCE\xB8\xC2\xB2";
        if (term.find(u8_theta2) != std::string::npos) {
            comp = 3;
            size_t pos = term.find(u8_theta2);
            num_str = term.substr(0, pos) + term.substr(pos + u8_theta2.size());
        } else if (term.find("theta2") != std::string::npos) {
            comp = 3;
            size_t pos = term.find("theta2");
            num_str = term.substr(0, pos) + term.substr(pos + 6);
        } else if (term.find("theta^2") != std::string::npos) {
            comp = 3;
            size_t pos = term.find("theta^2");
            num_str = term.substr(0, pos) + term.substr(pos + 7);
        } else if (term.find("θ^2") != std::string::npos) {
            comp = 3;
            size_t pos = term.find("θ^2");
            num_str = term.substr(0, pos) + term.substr(pos + 4);
        } else if (term.find("theta") != std::string::npos) {
            comp = 2;
            size_t pos = term.find("theta");
            num_str = term.substr(0, pos) + term.substr(pos + 5);
        } else if (term.find("\xCE\xB8") != std::string::npos) {
            comp = 2;
            size_t pos = term.find("\xCE\xB8");
            num_str = term.substr(0, pos) + term.substr(pos + 2);
        } else if (term.find('i') != std::string::npos || term.find('j') != std::string::npos) {
            comp = 1;
            for (char c : term) if (c != 'i' && c != 'j') num_str += c;
        } else if (term.find('g') != std::string::npos) {
            comp = 2;
            for (char c : term) if (c != 'g') num_str += c;
        } else if (term.find('p') != std::string::npos) {
            comp = 2;
            for (char c : term) if (c != 'p') num_str += c;
        } else {
            comp = 0;
            num_str = term;
        }

        while (!num_str.empty() && (num_str.back() == '*' || std::isspace(static_cast<unsigned char>(num_str.back())))) num_str.pop_back();
        while (!num_str.empty() && (num_str.front() == '*' || std::isspace(static_cast<unsigned char>(num_str.front())))) num_str.erase(num_str.begin());

        double coeff = 1.0;
        if (num_str.empty()) {
            coeff = 1.0;
        } else {
            try { coeff = std::stod(num_str); } catch (...) { coeff = 0.0; }
        }
        coeff *= sign;

        if (comp == 0) rx += coeff;
        else if (comp == 1) ry += coeff;
        else if (comp == 2) rp += coeff;
        else if (comp == 3) rth += coeff;
    }

    return GhostNumberG(rx, ry, rp, rth);
}

// OPERADOR DE IGUALDADE
bool GhostNumberG::operator==(const GhostNumberG& other) const {
    return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9 && std::abs(p - other.p) < 1e-9;
}

bool GhostNumberG::operator!=(const GhostNumberG& other) const {
    return !(*this == other);
}

bool GhostNumberG::operator>(const GhostNumberG& other) const {
    return this->get_real() > other.get_real();
}

bool GhostNumberG::operator<(const GhostNumberG& other) const {
    return this->get_real() < other.get_real();
}

bool GhostNumberG::operator>=(const GhostNumberG& other) const {
    return this->get_real() >= other.get_real();
}

bool GhostNumberG::operator<=(const GhostNumberG& other) const {
    return this->get_real() <= other.get_real();
}

// OPERADOR DE MULTIPLICAÇÃO
GhostNumberG GhostNumberG::operator*(const GhostNumberG& other) const {
    // g = (x1 + iy1) + p1θ
    // h = (x2 + iy2) + p2θ
    // gh = (x1x2 - y1y2) + i(x1y2 + y1x2) + (x1p2 + x2p1)θ
    
    double new_x = this->x * other.x - this->y * other.y;
    double new_y = this->x * other.y + this->y * other.x;

    // Interação determinístico-fantasma (Nilpotente)
    double new_p = (this->x * other.p) + (other.x * this->p);

    return GhostNumberG(new_x, new_y, new_p, 0.0000);
}

/*GhostNumberG GhostNumberG::operator/(const GhostNumberG& other) const {
    double denom = other.norm_squared();
    if (std::abs(denom) < 1e-12) { // Evita divisão por zero
        // Retornar um valor que indica erro ou um estado inválido
        return GhostNumberG(NAN, NAN, NAN, this->theta2);
    }
    // Divisão é a multiplicação pelo inverso: (this * other.conjugate()) / other.norm_squared()
    GhostNumberG conj = other.conjugate();
    GhostNumberG num = (*this) * conj;
    
    double inv_denom = 1.0 / denom;
    
    return GhostNumberG(num.x * inv_denom, num.y * inv_denom, num.p * inv_denom, this->theta2);
}
*/

// --- CORREÇÃO NA DIVISÃO (Conservação Complexa) ---
GhostNumberG GhostNumberG::operator/(const GhostNumberG& other) const {
    // Cenário A (Singularidade / Fantasma Puro): Divisor com base clássica nula
    if (std::abs(other.x) < 1e-15 && std::abs(other.y) < 1e-15) {
        // Calcula a magnitude total no plano complexo (Norma)
        double complex_mag = std::sqrt(this->x * this->x + this->y * this->y);
        
        // Se houver energia real/imaginária, transfere para p. Senão, preserva o p atual.
        double final_magnitude = (complex_mag > 1e-15) ? complex_mag : this->p;
        
        return GhostNumberG(0.0000, 0.0000, final_magnitude, 0.0000);
    }

    // Cenário B (Divisão Regular): Usa g1 * g2^-1 baseado em Taylor/Nilpotente
    return (*this) * other.inverse();
}

GhostNumberG GhostNumberG::operator-(const GhostNumberG& other) const {
    return GhostNumberG(this->x - other.x, this->y - other.y, this->p - other.p, this->theta2 - other.theta2);
}

// OPERADOR DE ADIÇÃO
GhostNumberG GhostNumberG::operator+(const GhostNumberG& other) const {
    // Superposição linear pura conforme Phantom Number Theory G
    return GhostNumberG(this->x + other.x, this->y + other.y, this->p + other.p, this->theta2 + other.theta2);
}

GhostNumberG& GhostNumberG::operator+=(const GhostNumberG& other) {
    *this = *this + other;
    return *this;
}

GhostNumberG& GhostNumberG::operator-=(const GhostNumberG& other) {
    *this = *this - other;
    return *this;
}

GhostNumberG& GhostNumberG::operator*=(const GhostNumberG& other) {
    *this = *this * other;
    return *this;
}

GhostNumberG& GhostNumberG::operator/=(const GhostNumberG& other) {
    *this = *this / other;
    return *this;
}


void GhostNumberG::rotate() {
    double temp = x;
    x = y;
    y = p;
    p = temp;
}

void GhostNumberG::entropize() {
    double e = std::accumulate(entropy.begin(), entropy.end(), 0.0);
    x += fmod(e, 7.0);
    y += fmod(e, 5.0);
    p += fmod(e, 3.0);
}

GhostNumberG GhostNumberG::from_bytes(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 4) throw std::invalid_argument("bytes insuficientes para criar GhostNumberG");
    return GhostNumberG(bytes[0], bytes[1], bytes[2], bytes[3] / 255.0);
}

std::vector<uint8_t> GhostNumberG::to_bytes() const {
    std::vector<uint8_t> bytes;
    auto rx = static_cast<int>(std::round(x));
    auto ry = static_cast<int>(std::round(y));
    auto rp = static_cast<int>(std::round(p));
    auto rt = static_cast<int>(std::round(theta2 * 255.0));

    bytes.push_back(static_cast<uint8_t>(rx < 0 ? 0 : (rx > 255 ? 255 : rx)));
    bytes.push_back(static_cast<uint8_t>(ry < 0 ? 0 : (ry > 255 ? 255 : ry)));
    bytes.push_back(static_cast<uint8_t>(rp < 0 ? 0 : (rp > 255 ? 255 : rp)));
    bytes.push_back(static_cast<uint8_t>(rt < 0 ? 0 : (rt > 255 ? 255 : rt)));
    return bytes;
}

GhostNumberG GhostNumberG::operator++() {
    x += 1.0;
    return *this;
}

GhostNumberG GhostNumberG::operator++(int) {
    GhostNumberG temp = *this;
    x += 1.0;
    return temp;
}

GhostNumberG GhostNumberG::operator--() {
    x -= 1.0;
    return *this;
}

GhostNumberG GhostNumberG::operator--(int) {
    GhostNumberG temp = *this;
    x -= 1.0;
    return temp;
}

GhostNumberG GhostNumberG::abs() {
    return GhostNumberG(std::abs(x), std::abs(y), std::abs(p), theta2);
}

GhostNumberG GhostNumberG::sqrt(const GhostNumberG& g) {
    return GhostNumberG(std::sqrt(std::abs(g.x)), std::sqrt(std::abs(g.y)), std::sqrt(std::abs(g.p)), g.theta2);
}

GhostNumberG GhostNumberG::exp(const GhostNumberG& g) {
    double ex = std::exp(g.x);
    return GhostNumberG(ex * std::cos(g.y), ex * std::sin(g.y), g.p, g.theta2);
}

GhostNumberG GhostNumberG::log(const GhostNumberG& g) {
    return GhostNumberG(std::log(std::abs(g.x)), std::log(std::abs(g.y)), std::log(std::abs(g.p)), g.theta2);
}

GhostNumberG GhostNumberG::pow(double exponent) {
    return GhostNumberG(std::pow(x, exponent), std::pow(y, exponent), std::pow(p, exponent), theta2);
}

std::vector<std::string> GhostNumberG::get_vector_string(){
    std::vector<std::string> result;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    
    ss.str(""); ss << x; result.push_back(ss.str());
    ss.str(""); ss << y; result.push_back(ss.str());
    ss.str(""); ss << p; result.push_back(ss.str());
    ss.str(""); ss << theta2; result.push_back(ss.str());
    
    return result;
}
std::vector<double> GhostNumberG::get_vector_double(){
    std::vector<double> result;
    result.push_back(x);
    result.push_back(y);
    result.push_back(p);
    result.push_back(theta2);
    return result;  
}
std::vector<int> GhostNumberG::get_vector_int(){
    std::vector<int> result;
    result.push_back(static_cast<int>(x));
    result.push_back(static_cast<int>(y));
    result.push_back(static_cast<int>(p));
    result.push_back(static_cast<int>(theta2));
    return result;
}
std::vector<GhostNumberG> GhostNumberG::get_vector_gnumber(){
    std::vector<GhostNumberG> result;
    result.push_back(GhostNumberG(x));
    result.push_back(GhostNumberG(y));
    result.push_back(GhostNumberG(p));
    result.push_back(GhostNumberG(theta2));
    return result;
}
std::vector<std::complex<double>> GhostNumberG::get_vector_complex(){
    std::vector<std::complex<double>> result;
    result.push_back(std::complex<double>(x, y));
    result.push_back(std::complex<double>(p, 0));
    result.push_back(std::complex<double>(theta2, 0));
    return result;
}

double GhostNumberG::get_double_real() const{
    return x;
}

//GhostNumberG::operator uint8_t() const { return rawByte; }
//GhostNumberG::operator uint8_t() { return rawByte; }

GhostNumberG::operator double() const{
    return x;
}

GhostNumberG::operator int() const{
    return static_cast<int>(x);
}

GhostNumberG::operator std::string() const{
    std::stringstream ss;
    ss << "x: " << x << ", y: " << y << ", p: " << p << ", theta2: " << theta2;
    return ss.str();
}

GhostNumberG::operator std::complex<double>() const{
    return std::complex<double>(x, y);
}

GhostNumberG::operator bool() const{
    return isValid();
}

GhostNumberG::operator unsigned char() const {
    ///return static_cast<unsigned char>(std::round(this->x));
    // Adapte conforme sua estrutura interna
    // Exemplo: suponha que você tenha um membro `uint8_t value;`
    return static_cast<unsigned char>(this->toByte()); 
    // Ou qualquer lógica que converta seu número fantasma para 0-255
}

GhostNumberG GhostNumberG::operator-() const {
    return GhostNumberG(-x, -y, -p, 0.0000);
}

GhostNumberG GhostNumberG::operator-() {
    return GhostNumberG(-x, -y, -p, 0.0000);
}

GhostNumberG GhostNumberG::operator+() {
    return *this;
}

GhostNumberG& GhostNumberG::operator=(const GhostNumberG& other) {
    if (this != &other) {
        this->rawByte = other.rawByte;
        this->x = other.x;
        this->y = other.y;
        this->p = other.p;
        this->theta2 = other.theta2;
        this->b = other.b;
        this->theta = other.theta;
        this->nilpotent = other.nilpotent;
        this->value = other.value;
        this->entropy = other.entropy;
    }
    return *this;
}

double GhostNumberG::get_theta2() const {
    return theta2;
}

double GhostNumberG::get_real() const {
    return x;
}

double GhostNumberG::get_imag() const {
    return y;
}

double GhostNumberG::get_ghost() const {
    return p;
}

GhostNumberG GhostNumberG::zero(double theta2){
    return GhostNumberG(0, 0, 0, 0.0);
}

GhostNumberG GhostNumberG::one(double theta2) {
    return GhostNumberG(1, 0, 0, 0.0);
}

GhostNumberG GhostNumberG::from_seed(int val, const std::string& seed, const std::vector<uint8_t>& context) {
    double entropy_sum = std::accumulate(context.begin(), context.end(), 0.0);
    double seed_modifier = static_cast<double>(std::hash<std::string>{}(seed) % 1000) / 1000.0;

    double x = static_cast<double>(val) + fmod(entropy_sum, 13.0);
    double y = seed_modifier;
    double p = fmod(entropy_sum, 7.0);
    double theta2 = 1.0 + fmod(seed_modifier + x + y + p, 1.0);

    GhostNumberG g(x, y, p, theta2);
    g.entropy = context;
    return g;
}


int GhostNumberG::get_int() const {
    return static_cast<int>(x);
}

double GhostNumberG::get_double() const {
    return x;
}

int GhostNumberG::get_gnumber() const {
    return static_cast<int>(x);
}

GhostNumberG GhostNumberG::i() {
    return GhostNumberG(0, 1, 0, 0.0);
}

GhostNumberG GhostNumberG::pi() {
    return GhostNumberG(3.14159265358979323846, 0, 0, 0.0);
}
GhostNumberG GhostNumberG::e() {
    return GhostNumberG(2.7182818284590452354, 0, 0, 0.0);
}
GhostNumberG GhostNumberG::phi() {
    return GhostNumberG(1.6180339887498948482, 0, 0, 0.0);
}

GhostNumberG GhostNumberG::operator*(double scalar) const {
    return GhostNumberG(x * scalar, y * scalar, p * scalar, theta2);
}

GhostNumberG GhostNumberG::operator/(double scalar) const {
    return GhostNumberG(x / scalar, y / scalar, p / scalar, theta2);
}

GhostNumberG GhostNumberG::operator+(double scalar) const {
    return GhostNumberG(x + scalar, y, p, theta2);
}

GhostNumberG GhostNumberG::operator-(double scalar) const {
    return GhostNumberG(x - scalar, y, p, theta2);
}

bool GhostNumberG::isInvertible() const {
    return std::abs(norm_squared()) > 1e-9;
}

GhostNumberG GhostNumberG::inverse() const {
    // Inversa Regular para g = z1 + z2θ onde θ²=0
    // g^-1 = 1/z1 - (z2/z1²)θ
    // z1 = x + iy
    double z1_norm2 = x * x + y * y;
    if (z1_norm2 < 1e-18) 
        throw std::runtime_error("Número não invertível (base clássica zero).");

    // 1/z1 = (x - iy) / |z1|²
    double inv_z1_x = x / z1_norm2;
    double inv_z1_y = -y / z1_norm2;

    // z2/z1² = p / (x + iy)²
    // z1² = (x² - y²) + i(2xy)
    // 1/z1² = ((x² - y²) - i(2xy)) / |z1|⁴
    double z1_norm4 = z1_norm2 * z1_norm2;
    // Parte fantasma do inverso (p_inv): mantendo a componente escalar p 
    // real(-p * z1^-2)
    double p_inv = -p * (x * x - y * y) / z1_norm4;

    return GhostNumberG(inv_z1_x, inv_z1_y, p_inv, 0.0000);
}

std::complex<double> GhostNumberG::ghostNumberTo_complex() const {
    return std::complex<double>(x, y);
}

// Novas implementações para harmonização de theta e ajuste de componentes
GhostNumberG GhostNumberG::set_components(double new_x, double new_y, double new_p) const {
    return GhostNumberG(new_x, new_y, new_p, theta2);
}

double GhostNumberG::magnitude() const {
    return std::sqrt(x * x + y * y + p * p);
}

// Permite: matrix.at(0,0) = 1; (Sem converter implicitamente, mas atribuindo explicitamente)
GhostNumberG& GhostNumberG::operator=(int v) {
    this->x = static_cast<double>(v);
    this->y = 0.0;
    this->p = 0.0;
    this->theta2 = 0.0;
    this->rawByte = 0;
    return *this;
}

GhostNumberG& GhostNumberG::operator=(double v) {
    this->x = v;
    this->y = 0.0;
    this->p = 0.0;
    this->theta2 = 0.0;
    this->rawByte = 0;
    return *this;
}

GhostNumberG GhostNumberG::harmonize_theta(const GhostNumberG& other) {
    // Para manter a álgebra Phantom G nilpotente, harmonizamos forçando theta2 = 0.0
    this->theta2 = 0.0;
    return *this;
}
#include <cmath>

GhostNumberG GhostNumberG::relu() const {
    double new_x = (x > 0.0) ? x : 0.0;
    double new_p = (x > 0.0) ? p : 0.0;
    return GhostNumberG(new_x, y, new_p, theta2);
}

GhostNumberG GhostNumberG::sigmoid() const {
    double sig = 1.0 / (1.0 + std::exp(-x));
    double new_p = p * (sig * (1.0 - sig));
    return GhostNumberG(sig, y, new_p, theta2);
}

GhostNumberG GhostNumberG::sin() const {
    return GhostNumberG(std::sin(x), y, p * std::cos(x), theta2);
}

GhostNumberG GhostNumberG::cos() const {
    return GhostNumberG(std::cos(x), y, p * -std::sin(x), theta2);
}

GhostNumberG GhostNumberG::tan() const {
    double sec_x = 1.0 / std::cos(x);
    return GhostNumberG(std::tan(x), y, p * (sec_x * sec_x), theta2);
}

GhostNumberG GhostNumberG::tanh() const {
    double th = std::tanh(x);
    double new_p = p * (1.0 - th * th);
    return GhostNumberG(th, y, new_p, theta2);
}

GhostNumberG GhostNumberG::leaky_relu(double alpha) const {
    double new_x = (x > 0.0) ? x : alpha * x;
    double new_p = (x > 0.0) ? p : alpha * p;
    return GhostNumberG(new_x, y, new_p, theta2);
}

GhostNumberG GhostNumberG::gelu() const {
    const double kSqrt2OverPi = 0.7978845608028654;
    double inner = kSqrt2OverPi * (x + 0.044715 * x * x * x);
    double th = std::tanh(inner);
    double cdf = 0.5 * (1.0 + th);
    double gelu_val = x * cdf;
    double d_inner = kSqrt2OverPi * (1.0 + 3.0 * 0.044715 * x * x);
    double d_gelu = cdf + 0.5 * x * (1.0 - th * th) * d_inner;
    double new_p = p * d_gelu;
    return GhostNumberG(gelu_val, y, new_p, theta2);
}
