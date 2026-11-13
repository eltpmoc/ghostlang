// Testes do inverso de circuits (circuit nome_dg): GhostQTape e GhostQCircuit::append_inverse.
// Critério: U seguido de U† devolve exatamente o estado inicial (diferença < 1e-10, fase global inclusive).
#include "GhostQTape.hpp"
#include <cmath>
#include <complex>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FALHOU: " << what << "\n"; ++failures; }
}

static double max_diff(const GhostQReg& a, const GhostQReg& b) {
    double d = 0;
    for (std::size_t i = 0; i < a.state().size(); ++i) d = std::max(d, std::abs(a.state()[i] - b.state()[i]));
    return d;
}

// Grava `count` portas aleatórias de todos os tipos.
static void random_gates(GhostQTape& t, int count, unsigned seed) {
    std::mt19937 rng(seed);
    const int n = t.size();
    std::uniform_int_distribution<int> q(0, n - 1), g(0, 33);
    std::uniform_real_distribution<double> ang(-3.0, 3.0);
    auto two = [&](int& a, int& b) { a = q(rng); do { b = q(rng); } while (b == a); };
    for (int k = 0; k < count; ++k) {
        int a, b, c;
        switch (g(rng)) {
            case 0: t.h(q(rng)); break;
            case 1: t.x(q(rng)); break;
            case 2: t.y(q(rng)); break;
            case 3: t.z(q(rng)); break;
            case 4: t.s(q(rng)); break;
            case 5: t.sdg(q(rng)); break;
            case 6: t.t(q(rng)); break;
            case 7: t.tdg(q(rng)); break;
            case 8: t.sx(q(rng)); break;
            case 9: t.rx(q(rng), ang(rng)); break;
            case 10: t.ry(q(rng), ang(rng)); break;
            case 11: t.rz(q(rng), ang(rng)); break;
            case 12: t.p(q(rng), ang(rng)); break;
            case 13: t.u(q(rng), ang(rng), ang(rng), ang(rng)); break;
            case 14: t.h_all(); break;
            case 15: t.x_all(); break;
            case 16: two(a, b); t.cx(a, b); break;
            case 17: two(a, b); t.cy(a, b); break;
            case 18: two(a, b); t.cz(a, b); break;
            case 19: two(a, b); t.ch(a, b); break;
            case 20: two(a, b); t.swap(a, b); break;
            case 21: two(a, b); t.cp(a, b, ang(rng)); break;
            case 22: two(a, b); t.crx(a, b, ang(rng)); break;
            case 23: two(a, b); t.cry(a, b, ang(rng)); break;
            case 24: two(a, b); t.crz(a, b, ang(rng)); break;
            case 25: two(a, b); t.rzz(a, b, ang(rng)); break;
            case 26: two(a, b); do { c = q(rng); } while (c == a || c == b); t.ccx(a, b, c); break;
            case 27: two(a, b); do { c = q(rng); } while (c == a || c == b); t.cswap(a, b, c); break;
            case 28: t.qft(); break;
            case 29: t.iqft(); break;
            case 30: t.qft_range(1, n - 2); break;
            case 31: t.iqft_range(0, 2); break;
            case 32: t.phase_flip(std::uniform_int_distribution<long long>(0, (1LL << n) - 1)(rng)); break;
            default: t.diffuser(); break;
        }
    }
}

// Mesmo formato do código gerado pelo compilador para circuit corpo(a) { ... } e corpo_dg.
template <class Q> void corpo(Q& q, int a) { q.h(a); q.cx(a, a + 1); q.rz(a + 1, 0.3); q.s(a); q.sx(a + 1); }
template <class Q> void corpo_dg(Q& q, int a) { auto rec = ghost_begin_inverse(q); corpo(rec, a); ghost_end_inverse(q, rec); }
template <class Q> void duplo(Q& q, int a) { corpo(q, a); q.t(a); corpo_dg(q, a); } // inverso dentro de circuit
template <class Q> void duplo_dg(Q& q, int a) { auto rec = ghost_begin_inverse(q); duplo(rec, a); ghost_end_inverse(q, rec); }
// Camada treinável (qcircuit)
template <class Q> void camada(Q& q, int n) {
    for (int i = 0; i < n; ++i) { q.ry_train(i); q.rz_train(i); }
    for (int i = 0; i + 1 < n; ++i) q.cx(i, i + 1);
    q.s(0); q.t(n - 1);
}
template <class Q> void camada_dg(Q& q, int n) { auto rec = ghost_begin_inverse(q); camada(rec, n); ghost_end_inverse(q, rec); }

