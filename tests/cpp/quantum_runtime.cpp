// Testes do simulador statevector GhostQReg (F3).
// Verifica física, não só compilação: normas, inversas, emaranhamento, QFT, Grover e medição.
#include "GhostQuantum.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using amp = GhostQReg::amp;
static int failures = 0;

static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FALHOU: " << what << "\n"; ++failures; }
}
static bool near(double a, double b, double eps = 1e-9) { return std::abs(a - b) < eps; }
static bool near(amp a, amp b, double eps = 1e-9) { return std::abs(a - b) < eps; }

static double fidelity(const std::vector<amp>& a, const std::vector<amp>& b) {
    amp s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) s += std::conj(a[i]) * b[i];
    return std::norm(s);
}

static void test_basic_gates() {
    GhostQReg q(1);
    q.x(0);
    check(near(q.amplitude(1), amp(1, 0)), "X|0> = |1>");
    q.reset(); q.h(0);
    check(near(q.amplitude(0), amp(1 / std::sqrt(2.0), 0)) && near(q.amplitude(1), amp(1 / std::sqrt(2.0), 0)), "H|0> = |+>");
    q.reset(); q.ry(0, PI);
    check(near(q.prob(0), 1.0), "RY(pi)|0> = |1>");
    q.reset(); q.rx(0, PI / 2);
    check(near(q.prob(0), 0.5), "RX(pi/2) da 50%");
    q.reset(); q.h(0); q.s(0); q.s(0); q.h(0);
    check(near(q.prob(0), 1.0), "H S S H = X (fase -1 em |1>)");
    q.reset(); q.h(0); q.t(0); q.tdg(0); q.h(0);
    check(near(q.prob(0), 0.0), "T Tdg = I");
    q.reset(); q.sx(0); q.sx(0);
    check(near(q.prob(0), 1.0), "SX SX = X");
    q.reset(); q.y(0);
    check(near(q.amplitude(1), amp(0, 1)), "Y|0> = i|1>");
}

static void test_bell_ghz() {
    GhostQReg q(2);
    q.h(0); q.cx(0, 1);
    const double r = 1 / std::sqrt(2.0);
    check(near(q.amplitude(0), amp(r, 0)) && near(q.amplitude(3), amp(r, 0)), "Bell: amplitudes |00> e |11>");
    check(near(q.amplitude(1), amp(0, 0)) && near(q.amplitude(2), amp(0, 0)), "Bell: sem |01> e |10>");
    check(q.ket() == "0.7071|00> + 0.7071|11>", "Bell: ket = " + q.ket());
    check(near(q.expect("ZZ"), 1.0) && near(q.expect("XX"), 1.0) && near(q.expect("YY"), -1.0), "Bell: <ZZ>=1, <XX>=1, <YY>=-1");
    check(near(q.expect_z(0), 0.0), "Bell: <Z0> = 0");

    for (int s = 0; s < 50; ++s) {
        GhostQReg b(2); b.seed(1000 + s); b.h(0); b.cx(0, 1);
        const int m0 = b.measure(0), m1 = b.measure(1);
        check(m0 == m1, "Bell: medicoes correlacionadas");
        check(near(b.norm(), 1.0), "Bell: norma 1 apos colapso");
    }

    GhostQReg g(5); g.seed(7);
    g.h(0); for (int k = 0; k < 4; ++k) g.cx(k, k + 1);
    check(near(g.prob_state(0), 0.5) && near(g.prob_state(31), 0.5), "GHZ(5): 50% |00000> e 50% |11111>");
    const auto c = g.counts(4000);
    check(c.size() == 2 && c.count("00000") && c.count("11111"), "GHZ(5): so dois resultados");
    const int zeros = c.count("00000") ? c.at("00000") : 0;
    check(zeros > 1800 && zeros < 2200, "GHZ(5): contagens perto de 50% (" + std::to_string(zeros) + ")");
}

