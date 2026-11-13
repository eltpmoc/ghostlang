// Testes da qlayer e da rede híbrida (F4): gradientes x diferenças finitas e treino real.
#include "GhostHybrid.hpp"
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

static void randomize(GhostMatrix& W, std::mt19937_64& rng, double scale = 0.8) {
    std::uniform_real_distribution<double> d(-scale, scale);
    for (size_t r = 0; r < W.getRows(); ++r) {
        for (size_t c = 0; c < W.getCols(); ++c) W(r, c) = GhostNumberG(d(rng), 0.0, 0.0);
        W.bias[r] = GhostNumberG(d(rng), 0.0, 0.0);
    }
}

static void test_qlayer_gradients() {
    GhostQLayer q(3, 2);
    q.seed(9);
    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> d(-1.5, 1.5);
    std::vector<double> x = {d(rng), d(rng), d(rng)}, gout = {d(rng), d(rng), d(rng)};
    std::vector<double> gin(3, 0.0), gth(q.num_params(), 0.0);
    q.backward(x, gout, gin, gth);
    auto L = [&](const std::vector<double>& xx) { auto y = q.forward(xx); double s = 0; for (int i = 0; i < 3; ++i) s += gout[i] * y[i]; return s; };
    const double h = 1e-5;
    double worst = 0;
    for (int i = 0; i < 3; ++i) {
        auto xp = x, xm = x; xp[i] += h; xm[i] -= h;
        worst = std::max(worst, std::abs(gin[i] - (L(xp) - L(xm)) / (2 * h)));
    }
    for (int p = 0; p < q.num_params(); ++p) {
        double& t = *q.param_ptr(p); const double v = t;
        t = v + h; const double up = L(x);
        t = v - h; const double dn = L(x);
        t = v;
        worst = std::max(worst, std::abs(gth[p] - (up - dn) / (2 * h)));
    }
    check(worst < 1e-6, "qlayer: gradientes de entrada e de theta == diferencas finitas (erro " + std::to_string(worst) + ")");
    check(q.forward(x).size() == 3, "qlayer: 3 saidas");
}

static void test_network_gradients() {
    std::mt19937_64 rng(2);
    GhostMatrix W1(3, 2), W2(1, 3), A(3, 3), B(1, 3);
    randomize(W1, rng); randomize(W2, rng); randomize(A, rng); randomize(B, rng);
    GhostQLayer Q(3, 2);
    Q.seed(4);
    GhostMatrix X(2, 5), Y(1, 5);
    std::uniform_real_distribution<double> d(-1, 1);
    for (size_t c = 0; c < 5; ++c) { X(0, c) = GhostNumberG(d(rng), 0, 0); X(1, c) = GhostNumberG(d(rng), 0, 0); Y(0, c) = GhostNumberG(d(rng) > 0 ? 1.0 : 0.0, 0, 0); }

    // densa -> densa (ReLU) -> qlayer -> densa
    GhostHybridNetwork net("teste", {&W1, &A, &Q, &W2});
    std::vector<double> g;
    net.loss_and_grad(X, Y, &g);
    auto ptrs = net.parameters();
    check(static_cast<int>(ptrs.size()) == net.num_params() && g.size() == ptrs.size(), "rede: contagem de parametros");
    const double h = 1e-6;
    double worst = 0;
    for (size_t p = 0; p < ptrs.size(); ++p) {
        const double v = *ptrs[p];
        *ptrs[p] = v + h; const double up = net.loss(X, Y);
        *ptrs[p] = v - h; const double dn = net.loss(X, Y);
        *ptrs[p] = v;
        worst = std::max(worst, std::abs(g[p] - (up - dn) / (2 * h)));
    }
    check(worst < 1e-5, "rede hibrida: backprop == diferencas finitas em " + std::to_string(ptrs.size()) + " parametros (erro " + std::to_string(worst) + ")");
    (void)B;
}

static void test_training_circle() {
    // Classe 1 dentro do circulo x^2 + y^2 < 0.45 (nao linearmente separavel)
    const int side = 8, N = side * side;
    GhostMatrix X(2, N), Y(1, N);
    for (int i = 0; i < side; ++i) for (int j = 0; j < side; ++j) {
        const double a = -1 + 2.0 * i / (side - 1), b = -1 + 2.0 * j / (side - 1);
        const int k = i * side + j;
        X(0, k) = GhostNumberG(a, 0, 0); X(1, k) = GhostNumberG(b, 0, 0);
        Y(0, k) = GhostNumberG(a * a + b * b < 0.45 ? 1.0 : 0.0, 0, 0);
    }
    std::mt19937_64 rng(7);
    GhostMatrix W1(2, 2), W2(1, 2);
    randomize(W1, rng, 1.0); randomize(W2, rng, 0.5);
    GhostQLayer Q(2, 2);
    Q.seed(3);
    GhostHybridNetwork net("circulo", {&W1, &Q, &W2});
    const double before = net.loss(X, Y);
    net.optimize(X, Y, 150, 0.05, "adam");
    const double after = net.loss(X, Y), acc = net.accuracy(X, Y);
    check(after < before * 0.5, "treino reduz a perda (" + std::to_string(before) + " -> " + std::to_string(after) + ")");
    check(acc >= 0.9, "rede hibrida separa o circulo (acuracia " + std::to_string(acc) + ")");
    check(net.classify(0.0, 0.0) == 1 && net.classify(0.95, 0.95) == 0, "classify: centro=1, canto=0");
    std::cout << "  circulo: perda " << before << " -> " << after << ", acuracia " << acc * 100 << "%\n";
}

static void test_errors() {
    GhostMatrix W1(3, 2);
    GhostQLayer Q(2, 1);
    bool thrown = false;
    try { GhostHybridNetwork bad("ruim", {&W1, &Q}); } catch (const std::invalid_argument& e) { thrown = std::string(e.what()).find("espera 2 entradas") != std::string::npos; }
    check(thrown, "dimensoes incompativeis geram erro claro");
}

int main() {
    test_qlayer_gradients();
    test_network_gradients();
    test_training_circle();
    test_errors();
    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "Hybrid: OK (gradientes da qlayer, backprop hibrido, treino do circulo, erros).\n";
    return 0;
}
