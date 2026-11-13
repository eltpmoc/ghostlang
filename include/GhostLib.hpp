#ifndef GHOST_LIB_HPP
#define GHOST_LIB_HPP

#include "GhostMatrix.hpp"
#include "GhostServer.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <vector>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <complex>

inline double complex_abs(const std::complex<double>& z) { return std::abs(z); }
inline double complex_arg(const std::complex<double>& z) { return std::arg(z); }
inline std::complex<double> complex_conj(const std::complex<double>& z) { return std::conj(z); }
inline double complex_real(const std::complex<double>& z) { return z.real(); }
inline double complex_imag(const std::complex<double>& z) { return z.imag(); }
inline std::complex<double> complex(double r, double im = 0.0) { return std::complex<double>(r, im); }
inline std::complex<double> complex_sqrt(const std::complex<double>& z) { return std::sqrt(z); }
inline std::complex<double> complex_sqrt(double x) { return std::sqrt(std::complex<double>(x, 0.0)); }
inline std::complex<double> csqrt(const std::complex<double>& z) { return std::sqrt(z); }
inline std::complex<double> csqrt(double x) { return std::sqrt(std::complex<double>(x, 0.0)); }
inline std::complex<double> complex_nth_root(const std::complex<double>& z, int n) {
    if (n <= 0) {
        throw std::invalid_argument("indice da raiz deve ser um numero natural maior que zero (n >= 1)");
    }
    if (n == 1) return z;
    if (n == 2) return std::sqrt(z);
    double r = std::abs(z);
    double theta = std::arg(z);
    double r_root = std::pow(r, 1.0 / static_cast<double>(n));
    double theta_root = theta / static_cast<double>(n);
    return std::polar(r_root, theta_root);
}
inline std::complex<double> complex_nth_root(double x, int n) {
    return complex_nth_root(std::complex<double>(x, 0.0), n);
}
inline std::complex<double> nth_root_c(const std::complex<double>& z, int n) { return complex_nth_root(z, n); }
inline std::complex<double> nth_root_c(double x, int n) { return complex_nth_root(x, n); }
inline std::complex<double> croot(const std::complex<double>& z, int n) { return complex_nth_root(z, n); }
inline std::complex<double> croot(double x, int n) { return complex_nth_root(x, n); }
inline std::complex<double> raiz_n_complexa(const std::complex<double>& z, int n) { return complex_nth_root(z, n); }
inline std::complex<double> raiz_n_complexa(double x, int n) { return complex_nth_root(x, n); }
inline std::complex<double> raiz_quadrada_complexa(const std::complex<double>& z) { return std::sqrt(z); }
inline std::complex<double> raiz_quadrada_complexa(double x) { return std::sqrt(std::complex<double>(x, 0.0)); }
inline std::complex<double> raiz_cubica_complexa(const std::complex<double>& z) { return complex_nth_root(z, 3); }
inline std::complex<double> raiz_cubica_complexa(double x) { return complex_nth_root(x, 3); }

// ==========================================
// 0. Universal String Concatenation
// ==========================================
#include <type_traits>
#include "GhostNumText.hpp"
#include "GhostFormat.hpp"

// "texto" + numero: mesmo texto do print e do REPL (2.0 -> "2", 2.5 -> "2.5")
template <typename T>
inline std::string ghost_arith_text(const T& v) {
    if constexpr (std::is_floating_point<T>::value) return ghost_num_text(static_cast<double>(v));
    else return std::to_string(v);
}

template <typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
operator+(const std::string& lhs, const T& rhs) {
    return lhs + ghost_arith_text(rhs);
}

template <typename T>
inline typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
operator+(const T& lhs, const std::string& rhs) {
    return ghost_arith_text(lhs) + rhs;
}

inline std::string operator+(const std::string& lhs, const GhostNumberG& rhs) {
    std::ostringstream oss;
    oss << lhs << rhs.to_string();
    return oss.str();
}