int main() {
    // 1) 500 portas aleatórias de todos os tipos, 3 sementes: U U† = I
    for (unsigned seed = 1; seed <= 3; ++seed) {
        GhostQReg reg(5);
        reg.seed(seed);
        for (int k = 0; k < 5; ++k) reg.ry(k, 0.3 + 0.2 * k); // estado inicial não trivial
        reg.cx(0, 3);
        const GhostQReg start = reg;
        GhostQTape tape(5);
        random_gates(tape, 500, seed);
        check(tape.num_gates() == 500, "tape grava 500 portas");
        tape.apply(reg);
        check(max_diff(reg, start) > 1e-3, "U muda o estado (seed " + std::to_string(seed) + ")");
        tape.apply_inverse(reg);
        const double d = max_diff(reg, start);
        check(d < 1e-10, "U U+ = I com 500 portas (seed " + std::to_string(seed) + "), diferenca " + std::to_string(d));
    }

    // 2) inverso gravado em outro gravador (inverso aninhado): (U†)† aplicado depois de U† = I
    {
        GhostQTape u(4), udg(4);
        random_gates(u, 200, 7);
        u.apply_inverse(udg);
        GhostQReg reg(4);
        reg.h(0); reg.cx(0, 2);
        const GhostQReg start = reg;
        udg.apply(reg);
        u.apply(reg);
        check(max_diff(reg, start) < 1e-10, "U+ gravado em outro tape, seguido de U = I");
    }

    // 3) formato do código gerado: qreg, aninhado e inverso dentro de circuit
    {
        GhostQReg reg(3);
        reg.ry(0, 1.1); reg.ry(2, -0.4);
        const GhostQReg start = reg;
        corpo(reg, 1);
        corpo_dg(reg, 1);
        check(max_diff(reg, start) < 1e-10, "corpo seguido de corpo_dg = I");
        duplo(reg, 0);
        GhostQReg manual = start; // corpo, T e o inverso de corpo escrito à mão
        corpo(manual, 0);
        manual.t(0);
        manual.sx(1); manual.sx(1); manual.sx(1);
        manual.sdg(0); manual.rz(1, -0.3); manual.cx(0, 1); manual.h(0);
        check(max_diff(reg, manual) < 1e-10, "corpo_dg dentro de circuit = inverso escrito a mao");
        reg = start;
        duplo(reg, 0);
        duplo_dg(reg, 0);
        check(max_diff(reg, start) < 1e-10, "duplo seguido de duplo_dg = I (inverso de um circuit que contem inverso)");
    }

    // 4) medição e reset não são reversíveis
    {
        GhostQTape t(2);
        bool threw = false;
        try { t.measure(0); } catch (const std::runtime_error&) { threw = true; }
        check(threw, "measure em circuit invertido lanca erro");
        threw = false;
        try { t.reset(); } catch (const std::runtime_error&) { threw = true; }
        check(threw, "reset em circuit invertido lanca erro");
        threw = false;
        try { t.cx(0, 5); } catch (const std::out_of_range&) { threw = true; }
        check(threw, "qubit fora do registrador no tape");
        threw = false;
        GhostQReg r3(3);
        try { t.apply_inverse(r3); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "tamanho diferente entre tape e qreg");
    }

    // 5) qcircuit: portas fixas, treináveis, compartilhadas, features e feature map
    {
        GhostQCircuit c(3);
        c.seed(5);
        c.h(0);
        const int shared = c.add_param(0.7);       // parâmetro anterior à gravação: compartilhado
        const int params_before = c.num_params();
        const int gates_before = c.num_gates();
        GhostQCircuit rec = c.inverse_recorder();
        rec.encode_all(0.8);
        rec.ansatz(2);
        rec.s(1); rec.t(2); rec.rx(0, 0.4); rec.rzz(0, 2, -0.9);
        rec.rzz_train(1, 2);
        rec.ry_param(0, shared);
        rec.rx_param_scaled(2, shared, -1.5);
        rec.encode_rzz(0, 1, 0, 1, 0.6);
        rec.zz_feature_map(1, 0.5);
        const int recorded = rec.num_gates() - gates_before;
        const int new_params = rec.num_params() - params_before;

        // c2 = c + U (cópia do gravador) + U†
        GhostQCircuit c2 = rec;
        c2.append_inverse(rec, gates_before, params_before);
        check(c2.num_gates() == rec.num_gates() + recorded + 2 + 6, "numero de portas do inverso (S -> 3, T -> 7)");
        check(c2.num_params() == rec.num_params() + new_params, "parametros novos ganham copia; o compartilhado nao");
        const std::vector<double> x = {0.3, -1.2, 2.0};
        // estado de c (só H no q0 e o parâmetro sem porta) deve ser igual ao de c2
        const GhostQReg a = c.state_vec(x);
        const GhostQReg b = c2.state_vec(x);
        check(max_diff(a, b) < 1e-10, "qcircuit: U U+ = I com features, ansatz, rzz, S, T e parametro compartilhado");
        const GhostQReg u = rec.state_vec(x);
        check(max_diff(a, u) > 1e-3, "qcircuit: U sozinho muda o estado");

        bool threw = false;
        GhostQCircuit other(2);
        try { other.append_inverse(rec, 0, 0); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "qcircuit: inverso com numero de qubits diferente lanca erro");
    }

    // 6) qcircuit pelo formato do código gerado: o gradiente continua funcionando depois do inverso
    {
        GhostQCircuit c(3);
        c.seed(9);
        camada(c, 3);
        const int p1 = c.num_params();
        camada_dg(c, 3);
        check(c.num_params() == 2 * p1, "camada_dg cria parametros proprios");
        // Os parâmetros novos nascem em ordem reversa: o novo k corresponde ao original p1-1-k.
        // Igualando-os aos originais, camada . camada_dg = I.
        for (int k = 0; k < p1; ++k) c.set_param(p1 + k, c.param(p1 - 1 - k));
        const GhostQReg st = c.state_vec({});
        check(std::abs(st.amplitude(0) - std::complex<double>(1.0, 0.0)) < 1e-10, "qcircuit: camada seguida de camada_dg (mesmos theta) volta a |000>");
        c.observable("ZZI", 1.0);
        c.observable("IXI", 0.5);
        const double e0 = c.energy();
        const double e1 = c.minimize(60, 0.1);
        check(e1 < e0 - 1e-3 && e1 < -1.0, "qcircuit com inverso continua treinavel (parameter-shift): " + std::to_string(e0) + " -> " + std::to_string(e1));
    }

    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "circuit_inverse: todos os testes passaram\n";
    return 0;
}
