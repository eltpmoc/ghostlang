// Testes do circuit controlado (circuit nome_ctrl): GhostQTape::apply_controlled e a máscara de
// controle do GhostQReg (CPU e, se houver, GPU).
// Referência: para U que não toca o controle c, controlado-U deixa as amplitudes com bit c = 0 e
// aplica U nas de bit c = 1:  out[i] = bit_c(i) ? (U psi)[i] : psi[i].
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

static double max_diff(const std::vector<GhostQReg::amp>& a, const std::vector<GhostQReg::amp>& b) {
    double d = 0;
    for (std::size_t i = 0; i < a.size(); ++i) d = std::max(d, std::abs(a[i] - b[i]));
    return d;
}

// Portas aleatórias (todas as que podem ser controladas) só nos qubits [0, m).
static void random_gates(GhostQTape& t, int m, int count, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> q(0, m - 1), g(0, 27);
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
            case 14: two(a, b); t.cx(a, b); break;
            case 15: two(a, b); t.cy(a, b); break;
            case 16: two(a, b); t.cz(a, b); break;
            case 17: two(a, b); t.ch(a, b); break;
            case 18: two(a, b); t.swap(a, b); break;
            case 19: two(a, b); t.cp(a, b, ang(rng)); break;
            case 20: two(a, b); t.crx(a, b, ang(rng)); break;
            case 21: two(a, b); t.cry(a, b, ang(rng)); break;
            case 22: two(a, b); t.crz(a, b, ang(rng)); break;
            case 23: two(a, b); t.rzz(a, b, ang(rng)); break;
            case 24: two(a, b); do { c = q(rng); } while (c == a || c == b); t.ccx(a, b, c); break;
            case 25: two(a, b); do { c = q(rng); } while (c == a || c == b); t.cswap(a, b, c); break;
            case 26: t.qft_range(0, m); break;
            default: t.iqft_range(1, m - 1); break;
        }
    }
}

// Estado inicial genérico (todas as amplitudes não nulas).
static void prepare(GhostQReg& r, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> ang(0.2, 2.9);
    for (int q = 0; q < r.size(); ++q) { r.ry(q, ang(rng)); r.rz(q, ang(rng)); }
    for (int q = 0; q + 1 < r.size(); ++q) r.cx(q, q + 1);
}