inline std::string operator+(const GhostNumberG& lhs, const std::string& rhs) {
    std::ostringstream oss;
    oss << lhs.to_string() << rhs;
    return oss.str();
}

// ==========================================
// 1. Core I/O
// ==========================================
template<typename T>
auto ghost_convert_arg(T&& arg) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return arg.c_str();
    } else {
        return std::forward<T>(arg);
    }
}

template<typename... Args>
void ghost_print(const char* format, Args... args) {
    printf(format, ghost_convert_arg(std::forward<Args>(args))...);
}

struct GhostInputResult {
    std::string text;
    GhostInputResult(std::string s = "") : text(std::move(s)) {}

    operator std::string() const { return text; }
    operator const char*() const { return text.c_str(); }
    operator int() const {
        try {
            size_t idx = 0;
            return std::stoi(text, &idx);
        } catch (...) { return 0; }
    }
    operator long long() const {
        try { return std::stoll(text); } catch (...) { return 0LL; }
    }
    operator double() const {
        try { return std::stod(text); } catch (...) { return 0.0; }
    }
    operator float() const {
        try { return std::stof(text); } catch (...) { return 0.0f; }
    }
    operator bool() const {
        return !text.empty() && text != "0" && text != "false" && text != "falso";
    }
    operator GhostNumberG() const {
        return parse_ghost_number(text);
    }

    std::string str() const { return text; }
    int to_int() const { return operator int(); }
    double to_double() const { return operator double(); }
};

inline std::ostream& operator<<(std::ostream& os, const GhostInputResult& r) {
    return os << r.text;
}