static void test_controlled_and_swap() {
    GhostQReg q(3);
    q.x(0); q.x(1); q.ccx(0, 1, 2);
    check(near(q.prob_state(7), 1.0), "Toffoli com controles em 1");
    q.reset(); q.x(0); q.ccx(0, 1, 2);
    check(near(q.prob_state(1), 1.0), "Toffoli com um controle em 0 nao age");
    q.reset(); q.x(0); q.swap(0, 2);
    check(near(q.prob_state(4), 1.0), "SWAP q0<->q2");
    q.reset(); q.x(2); q.x(0); q.cswap(2, 0, 1);
    check(near(q.prob_state(6), 1.0), "CSWAP com controle em 1");
    q.reset(); q.h(0); q.h(1); q.cz(0, 1);
    check(near(q.amplitude(3), amp(-0.5, 0)), "CZ inverte a fase de |11>");
    q.reset(); q.x(1); q.cp(1, 0, PI / 2); q.x(0); q.cp(1, 0, PI / 2);
    check(near(q.amplitude(3), amp(0, 1)), "CP(pi/2) aplica fase i em |11>");
}

// Sequência aleatória seguida da inversa deve voltar ao estado inicial (portas unitárias e inversas corretas).
static void test_unitarity() {
    const int n = 6;
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> ang(-PI, PI);
    std::uniform_int_distribution<int> qd(0, n - 1), gd(0, 15);

    GhostQReg q(n);
    for (int k = 0; k < 30; ++k) { q.u(qd(rng), ang(rng), ang(rng), ang(rng)); int a = qd(rng), b = qd(rng); if (a != b) q.cx(a, b); }
    const std::vector<amp> start = q.state();
    check(near(q.norm(), 1.0, 1e-12), "estado inicial normalizado");

    std::vector<std::function<void()>> undo;
    for (int k = 0; k < 400; ++k) {
        int a = qd(rng), b = qd(rng), c = qd(rng);
        while (b == a) b = qd(rng);
        while (c == a || c == b) c = qd(rng);
        const double th = ang(rng), ph = ang(rng), la = ang(rng);
        switch (gd(rng)) {
            case 0: q.h(a); undo.push_back([&q, a] { q.h(a); }); break;
            case 1: q.x(a); undo.push_back([&q, a] { q.x(a); }); break;
            case 2: q.y(a); undo.push_back([&q, a] { q.y(a); }); break;
            case 3: q.s(a); undo.push_back([&q, a] { q.sdg(a); }); break;
            case 4: q.t(a); undo.push_back([&q, a] { q.tdg(a); }); break;
            case 5: q.rx(a, th); undo.push_back([&q, a, th] { q.rx(a, -th); }); break;
            case 6: q.ry(a, th); undo.push_back([&q, a, th] { q.ry(a, -th); }); break;
            case 7: q.rz(a, th); undo.push_back([&q, a, th] { q.rz(a, -th); }); break;
            case 8: q.u(a, th, ph, la); undo.push_back([&q, a, th, ph, la] { q.u(a, -th, -la, -ph); }); break;
            case 9: q.cx(a, b); undo.push_back([&q, a, b] { q.cx(a, b); }); break;
            case 10: q.cp(a, b, th); undo.push_back([&q, a, b, th] { q.cp(a, b, -th); }); break;
            case 11: q.crx(a, b, th); undo.push_back([&q, a, b, th] { q.crx(a, b, -th); }); break;
            case 12: q.ccx(a, b, c); undo.push_back([&q, a, b, c] { q.ccx(a, b, c); }); break;
            case 13: q.swap(a, b); undo.push_back([&q, a, b] { q.swap(a, b); }); break;
            case 14: q.cswap(a, b, c); undo.push_back([&q, a, b, c] { q.cswap(a, b, c); }); break;
            default: q.sx(a); undo.push_back([&q, a] { q.sx(a); q.sx(a); q.sx(a); }); break;
        }
        if (!near(q.norm(), 1.0, 1e-10)) { check(false, "norma preservada em toda porta"); break; }
    }
    for (auto it = undo.rbegin(); it != undo.rend(); ++it) (*it)();
    check(fidelity(start, q.state()) > 1 - 1e-10, "sequencia + inversa = identidade (fidelidade)");
}

