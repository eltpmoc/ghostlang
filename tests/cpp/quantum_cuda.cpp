// Teste do backend CUDA do GhostQReg: mesmas operações na CPU e na GPU devem dar o mesmo estado.
// Sem backend/GPU, imprime SKIP e passa (o fallback para CPU é o comportamento esperado).
#include "GhostQML.hpp"
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FALHOU: " << what << "\n"; ++failures; }
}

static double max_diff(const GhostQReg& a, const GhostQReg& b) {
    const auto& x = a.state();
    const auto& y = b.state();
    double m = 0;
    for (size_t i = 0; i < x.size(); ++i) m = std::max(m, std::abs(x[i] - y[i]));
    return m;
}

// Aplica a mesma sequência aleatória nos dois registradores
static void random_circuit(GhostQReg& a, GhostQReg& b, int n, int gates, unsigned seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> ang(-PI, PI);
    std::uniform_int_distribution<int> qd(0, n - 1), gd(0, 19);
    for (int k = 0; k < gates; ++k) {
        int q = qd(rng), r = qd(rng), s = qd(rng);
        while (r == q) r = qd(rng);
        while (s == q || s == r) s = qd(rng);
        const double t1 = ang(rng), t2 = ang(rng), t3 = ang(rng);
        std::function<void(GhostQReg&)> op;
        switch (gd(rng)) {
            case 0: op = [=](GhostQReg& g) { g.h(q); }; break;
            case 1: op = [=](GhostQReg& g) { g.x(q); }; break;
            case 2: op = [=](GhostQReg& g) { g.y(q); }; break;
            case 3: op = [=](GhostQReg& g) { g.s(q); }; break;
            case 4: op = [=](GhostQReg& g) { g.t(q); }; break;
            case 5: op = [=](GhostQReg& g) { g.rx(q, t1); }; break;
            case 6: op = [=](GhostQReg& g) { g.ry(q, t1); }; break;
            case 7: op = [=](GhostQReg& g) { g.rz(q, t1); }; break;
            case 8: op = [=](GhostQReg& g) { g.u(q, t1, t2, t3); }; break;
            case 9: op = [=](GhostQReg& g) { g.cx(q, r); }; break;
            case 10: op = [=](GhostQReg& g) { g.cz(q, r); }; break;
            case 11: op = [=](GhostQReg& g) { g.cp(q, r, t1); }; break;
            case 12: op = [=](GhostQReg& g) { g.crx(q, r, t1); }; break;
            case 13: op = [=](GhostQReg& g) { if (n >= 3) g.ccx(q, r, s); }; break;
            case 14: op = [=](GhostQReg& g) { g.swap(q, r); }; break;
            case 15: op = [=](GhostQReg& g) { if (n >= 3) g.cswap(q, r, s); }; break;
            case 16: op = [=](GhostQReg& g) { g.rzz(q, r, t1); }; break;
            case 17: op = [=, &rng](GhostQReg& g) { g.phase_flip(static_cast<long long>((q * 7919 + r) % (1LL << n))); }; break;
            case 18: op = [=](GhostQReg& g) { g.diffuser(); }; break;
            default: op = [=](GhostQReg& g) { g.sx(q); }; break;
        }
        op(a);
        op(b);
    }
}