template<typename T>
inline void ghost_input(T& var) {
    if (std::cin.peek() == '\n') std::cin.get();
    else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
    std::string s;
    if (std::getline(std::cin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        try {
            std::istringstream iss(s);
            iss >> var;
        } catch (...) {}
    }
}

template<>
inline void ghost_input<std::string>(std::string& var) {
    if (std::cin.peek() == '\n') std::cin.get();
    else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
    if (std::getline(std::cin, var)) {
        if (!var.empty() && var.back() == '\r') var.pop_back();
    }
}

template<>
inline void ghost_input<GhostNumberG>(GhostNumberG& var) {
    if (std::cin.peek() == '\n') std::cin.get();
    else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
    std::string s;
    if (std::getline(std::cin, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        var = parse_ghost_number(s);
    }
}

inline GhostInputResult input(const std::string& prompt = "") {
    if (!prompt.empty()) {
        std::cout << prompt << std::flush;
    }
    if (std::cin.peek() == '\n') std::cin.get();
    else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
    std::string line;
    if (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return GhostInputResult(line);
    }
    return GhostInputResult("");
}

inline GhostInputResult inputln(const std::string& prompt = "") {
    return input(prompt);
}

// Permite encerrar interfaces GhostLang ao receber EOF (Ctrl+D ou pipe).
inline bool input_closed() { return std::cin.fail(); }

inline int to_int(const std::string& str) {
    try { return std::stoi(str); } catch (...) { return 0; }
}

inline std::string to_string(const GhostMatrix& mat) {
    std::ostringstream oss;
    oss << "[";
    for (size_t r = 0; r < mat.getRows(); ++r) {
        oss << mat(r, 0).x;
        if (r < mat.getRows() - 1) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

// ==========================================
// F-strings: f"x = ${x}" -> ghost_fstr(std::string("x = "), (x))
// Cada parte e escrita com <<; matrizes usam to_string e bool vira true/false.
// ==========================================
template <typename T>
inline void ghost_fstr_piece(std::ostringstream& oss, const T& value) { oss << value; }
inline void ghost_fstr_piece(std::ostringstream& oss, const GhostMatrix& value) { oss << to_string(value); }
inline void ghost_fstr_piece(std::ostringstream& oss, bool value) { oss << (value ? "true" : "false"); }
inline void ghost_fstr_piece(std::ostringstream& oss, double value) { oss << ghost_num_text(value); }
inline void ghost_fstr_piece(std::ostringstream& oss, float value) { oss << ghost_num_text(value); }

// f"${x:.2f}" -> ghost_fmt((x), ".2f")  (include/GhostFormat.hpp, o mesmo código do REPL)
template <typename T>
inline std::string ghost_fmt(const T& value, const char* spec) {
    if constexpr (std::is_same_v<T, GhostNumberG>) {
        std::string sp(spec ? spec : "");
        if (sp == "4d" || sp == "4D" || sp == "vetorial") return value.to_4d(false);
        if (sp == "G=4d" || sp == "G=4D") return value.to_4d(true);
        if (sp == "math" || sp == "alg" || sp == "matematica") return value.to_math(false);
        if (sp == "G=math" || sp == "G=alg") return value.to_math(true);
        return value.to_string();
    } else if constexpr (std::is_same_v<T, bool>) {
        GhostFmtSpec f;
        if (ghost_parse_fmt(spec, f) && f.type && f.type != 's') return ghost_fmt_num(value ? 1.0 : 0.0, true, spec);
        return ghost_fmt_text(value ? "true" : "false", spec);
    } else if constexpr (std::is_arithmetic_v<T>) {
        return ghost_fmt_num(static_cast<double>(value), std::is_integral_v<T>, spec);
    } else if constexpr (std::is_convertible_v<T, std::string>) {
        return ghost_fmt_text(std::string(value), spec);
    } else {
        std::ostringstream oss;
        ghost_fstr_piece(oss, value);
        return ghost_fmt_text(oss.str(), spec);
    }
}

template <typename... Args>
inline std::string ghost_fstr(const Args&... args) {
    std::ostringstream oss;
    (ghost_fstr_piece(oss, args), ...);
    return oss.str();
}

inline GhostMatrix to_state(const std::string& str) {
    std::string s = str;
    s.erase(std::remove(s.begin(), s.end(), '['), s.end());
    s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
    std::vector<double> vals;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try { vals.push_back(std::stod(item)); } catch(...) { }
    }
    if (vals.empty()) return GhostMatrix(1, 1);
    GhostMatrix gm(vals.size(), 1);
    for (size_t i = 0; i < vals.size(); ++i) gm(i, 0) = GhostNumberG(vals[i], 0.0, 0.0);
    return gm;
}

// Tokenizacao e Embeddings Basicos (ASCII Normalizado)
inline GhostMatrix encode(const std::string& text, size_t dim) {
    GhostMatrix gm(dim, 1);
    for (size_t i = 0; i < dim; ++i) {
        if (i < text.size()) {
            double norm = static_cast<double>(text[i]) / 255.0; // Normaliza de 0 a 1
            double freq = i * 0.1; // Frequência espacial para o RoPE
            gm(i, 0) = GhostNumberG(norm * std::cos(freq), 0.0, norm * std::sin(freq));
        } else {
            gm(i, 0) = GhostNumberG(0.0, 0.0, 0.0); // Padding
        }
    }
    return gm;
}

inline std::string decode(const GhostMatrix& mat) {
    std::string result = "";
    for (size_t r = 0; r < mat.getRows(); ++r) {
        double real_v = mat(r, 0).x;
        double ghost_v = mat(r, 0).p;
        double magnitude = std::sqrt(real_v * real_v + ghost_v * ghost_v);
        if (magnitude <= 0.01) continue; // Pula padding e zeros
        int ascii = static_cast<int>(std::round(magnitude * 255.0));
        // Limita ao range de caracteres imprimiveis validos para seguranca
        if (ascii < 32) ascii = 32; 
        if (ascii > 126) ascii = 126;
        result += static_cast<char>(ascii);
    }
    return result;
}

// ==========================================
// 2. Extended File I/O
// ==========================================
inline std::string readfile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string s(fsize, '\0');
    size_t bytes_read = fread(&s[0], 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        s.resize(bytes_read);
    }
    fclose(f);
    return s;
}

inline std::string openfile(const std::string& path) { return readfile(path); }

inline void writefile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (file.is_open()) { file << content; file.close(); }
}

template <typename T>
inline void save(const T& content, const std::string& path) {
    std::ofstream file(path);
    if (file.is_open()) { file << content; file.close(); }
}

inline std::string load(const std::string& path) {
    return readfile(path);
}

inline std::string read(const std::string& path) {
    return readfile(path);
}

inline std::vector<std::string> string_split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline int ghost_to_int(const std::string& s) {
    try { return std::stoi(s); } catch(...) { return 10; }
}

inline std::string string_contains(const std::string& text, const std::string& pattern) {
    return text.find(pattern) != std::string::npos ? "true" : "false";
}

inline void deletefile(const std::string& path) {
    std::remove(path.c_str());
}

inline void copyfile(const std::string& src, const std::string& dest) {
    std::ifstream srcFile(src, std::ios::binary);
    std::ofstream destFile(dest, std::ios::binary);
    if (srcFile.is_open() && destFile.is_open()) { destFile << srcFile.rdbuf(); }
}

inline std::string searchInFile(const std::string& path, const std::string& query) {
    std::ifstream file(path);
    if (!file.is_open()) return "[Ghost AI] Erro: Arquivo inacessivel.";
    std::string line;
    std::string result = "[Ghost AI Search] Analise para '" + query + "':\n";
    int lineNum = 1;
    bool found = false;
    while (std::getline(file, line)) {
        if (line.find(query) != std::string::npos) {
            result += " > Linha " + std::to_string(lineNum) + ": " + line + "\n";
            found = true;
        }
        lineNum++;
    }
    if (!found) return "[Ghost AI Search] Padrao nao reconhecido.\n";
    return result;
}

inline bool file_exists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

inline void dir_create(const std::string& path) {
    // Para simplificar, usamos system mkdir nativo do OS subjacente
    std::string cmd = "mkdir \"" + path + "\" >nul 2>&1";
    system(cmd.c_str());
}

// ==========================================
// 3. Regex Wrapper
// ==========================================
inline bool regex_match(const std::string& text, const std::string& pattern) {
    std::regex re(pattern);
    return std::regex_match(text, re);
}

inline std::string regex_replace(const std::string& text, const std::string& pattern, const std::string& replacement) {
    std::regex re(pattern);
    return std::regex_replace(text, re, replacement);
}

inline bool regex_search(const std::string& text, const std::string& pattern) {
    std::regex re(pattern);
    return std::regex_search(text, re);
}

// Helpers for JSON literal dynamic transpilation
template <typename T>
inline std::string ghost_to_json_string(const T& val) {
    return std::to_string(val);
}
inline std::string ghost_to_json_string(const std::string& val) {
    return "\"" + val + "\"";
}
inline std::string ghost_to_json_string(const char* val) {
    return "\"" + std::string(val) + "\"";
}
inline std::string ghost_to_json_string(bool val) {
    return val ? "true" : "false";
}

// ==========================================
// 4. GhostDate and Number Formatting
// ==========================================

inline std::string format_number(double val, int decimals) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << val;
    return ss.str();
}