static void test_qft() {
    const int n = 4, N = 1 << n, xval = 5;
    GhostQReg q(n);
    q.set_basis(xval);
    q.qft();
    bool ok = true;
    for (int y = 0; y < N; ++y) {
        const amp expected = std::polar(1.0 / std::sqrt(double(N)), 2 * PI * xval * y / N);
        if (!near(q.amplitude(y), expected, 1e-10)) ok = false;
    }
    check(ok, "QFT|5> = sum_y e^{2 pi i 5y/16}|y>/4");
    q.iqft();
    check(near(q.prob_state(xval), 1.0, 1e-10), "IQFT(QFT|5>) = |5>");

    GhostQReg r(5);
    r.set_basis(9); r.qft_range(1, 3); r.iqft_range(1, 3);
    check(near(r.prob_state(9), 1.0, 1e-10), "QFT/IQFT em sub-intervalo");
}

static void test_grover() {
    const int n = 4; const long long marked = 11;
    GhostQReg q(n);
    q.h_all();
    const int iters = static_cast<int>(std::floor(PI / 4 * std::sqrt(double(1 << n))));
    for (int k = 0; k < iters; ++k) { q.phase_flip(marked); q.diffuser(); }
    check(q.prob_state(marked) > 0.95, "Grover(4 qubits, 3 iteracoes) encontra |1011> com P=" + std::to_string(q.prob_state(marked)));
    check(near(q.norm(), 1.0, 1e-12), "Grover preserva a norma");
}

static void test_reproducible_measurement() {
    auto run = [] { GhostQReg q(3); q.seed(123); q.h_all(); return q.counts(500); };
    check(run() == run(), "mesma seed -> mesmas contagens");
    GhostQReg q(3); q.seed(5); q.h_all();
    const long long r = q.measure_all();
    check(near(q.prob_state(r), 1.0), "measure_all colapsa para o resultado");
}

static void test_errors_and_draw() {
    bool thrown = false;
    try { GhostQReg q(2); q.h(2); } catch (const std::out_of_range&) { thrown = true; }
    check(thrown, "qubit fora do registrador gera erro");
    thrown = false;
    try { GhostQReg q(2); q.cx(1, 1); } catch (const std::invalid_argument&) { thrown = true; }
    check(thrown, "controle == alvo gera erro");
    thrown = false;
    try { GhostQReg q(40); } catch (const std::invalid_argument&) { thrown = true; }
    check(thrown, "limite de qubits");

    GhostQReg q(3); q.h(0); q.cx(0, 2); q.measure(1);
    const std::string d = q.draw();
    check(d.find("H") != std::string::npos && d.find("*") != std::string::npos && d.find("+") != std::string::npos && d.find("M") != std::string::npos,
          "draw mostra H, controle, cruzamento e medicao");
}

static void test_large_register() {
    GhostQReg q(20);
    q.h(0);
    for (int k = 0; k < 19; ++k) q.cx(k, k + 1);
    check(near(q.prob_state(0), 0.5, 1e-9) && near(q.prob_state((1LL << 20) - 1), 0.5, 1e-9), "GHZ(20) com OpenMP");
}

int main() {
    test_basic_gates();
    test_bell_ghz();
    test_controlled_and_swap();
    test_unitarity();
    test_qft();
    test_grover();
    test_reproducible_measurement();
    test_errors_and_draw();
    test_large_register();
    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "Quantum runtime: OK (portas, Bell/GHZ, unitariedade, QFT, Grover, medicao, draw, 20 qubits).\n";
    return 0;
}
