// Testes de QML (F4): parameter-shift x diferenças finitas, VQE do H2 x diagonalização exata,
// classificador híbrido, save/load e mensagens de erro.
#include "GhostQML.hpp"
#include <cmath>
#include <complex>
#include <cstdio>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FALHOU: " << what << "\n"; ++failures; }
}

// ---- Hamiltoniano denso para conferência (ordem: letra mais à esquerda = qubit mais alto) ----
using cmat = std::vector<std::vector<std::complex<double>>>;
static cmat pauli(char p) {
    using c = std::complex<double>;
    switch (p) {
        case 'X': return {{0, 1}, {1, 0}};
        case 'Y': return {{0, c(0, -1)}, {c(0, 1), 0}};
        case 'Z': return {{1, 0}, {0, -1}};
        default: return {{1, 0}, {0, 1}};
    }
}
static cmat kron(const cmat& a, const cmat& b) {
    cmat r(a.size() * b.size(), std::vector<std::complex<double>>(a.size() * b.size()));
    for (size_t i = 0; i < a.size(); ++i) for (size_t j = 0; j < a.size(); ++j)
        for (size_t k = 0; k < b.size(); ++k) for (size_t l = 0; l < b.size(); ++l)
            r[i * b.size() + k][j * b.size() + l] = a[i][j] * b[k][l];
    return r;
}
// Menor autovalor de uma matriz real simétrica (Jacobi).
static double min_eigen(std::vector<std::vector<double>> a) {
    const size_t n = a.size();
    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0;
        for (size_t i = 0; i < n; ++i) for (size_t j = i + 1; j < n; ++j) off += a[i][j] * a[i][j];
        if (off < 1e-24) break;
        for (size_t p = 0; p < n; ++p) for (size_t q = p + 1; q < n; ++q) {
            if (std::abs(a[p][q]) < 1e-30) continue;
            const double th = 0.5 * std::atan2(2 * a[p][q], a[q][q] - a[p][p]);
            const double c = std::cos(th), s = std::sin(th);
            for (size_t k = 0; k < n; ++k) { const double akp = a[k][p], akq = a[k][q]; a[k][p] = c * akp - s * akq; a[k][q] = s * akp + c * akq; }
            for (size_t k = 0; k < n; ++k) { const double apk = a[p][k], aqk = a[q][k]; a[p][k] = c * apk - s * aqk; a[q][k] = s * apk + c * aqk; }
        }
    }
    double m = a[0][0];
    for (size_t i = 1; i < n; ++i) m = std::min(m, a[i][i]);
    return m;
}

// H2 (STO-3G, 0.735 A) reduzido a 2 qubits - coeficientes de referência da literatura.
static const std::vector<std::pair<double, std::string>> H2 = {
    {-1.052373245772859, "II"}, {0.39793742484318045, "IZ"}, {-0.39793742484318045, "ZI"},
    {-0.01128010425623538, "ZZ"}, {0.18093119978423156, "XX"}};

static void test_parameter_shift() {
    GhostQCircuit c(3);
    c.seed(11);
    c.encode_ry(0, 0);
    c.encode_rx(1, 1, 0.5);
    c.ansatz(2);
    c.ry_param(2, 0);          // parâmetro compartilhado
    c.h(1);
    c.cz(1, 2);
    c.rx_train(0);
    c.observable("ZZI", 0.7);
    c.observable("XIY", -0.3);
    c.observable("IIZ", 1.0);
    const std::vector<double> x = {0.4, -1.3};
    const std::vector<double> g = c.gradient_vec(x);
    double worst = 0;
    for (int i = 0; i < c.num_params(); ++i) {
        const double v = c.param(i), h = 1e-5;
        c.set_param(i, v + h); const double up = c.expect_vec(x);
        c.set_param(i, v - h); const double dn = c.expect_vec(x);
        c.set_param(i, v);
        worst = std::max(worst, std::abs(g[i] - (up - dn) / (2 * h)));
    }
    check(c.num_params() == 13, "contagem de parametros (" + std::to_string(c.num_params()) + ")");
    check(worst < 1e-6, "parameter-shift == diferencas finitas (erro max " + std::to_string(worst) + ")");
}

static void test_vqe_h2() {
    cmat H(4, std::vector<std::complex<double>>(4));
    for (const auto& t : H2) {
        const cmat k = kron(pauli(t.second[0]), pauli(t.second[1]));
        for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) H[i][j] += t.first * k[i][j];
    }
    std::vector<std::vector<double>> real(4, std::vector<double>(4));
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) real[i][j] = H[i][j].real();
    const double exact = min_eigen(real);
    check(std::abs(exact - (-1.857275)) < 1e-5, "energia exata do H2 = " + std::to_string(exact));

    GhostQCircuit c(2);
    c.seed(3);
    c.ansatz(2);
    for (const auto& t : H2) c.observable(t.second, t.first);
    const double e = c.minimize(300, 0.05);
    check(std::abs(e - exact) < 1e-4, "VQE converge para o estado fundamental (" + std::to_string(e) + " vs " + std::to_string(exact) + ")");
}

static void test_classifier() {
    GhostQCircuit c(2);
    c.seed(5);
    c.encode_all();
    c.ansatz(2);
    for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) {
        const double a = i / 7.0 * PI, b = j / 7.0 * PI;
        c.add_sample(a + b > PI ? 1 : 0, a, b);
    }
    check(c.samples() == 64, "64 amostras");
    const double before = c.loss();
    const double after = c.fit(60, 0.1);
    check(after < before, "fit reduz a perda (" + std::to_string(before) + " -> " + std::to_string(after) + ")");
    check(c.accuracy() >= 0.9, "acuracia >= 90% (" + std::to_string(c.accuracy()) + ")");
    check(c.predict(0.2, 0.3) == 0 && c.predict(2.9, 2.8) == 1, "predict devolve rotulos 0/1");

    const std::string path = "qml_test_params.txt";
    c.save(path);
    const double s1 = c.score(1.0, 2.0);
    c.set_param(0, c.param(0) + 1.0);
    c.load(path);
    check(std::abs(c.score(1.0, 2.0) - s1) < 1e-12, "save/load preserva o modelo");
    std::remove(path.c_str());
}

static void test_errors_and_draw() {
    GhostQCircuit c(2);
    bool thrown = false;
    try { c.observable("Z", 1.0); } catch (const std::invalid_argument&) { thrown = true; }
    check(thrown, "observavel com tamanho errado gera erro");
    thrown = false;
    try { c.ry_train(0); c.fit(1, 0.1); } catch (const std::logic_error&) { thrown = true; }
    check(thrown, "fit sem dados gera erro");
    c.encode_ry(1, 2);
    thrown = false;
    try { c.expect(0.1); } catch (const std::invalid_argument&) { thrown = true; }
    check(thrown, "entrada com poucas features gera erro");
    const std::string d = c.draw();
    check(d.find("RY(t0)") != std::string::npos && d.find("RY(x2)") != std::string::npos, "draw mostra parametros e features");
}

int main() {
    test_parameter_shift();
    test_vqe_h2();
    test_classifier();
    test_errors_and_draw();
    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "QML: OK (parameter-shift, VQE H2, classificador hibrido, save/load).\n";
    return 0;
}