inline double parse_number(const std::string& str) {
    try { return std::stod(str); } catch (...) { return 0.0; }
}

class GhostDate {
public:
    std::time_t time_val;
    
    GhostDate() {
        time_val = std::time(nullptr);
    }
    
    GhostDate(std::time_t t) : time_val(t) {}
    
    GhostDate(const std::string& str, const std::string& format = "%Y-%m-%d %H:%M:%S") {
        std::tm t = {};
        std::istringstream ss(str);
        ss >> std::get_time(&t, format.c_str());
        if (ss.fail()) {
            time_val = std::time(nullptr); // Fallback if parsing fails
        } else {
            time_val = std::mktime(&t);
        }
    }
    
    std::string format(const std::string& fmt) const {
        std::tm* t = std::localtime(&time_val);
        if (!t) return "";
        std::ostringstream ss;
        ss << std::put_time(t, fmt.c_str());
        return ss.str();
    }
};

inline GhostDate parse_date(const std::string& str, const std::string& format) {
    return GhostDate(str, format);
}

inline std::string format_date(const GhostDate& date, const std::string& format) {
    return date.format(format);
}

inline GhostDate now() {
    return GhostDate();
}

// Operators for concatenation with string
inline std::string operator+(const std::string& lhs, const GhostDate& rhs) {
    return lhs + rhs.format("%Y-%m-%d %H:%M:%S");
}
inline std::string operator+(const GhostDate& lhs, const std::string& rhs) {
    return lhs.format("%Y-%m-%d %H:%M:%S") + rhs;
}