int main() {
    // 1) controle simples, 400 portas aleatórias, 3 sementes (CPU)
    for (unsigned seed = 1; seed <= 3; ++seed) {
        const int n = 6, control = 5;
        GhostQReg reg(n);
        reg.device("cpu");
        prepare(reg, seed);
        GhostQReg withU = reg;
        GhostQTape u(n);
        random_gates(u, n - 1, 400, seed);
        u.apply(withU);
        const auto psi = reg.state();
        const auto upsi = withU.state();
        u.apply_controlled(reg, control);
        std::vector<GhostQReg::amp> expected(psi.size());
        for (std::size_t i = 0; i < psi.size(); ++i) expected[i] = ((i >> control) & 1) ? upsi[i] : psi[i];
        const double d = max_diff(reg.state(), expected);
        check(d < 1e-10, "controlado-U = referencia (seed " + std::to_string(seed) + "), diferenca " + std::to_string(d));
        check(reg.control_mask() == 0, "mascara volta a zero depois de aplicar");
    }

    // 2) dois controles (controlado dentro de controlado) + inverso controlado
    {
        const int n = 6;
        GhostQReg reg(n);
        reg.device("cpu");
        prepare(reg, 11);
        GhostQTape u(n);
        random_gates(u, 4, 150, 11);
        GhostQTape outer(n);
        u.apply_controlled(outer, 4); // grava U controlado por q4
        GhostQReg withU = reg;
        u.apply(withU);
        const auto psi = reg.state();
        const auto upsi = withU.state();
        outer.apply_controlled(reg, 5); // controlado por q5 e q4
        std::vector<GhostQReg::amp> expected(psi.size());
        for (std::size_t i = 0; i < psi.size(); ++i) expected[i] = (((i >> 4) & 1) && ((i >> 5) & 1)) ? upsi[i] : psi[i];
        check(max_diff(reg.state(), expected) < 1e-10, "dois controles (q4 e q5) = referencia");

        // inverso do controlado = controlado do inverso: desfaz
        GhostQTape inv(n);
        outer.apply_inverse(inv);
        inv.apply_controlled(reg, 5);
        check(max_diff(reg.state(), psi) < 1e-10, "controlado(U+) desfaz controlado(U)");
    }

    // 3) formato do código gerado (nome_ctrl e nome_dg_ctrl)
    {
        auto corpo = [](auto& q, int a) { q.h(a); q.cx(a, a + 1); q.rz(a + 1, 0.7); q.sx(a); };
        GhostQReg reg(3);
        reg.device("cpu");
        reg.h(2);           // controle em superposição
        reg.ry(0, 0.4);
        const auto psi = reg.state();
        {
            auto rec = ghost_begin_record(reg);
            corpo(rec, 0);
            ghost_end_controlled(reg, rec, 2);
        }
        {
            auto rec = ghost_begin_record(reg);
            {
                auto r2 = ghost_begin_inverse(rec);
                corpo(r2, 0);
                ghost_end_inverse(rec, r2);
            }
            ghost_end_controlled(reg, rec, 2.0); // controle pode vir como double (parâmetro sem tipo)
        }
        check(max_diff(reg.state(), psi) < 1e-10, "corpo_ctrl seguido de corpo_dg_ctrl = I");
        const std::string d = reg.draw();
        check(d.find('*') != std::string::npos, "draw mostra o controle extra");
    }

    // 4) erros: tudo validado antes, registrador intacto
    {
        GhostQReg reg(3);
        reg.device("cpu");
        reg.h(0); reg.h(2);
        const auto psi = reg.state();
        auto expect_error = [&](GhostQTape& t, int control, const std::string& what) {
            bool threw = false;
            try { t.apply_controlled(reg, control); } catch (const std::exception&) { threw = true; }
            check(threw, what + " lanca erro");
            check(max_diff(reg.state(), psi) < 1e-15, what + ": registrador nao muda");
            check(reg.control_mask() == 0, what + ": mascara restaurada");
        };
        GhostQTape a(3); a.x(0); a.h(2);
        expect_error(a, 2, "porta no qubit de controle (depois de outras portas)");
        GhostQTape b(3); b.x(0); b.h_all();
        expect_error(b, 2, "h_all controlado");
        GhostQTape c(3); c.qft();
        expect_error(c, 1, "qft() controlado");
        GhostQTape d(3); d.diffuser();
        expect_error(d, 0, "diffuser controlado");
        GhostQTape e(3); e.x(0);
        expect_error(e, 7, "controle fora do registrador");
        GhostQTape f(3); f.cx(1, 0);
        expect_error(f, 1, "controle usado como controle de cx");

        GhostQReg r2(2);
        r2.set_control_mask(1);
        bool threw = false;
        try { r2.measure(1); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "measure com mascara de controle lanca erro");
        r2.set_control_mask(0);

        GhostQCircuit qc(2);
        threw = false;
        try { auto rec = ghost_begin_record(qc); rec.h(0); ghost_end_controlled(qc, rec, 1); } catch (const std::runtime_error&) { threw = true; }
        check(threw, "qcircuit controlado lanca erro claro");
    }

    // 5) GPU (se houver): mesmo resultado da CPU com 15 qubits
    if (GhostQReg::cuda_available()) {
        const int n = 15, control = 14;
        GhostQReg cpu(n), gpu(n);
        cpu.device("cpu");
        gpu.device("cuda");
        prepare(cpu, 21);
        prepare(gpu, 21);
        GhostQTape u(n);
        random_gates(u, n - 2, 300, 21); // q13 e q14 sao controles
        GhostQTape outer(n);
        u.apply_controlled(outer, 13);
        outer.apply_controlled(cpu, control);
        outer.apply_controlled(gpu, control);
        u.apply_controlled(cpu, control);
        u.apply_controlled(gpu, control);
        check(max_diff(cpu.state(), gpu.state()) < 1e-10, "GPU = CPU com controles extras (15 qubits)");
        std::cout << "GPU: comparada\n";
    } else {
        std::cout << "GPU: SKIP (sem backend CUDA)\n";
    }

    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "circuit_control: todos os testes passaram\n";
    return 0;
}
