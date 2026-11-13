// Testes de QAOA (MaxCut) e kernels quânticos (F4).
#include "GhostQML.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FALHOU: " << what << "\n"; ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }

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
            for (size_t k = 0; k < n; ++k) { const double x = a[k][p], y = a[k][q]; a[k][p] = c * x - s * y; a[k][q] = s * x + c * y; }
            for (size_t k = 0; k < n; ++k) { const double x = a[p][k], y = a[q][k]; a[p][k] = c * x - s * y; a[q][k] = s * x + c * y; }
        }
    }
    double m = a[0][0];
    for (size_t i = 1; i < n; ++i) m = std::min(m, a[i][i]);
    return m;
}

static void test_rzz_gate() {
    GhostQReg q(2);
    const double th = 0.7;
    q.h(0); q.h(1); q.rzz(0, 1, th);
    check(near(std::arg(q.amplitude(0)), -th / 2) && near(std::arg(q.amplitude(3)), -th / 2), "RZZ: fase e^{-i th/2} em |00> e |11>");
    check(near(std::arg(q.amplitude(1)), th / 2) && near(std::arg(q.amplitude(2)), th / 2), "RZZ: fase e^{+i th/2} em |01> e |10>");
    check(near(q.norm(), 1.0), "RZZ preserva a norma");
}

static void test_scaled_gradients() {
    GhostQCircuit c(3);
    c.seed(21);
    c.h(0); c.h(1); c.h(2);
    const int g = c.rzz_train(0, 1);
    c.rzz_param(1, 2, g, -0.7);          // mesmo θ, escala diferente
    const int b = c.add_param(0.3);
    for (int q = 0; q < 3; ++q) c.rx_param_scaled(q, b, 2.0);
    c.encode_rzz(0, 2, 0, 1, 0.8);       // feature de produto
    c.ry_train(1);
    c.observable("ZZI", 0.5);
    c.observable("IXZ", -1.2);
    const std::vector<double> x = {0.6, -1.1};
    const auto grad = c.gradient_vec(x);
    double worst = 0;
    for (int i = 0; i < c.num_params(); ++i) {
        const double v = c.param(i), h = 1e-5;
        c.set_param(i, v + h); const double up = c.expect_vec(x);
        c.set_param(i, v - h); const double dn = c.expect_vec(x);
        c.set_param(i, v);
        worst = std::max(worst, std::abs(grad[i] - (up - dn) / (2 * h)));
    }
    check(worst < 1e-6, "parameter-shift com escala e RZZ == diferencas finitas (erro " + std::to_string(worst) + ")");

    // gradiente de entrada da qlayer com feature de produto
    GhostQLayer L(2, 1);
    L.seed(2);
    L.circuit().encode_rzz(0, 1, 0, 1, 0.9);
    const std::vector<double> xi = {0.4, -0.8}, gout = {0.7, -0.3};
    std::vector<double> gin(2, 0.0), gth(L.num_params(), 0.0);
    L.backward(xi, gout, gin, gth);
    auto Lf = [&](std::vector<double> xx) { auto y = L.forward(xx); return gout[0] * y[0] + gout[1] * y[1]; };
    double w2 = 0;
    for (int i = 0; i < 2; ++i) {
        auto p = xi, m = xi; p[i] += 1e-5; m[i] -= 1e-5;
        w2 = std::max(w2, std::abs(gin[i] - (Lf(p) - Lf(m)) / 2e-5));
    }
    check(w2 < 1e-6, "qlayer: gradiente de entrada com x_i*x_j (erro " + std::to_string(w2) + ")");
}