// Ostream operator for print
inline std::ostream& operator<<(std::ostream& os, const GhostDate& date) {
    os << date.format("%Y-%m-%d %H:%M:%S");
    return os;
}

// ==========================================
// 5. GhostJson (Mini Parser Nativo)
// ==========================================
class GhostJson {
public:
    std::map<std::string, std::string> data;

    GhostJson() {}
    GhostJson(const std::string& raw) {
        parse_raw(raw);
    }
    GhostJson(const char* raw) {
        if (raw) parse_raw(std::string(raw));
    }

    static GhostJson parse(const std::string& json_str) {
        return GhostJson(json_str);
    }

    static std::string stringify(const GhostJson& j) {
        return j.toString();
    }

    void parse_raw(const std::string& json_str) {
        data.clear();
        // Implementação para parsear {"key": "value"} ou {"key": 123} com suporte a escapes
        std::string cleaned;
        cleaned.reserve(json_str.size());
        for (size_t k = 0; k < json_str.size(); ++k) {
            if (json_str[k] == '\\' && k + 1 < json_str.size() && (json_str[k+1] == '"' || json_str[k+1] == '\\')) {
                cleaned += json_str[k+1];
                ++k;
            } else {
                cleaned += json_str[k];
            }
        }
        // Limpar chaves externas e espacos extras
        size_t first_brace = cleaned.find('{');
        size_t last_brace = cleaned.find_last_of('}');
        if (first_brace != std::string::npos && last_brace != std::string::npos && last_brace > first_brace) {
            cleaned = cleaned.substr(first_brace + 1, last_brace - first_brace - 1);
        }

        std::string current_key;
        std::string current_val;
        bool in_quotes = false;
        bool is_key = true;

        for (size_t i = 0; i < cleaned.size(); ++i) {
            char c = cleaned[i];
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ':' && !in_quotes) {
                is_key = false;
            } else if (c == ',' && !in_quotes) {
                if(!current_key.empty()) {
                    data[stripQuotes(current_key)] = stripQuotes(current_val);
                }
                current_key.clear();
                current_val.clear();
                is_key = true;
            } else {
                if (is_key) current_key += c;
                else current_val += c;
            }
        }
        if (!current_key.empty()) {
            data[stripQuotes(current_key)] = stripQuotes(current_val);
        }
    }

    std::string get(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) return it->second;
        return "null";
    }

    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }

    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
    }

    size_t size() const {
        return data.size();
    }

    bool empty() const {
        return data.empty();
    }

    std::string& operator[](const std::string& key) {
        return data[key];
    }

    std::string operator[](const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) return it->second;
        return "";
    }

    std::string& operator[](const char* key) {
        return data[std::string(key ? key : "")];
    }

    std::string operator[](const char* key) const {
        if (!key) return "";
        auto it = data.find(key);
        if (it != data.end()) return it->second;
        return "";
    }

    std::string toString() const {
        std::string out = "{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) out += ", ";
            if (isJsonLiteralValue(pair.second)) {
                out += "\"" + pair.first + "\": " + pair.second;
            } else {
                out += "\"" + pair.first + "\": \"" + pair.second + "\"";
            }
            first = false;
        }
        out += "}";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& os, const GhostJson& j) {
        os << j.toString();
        return os;
    }

