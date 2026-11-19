// Teste dedicado do OpenMP no runtime (recurso gpu-openmp):
//  1. o runtime foi compilado com OpenMP e usa mais de uma thread;
//  2. o dgemm nativo (cache-blocked + OpenMP) dá o mesmo resultado que o laço serial;
//  3. o mesmo dgemm com 1 thread e com todas as threads dá resultados iguais.
#include "GhostBLAS.hpp"

#include <omp.h>

#include <cmath>
#include <cstdio>
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <vector>

using GhostAlgebra::GhostBLAS;

static int falhas = 0;
static void confere(bool ok, const char* o_que) {
    std::printf("%s %s\n", ok ? "[ok]   " : "[FALHA]", o_que);
    if (!ok) ++falhas;
}

int main() {
#ifndef _OPENMP
    std::printf("[FALHA] compilado sem OpenMP\n");
    return 1;
#else
    const int maxThreads = omp_get_max_threads();
    std::printf("omp_get_max_threads() = %d, nucleos = %u\n", maxThreads, std::thread::hardware_concurrency());

    std::set<int> ids;
    std::mutex m;
    #pragma omp parallel
    {
        std::lock_guard<std::mutex> g(m);
        ids.insert(omp_get_thread_num());
    }
    if (std::thread::hardware_concurrency() > 1)
        confere(ids.size() > 1, "regiao paralela usa mais de uma thread");
    else
        std::printf("[aviso] maquina com 1 nucleo: paralelismo nao verificavel\n");

    const int n = 301; // não múltiplo do bloco 32
    std::mt19937_64 g(42);
    std::uniform_real_distribution<double> u(-1, 1);
    std::vector<double> A(n * n), B(n * n), Cpar(n * n), Cum(n * n), Cser(n * n, 0.0);
    for (auto& x : A) x = u(g);
    for (auto& x : B) x = u(g);

    for (int i = 0; i < n; ++i)
        for (int p = 0; p < n; ++p)
            for (int j = 0; j < n; ++j)
                Cser[i * n + j] += A[i * n + p] * B[p * n + j];

    GhostBLAS::dgemm_native(n, n, n, 1.0, A.data(), n, B.data(), n, 0.0, Cpar.data(), n);
    double dif = 0;
    for (int i = 0; i < n * n; ++i) dif = std::max(dif, std::fabs(Cpar[i] - Cser[i]));
    std::printf("diferenca maxima paralelo x serial: %.3e\n", dif);
    confere(dif < 1e-9, "dgemm nativo paralelo = laco serial");

    omp_set_num_threads(1);
    GhostBLAS::dgemm_native(n, n, n, 1.0, A.data(), n, B.data(), n, 0.0, Cum.data(), n);
    omp_set_num_threads(maxThreads);
    double dif1 = 0;
    for (int i = 0; i < n * n; ++i) dif1 = std::max(dif1, std::fabs(Cpar[i] - Cum[i]));
    confere(dif1 == 0.0, "1 thread e todas as threads dao o mesmo resultado");

    std::printf(falhas ? "OpenMP: %d falha(s)\n" : "OpenMP: tudo certo\n", falhas);
    return falhas ? 1 : 0;
#endif
}