static void test_qaoa() {
    // Triângulo com pesos: corte máximo = 2 + 3 = 5
    GhostQCircuit t(3);
    t.edge(0, 1, 1.0); t.edge(1, 2, 2.0); t.edge(0, 2, 3.0);
    check(near(t.maxcut_exact(), 5.0), "forca bruta: triangulo ponderado = 5");
    check(near(t.cut_of("001"), 4.0) && near(t.cut_of("010"), 3.0) && near(t.cut_of("011"), 5.0), "cut_of avalia solucoes");

    // Anel de 6 vértices + 2 cordas
    GhostQCircuit c(6);
    c.seed(4);
    for (int i = 0; i < 6; ++i) c.edge(i, (i + 1) % 6);
    c.edge(0, 3); c.edge(1, 4);
    c.qaoa(3);
    check(c.num_params() == 6, "QAOA p=3 tem 6 parametros (gamma, beta)");
    const double exact = c.maxcut_exact();
    const double before = c.expected_cut();
    c.minimize(150, 0.05);
    const double after = c.expected_cut();
    const std::string sol = c.maxcut_solution(2000);
    std::cout << "  QAOA: corte esperado " << before << " -> " << after << " de " << exact << " (razao " << after / exact
              << "), melhor amostra " << sol << " = " << c.cut_of(sol) << "\n";
    check(after > before, "otimizacao aumenta o corte esperado");
    check(after / exact >= 0.8, "razao de aproximacao >= 0.8");
    check(near(c.cut_of(sol), exact), "amostragem encontra o corte maximo");
}

static void test_kernels() {
    GhostQCircuit fm(2);
    fm.zz_feature_map(2, 1.0);
    std::mt19937_64 rng(3);
    std::uniform_real_distribution<double> d(-1, 1);
    std::vector<std::vector<double>> pts;
    for (int i = 0; i < 10; ++i) pts.push_back({d(rng), d(rng)});
    bool ok = true;
    std::vector<std::vector<double>> K(10, std::vector<double>(10));
    for (int i = 0; i < 10; ++i) for (int j = 0; j < 10; ++j) {
        K[i][j] = fm.kernel_vec(pts[i], pts[j]);
        if (K[i][j] < -1e-12 || K[i][j] > 1 + 1e-12) ok = false;
    }
    for (int i = 0; i < 10; ++i) { if (!near(K[i][i], 1.0, 1e-12)) ok = false; for (int j = 0; j < 10; ++j) if (!near(K[i][j], K[j][i], 1e-12)) ok = false; }
    check(ok, "kernel: k(x,x)=1, simetrico e em [0,1]");
    check(min_eigen(K) > -1e-9, "kernel: matriz de Gram semidefinida positiva");
    check(near(fm.kernel(0.1, 0.2, 0.1, 0.2), 1.0, 1e-12), "kernel(x0, x1, y0, y1) divide os argumentos");

    // XOR: classe 1 quando x0*x1 > 0 (nao linearmente separavel)
    GhostQCircuit c(2);
    c.zz_feature_map(1, 1.0); // 1 repeticao: com 2, o kernel perde posto neste problema
    for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) {
        const double a = -0.9 + 0.36 * i, b = -0.9 + 0.36 * j;
        c.add_sample(a * b > 0 ? 1 : 0, a, b);
    }
    for (int i = 0; i < 5; ++i) for (int j = 0; j < 5; ++j) {
        const double a = -0.75 + 0.375 * i + 0.01, b = -0.75 + 0.375 * j + 0.02;
        c.add_test(a * b > 0 ? 1 : 0, a, b);
    }
    const double train = c.kernel_fit(1e-3);
    const double test = c.kernel_test_accuracy();
    std::cout << "  kernel XOR: treino " << train * 100 << "%, teste " << test * 100 << "%\n";
    check(train >= 0.99, "kernel ridge: acuracia de treino >= 95%");
    check(test >= 0.9, "kernel ridge: acuracia de teste >= 90%");
    check(c.kernel_predict(0.5, 0.5) == 1 && c.kernel_predict(-0.5, 0.5) == 0, "kernel_predict no XOR");

    bool thrown = false;
    try { GhostQCircuit e(2); e.kernel_predict(0.1, 0.2); } catch (const std::logic_error&) { thrown = true; }
    check(thrown, "kernel_predict antes de kernel_fit gera erro");
}

int main() {
    test_rzz_gate();
    test_scaled_gradients();
    test_qaoa();
    test_kernels();
    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "QAOA/Kernels: OK (RZZ, gradientes com escala, MaxCut, kernel PSD, XOR).\n";
    return 0;
}