private:
    static bool isJsonLiteralValue(const std::string& s) {
        if (s.empty()) return false;
        if (s == "true" || s == "false" || s == "null") return true;
        if (s.front() == '{' && s.back() == '}') return true;
        if (s.front() == '[' && s.back() == ']') return true;
        size_t start = (s[0] == '-') ? 1 : 0;
        if (start == s.size()) return false;
        bool has_dot = false;
        for (size_t i = start; i < s.size(); ++i) {
            if (s[i] == '.') {
                if (has_dot) return false;
                has_dot = true;
            } else if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
                return false;
            }
        }
        return true;
    }

    std::string stripQuotes(const std::string& str) {
        std::string t = trim(str);
        if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
            return t.substr(1, t.size() - 2);
        }
        return t;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }
};

inline std::string to_string(const GhostJson& j) {
    return j.toString();
}

inline std::string ghost_to_json_string(const GhostJson& j) {
    return j.toString();
}

inline GhostJson json_parse(const std::string& str) {
    return GhostJson(str);
}

inline std::string json_get(const GhostJson& j, const std::string& key) {
    return j.get(key);
}

inline std::string json_get(GhostJson& j, const std::string& key) {
    return j.get(key);
}

inline std::string json_stringify(const GhostJson& j) {
    return j.toString();
}

// ==========================================
// 5. Ghost HTTP File Helpers (C++ Nativo)
// Usados internamente pela lib/http.g
// ==========================================

inline std::string ghost_http_mime(const std::string& path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html";
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")   return "application/javascript";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")  return "text/css";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".json") return "application/json";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")  return "image/png";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")  return "image/jpeg";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".svg")  return "image/svg+xml";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".ico")  return "image/x-icon";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".txt")  return "text/plain";
    return "application/octet-stream";
}

inline std::string ghost_http_file(const std::string& filepath, const std::string& content_type) {
    std::string content = readfile(filepath);
    if (content.empty()) {
        return "STATUS:404|TYPE:application/json|BODY:{\"error\": \"Arquivo nao encontrado: " + filepath + "\"}";
    }
    return "STATUS:200|TYPE:" + content_type + "|BODY:" + content;
}

inline std::string ghost_http_auto_file(const std::string& filepath) {
    return ghost_http_file(filepath, ghost_http_mime(filepath));
}

// ==========================================
// 6. HTTP Client & JSON Extractor (Multi-Agent)
// ==========================================
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wininet.h>
#include <conio.h>

inline std::string key_pressed() {
    if (_kbhit()) {
        char c = _getch();
        return std::string(1, c);
    }
    return "";
}

inline std::string wait_key() {
    char c = _getch();
    return std::string(1, c);
}

