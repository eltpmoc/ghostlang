#pragma once

// Backend de multiplicação de matrizes (dgemm) dos tensores N-dimensionais.
//
// A implementação fica em src/GhostBLAS.cpp, dentro da libghost_runtime.a, para que
// o programa gerado e o REPL usem o mesmo caminho. Com a opção CMake GHOST_USE_BLAS=ON
// o runtime é compilado com GHOST_HAS_BLAS e chama cblas_dgemm do OpenBLAS (que fica
// embutido na própria libghost_runtime.a); sem ela, roda o laço nativo cache-blocked
// com OpenMP. blas_backend() diz qual dos dois está ativo.

namespace GhostAlgebra {

class GhostBLAS {
public:
    static bool has_blas_hardware();
    static const char* backend_name();

    // dgemm: C = alpha * A * B + beta * C (row-major, sem transposição)
    static void dgemm(int m, int n, int k, double alpha,
                      const double* A, int lda,
                      const double* B, int ldb,
                      double beta, double* C, int ldc);

    // Sempre o laço nativo, mesmo com BLAS ligado (usado para comparar nos testes)
    static void dgemm_native(int m, int n, int k, double alpha,
                             const double* A, int lda,
                             const double* B, int ldb,
                             double beta, double* C, int ldc);
};

} // namespace GhostAlgebra

// Funções para a GhostLang (compilador e REPL)
bool has_blas_hardware();
const char* blas_backend();