int main() {
    std::cout << "Backend: " << GhostQReg::backend_info() << "\n";
    if (!GhostQReg::cuda_available()) {
        std::cout << "SKIP: sem backend CUDA; o simulador roda na CPU.\n";
        return 0;
    }

    for (int n : {3, 8, 15}) {
        GhostQReg cpu(n), gpu(n);
        cpu.device("cpu");
        gpu.device("cuda");
        check(cpu.device_name() == "cpu" && gpu.device_name() == "cuda", "device_name");
        random_circuit(cpu, gpu, n, 300, 100 + n);
        cpu.qft(); gpu.qft();
        const double d = max_diff(cpu, gpu);
        check(d < 1e-10, "n=" + std::to_string(n) + ": estado CPU == GPU (diferenca " + std::to_string(d) + ")");
        check(std::abs(cpu.norm() - gpu.norm()) < 1e-10 && std::abs(gpu.norm() - 1.0) < 1e-9, "n=" + std::to_string(n) + ": norma");
        for (int q = 0; q < n; ++q) if (std::abs(cpu.prob(q) - gpu.prob(q)) > 1e-10) { check(false, "prob(q) CPU == GPU"); break; }
        std::mt19937_64 rng(n);
        const std::string letters = "IXYZ";
        for (int k = 0; k < 10; ++k) {
            std::string p(n, 'I');
            for (auto& c : p) c = letters[rng() % 4];
            if (std::abs(cpu.expect(p) - gpu.expect(p)) > 1e-10) { check(false, "expect(" + p + ") CPU == GPU"); break; }
        }
        cpu.seed(77); gpu.seed(77);
        bool same = true;
        for (int q = 0; q < n; ++q) same = same && (cpu.measure(q) == gpu.measure(q));
        check(same && max_diff(cpu, gpu) < 1e-10, "n=" + std::to_string(n) + ": medicoes e colapso iguais com a mesma seed");
    }

    // Cópia, movimento e troca de dispositivo preservam o estado
    {
        GhostQReg g(10);
        g.device("cuda");
        g.h_all(); g.cx(0, 9); g.rz(3, 0.4);
        GhostQReg c = g;              // cópia com estado próprio
        c.x(0);
        GhostQReg ref(10); ref.device("cpu"); ref.h_all(); ref.cx(0, 9); ref.rz(3, 0.4);
        check(max_diff(g, ref) < 1e-12, "copia nao altera o original");
        check(c.device_name() == "cuda", "copia continua na GPU");
        GhostQReg m = std::move(g);
        check(max_diff(m, ref) < 1e-12, "movimento preserva o estado");
        m.device("cpu"); m.ry(2, 1.0); m.device("cuda"); m.ry(2, -1.0);
        check(max_diff(m, ref) < 1e-12, "cpu <-> cuda preserva o estado");
        m.set_basis(5);
        check(std::abs(m.prob_state(5) - 1.0) < 1e-12, "set_basis na GPU");
    }

    // Grover e GHZ grandes na GPU
    {
        GhostQReg g(16);
        g.device("cuda");
        g.h_all();
        const long long marked = 12345;
        const int iters = static_cast<int>(std::floor(PI / 4 * std::sqrt(double(1 << 16))));
        for (int k = 0; k < iters; ++k) { g.phase_flip(marked); g.diffuser(); }
        check(g.prob_state(marked) > 0.99, "Grover(16) na GPU: P=" + std::to_string(g.prob_state(marked)));
        GhostQReg z(22);
        z.device("cuda");
        z.h(0);
        for (int k = 0; k < 21; ++k) z.cx(k, k + 1);
        check(std::abs(z.prob_state(0) - 0.5) < 1e-9 && std::abs(z.prob_state((1LL << 22) - 1) - 0.5) < 1e-9, "GHZ(22) na GPU");
        // GHZ com n par: Z^n|1...1> = (+1)|1...1>, entao <Z...Z> = 1; e <X...X> = 1
        const double zz = z.expect(std::string(22, 'Z')), xx = z.expect(std::string(22, 'X'));
        GhostQReg zc(22);
        zc.device("cpu");
        zc.h(0);
        for (int k = 0; k < 21; ++k) zc.cx(k, k + 1);
        check(std::abs(zz - 1.0) < 1e-9 && std::abs(xx - 1.0) < 1e-9, "GHZ(22): <Z...Z>=1 e <X...X>=1 (GPU: " + std::to_string(zz) + ", " + std::to_string(xx) + ")");
        check(std::abs(zz - zc.expect(std::string(22, 'Z'))) < 1e-10 && std::abs(xx - zc.expect(std::string(22, 'X'))) < 1e-10, "GHZ(22): Pauli GPU == CPU");
    }

    // qcircuit usando a GPU (padrão "cuda")
    {
        GhostQCircuit c(4);
        c.seed(1);
        c.encode_all();
        c.ansatz(2);
        c.observable("ZZXI", 0.5);
        GhostQReg::set_default_device("cpu");
        const double ecpu = c.expect(0.3, 0.1, -0.2, 0.7);
        const auto gcpu = c.gradient_vec({0.3, 0.1, -0.2, 0.7});
        GhostQReg::set_default_device("cuda");
        const double egpu = c.expect(0.3, 0.1, -0.2, 0.7);
        const auto ggpu = c.gradient_vec({0.3, 0.1, -0.2, 0.7});
        GhostQReg::set_default_device("auto");
        double gd = 0;
        for (size_t i = 0; i < gcpu.size(); ++i) gd = std::max(gd, std::abs(gcpu[i] - ggpu[i]));
        check(std::abs(ecpu - egpu) < 1e-10 && gd < 1e-10, "qcircuit: valor esperado e gradiente iguais em CPU e GPU");
    }

    // Benchmark (informativo)
    for (int n : {20, 24}) {
        auto bench = [n](const char* dev) {
            GhostQReg g(n);
            g.device(dev);
            auto t0 = std::chrono::steady_clock::now();
            for (int layer = 0; layer < 10; ++layer) {
                for (int q = 0; q < n; ++q) g.ry(q, 0.1 * (q + 1));
                for (int q = 0; q + 1 < n; ++q) g.cx(q, q + 1);
            }
            volatile double p = g.prob(0); // força a sincronização
            (void)p;
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        };
        const double tc = bench("cpu"), tg = bench("cuda");
        std::cout << "  " << n << " qubits, " << 10 * (2 * n - 1) << " portas: CPU " << tc << " s | GPU " << tg
                  << " s | aceleracao " << tc / tg << "x\n";
    }

    if (failures) { std::cerr << failures << " falha(s)\n"; return 1; }
    std::cout << "Quantum CUDA: OK (CPU == GPU em portas, medicao, Pauli, copia, Grover/GHZ, qcircuit).\n";
    return 0;
}