inline std::string http_request(const std::string& method, const std::string& url_str, const std::string& body = "") {
    HINTERNET hInternet = InternetOpenA("GhostLang Agent/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "{\"error\": \"InternetOpen falhou\"}";

    std::string host = "";
    std::string path = "/";
    int port = INTERNET_DEFAULT_HTTP_PORT;
    
    std::string url = url_str;
    if (url.substr(0, 7) == "http://") url = url.substr(7);
    else if (url.substr(0, 8) == "https://") {
        url = url.substr(8);
        port = INTERNET_DEFAULT_HTTPS_PORT;
    }

    size_t slash_pos = url.find('/');
    if (slash_pos != std::string::npos) {
        host = url.substr(0, slash_pos);
        path = url.substr(slash_pos);
    } else {
        host = url;
    }

    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        port = std::stoi(host.substr(colon_pos + 1));
        host = host.substr(0, colon_pos);
    }

    HINTERNET hConnect = InternetConnectA(hInternet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "{\"error\": \"InternetConnect falhou\"}";
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (port == INTERNET_DEFAULT_HTTPS_PORT) {
        flags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, (const char**)"*/*\0", flags, 1);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "{\"error\": \"HttpOpenRequest falhou\"}";
    }

    std::string headers = "Content-Type: application/json\r\n";
    BOOL bSend = HttpSendRequestA(hRequest, headers.c_str(), headers.length(), body.empty() ? NULL : (LPVOID)body.c_str(), body.length());
    
    std::string response = "";
    if (bSend) {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response += buffer;
        }
    } else {
        response = "{\"error\": \"HttpSendRequest falhou\"}";
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

inline std::string http_get(const std::string& url) {
    return http_request("GET", url);
}

inline std::string http_post(const std::string& url, const std::string& body) {
    return http_request("POST", url, body);
}
#else
// Linux/macOS Implementation (POSIX)
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <memory>

inline std::string wait_key() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return std::string(1, ch);
}

inline std::string key_pressed() {
    struct termios oldt, newt;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    int c = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if (c != EOF) {
        return std::string(1, (char)c);
    }
    return "";
}

inline std::string http_request_linux(const std::string& method, const std::string& url, const std::string& body = "") {
    std::string cmd = "curl -s -X " + method;
    if (!body.empty()) {
        std::string escaped_body = body;
        size_t pos = 0;
        while ((pos = escaped_body.find("'", pos)) != std::string::npos) {
            escaped_body.replace(pos, 1, "'\\''");
            pos += 4;
        }
        cmd += " -H \"Content-Type: application/json\" -d '" + escaped_body + "'";
    }
    cmd += " \"" + url + "\"";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "{\"error\": \"popen failed\"}";
    
    char buffer[4096];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

inline std::string http_get(const std::string& url) {
    return http_request_linux("GET", url);
}

inline std::string http_post(const std::string& url, const std::string& body) {
    return http_request_linux("POST", url, body);
}
#endif

inline std::string json_extract(const std::string& json_str, const std::string& key) {
    std::string target = "\"" + key + "\"";
    size_t pos = json_str.find(target);
    if (pos == std::string::npos) {
        // Tentar sem aspas (caso o PowerShell tenha removido)
        target = key + ":";
        pos = json_str.find(target);
        if (pos == std::string::npos) {
            target = key + " :";
            pos = json_str.find(target);
            if (pos == std::string::npos) return "";
        }
    } else {
        pos = json_str.find(':', pos);
        if (pos == std::string::npos) return "";
    }
    
    pos++;
    while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\t' || json_str[pos] == '\r' || json_str[pos] == '\n')) {
        pos++;
    }
    
    if (pos >= json_str.length()) return "";
    
    std::string value;
    if (json_str[pos] == '"') {
        pos++;
        while (pos < json_str.length()) {
            if (json_str[pos] == '\\') {
                value += json_str[pos];
                pos++;
                if (pos < json_str.length()) value += json_str[pos];
            } else if (json_str[pos] == '"') {
                break;
            } else {
                value += json_str[pos];
            }
            pos++;
        }
    } else {
        while (pos < json_str.length() && json_str[pos] != ',' && json_str[pos] != '}' && json_str[pos] != ' ' && json_str[pos] != '\r' && json_str[pos] != '\n') {
            value += json_str[pos];
            pos++;
        }
    }
    return value;
}
#include <chrono>
#include <thread>
#include <cstdlib>

// ==========================================
// 7. Time & OS Natives
// ==========================================
inline std::string ghost_time_now_ms() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return std::to_string(ms.count());
}

inline void ghost_sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline std::string ghost_env_get(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : "";
}

inline std::string ghost_system_exec(const std::string& cmd) {
    std::string result = "";
#ifdef _MSC_VER
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return "{\"error\": \"popen failed\"}";
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
#ifdef _MSC_VER
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return result;
}

// ==========================================
// 8. Crypto Natives (Base64)
// ==========================================
inline std::string ghost_base64_encode(const std::string &in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

inline std::string ghost_base64_decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    int val=0, valb=-8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}

#endif
