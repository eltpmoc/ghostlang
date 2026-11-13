#include "GhostBLAS.hpp"

#include <algorithm>

#ifdef GHOST_HAS_BLAS
// Protótipo C-BLAS padrão. Não inclui cblas.h para não depender do caminho de
// instalação: os valores dos enums são fixos no padrão CBLAS.
extern "C" void cblas_dgemm(int Order, int TransA, int TransB, int M, int N, int K,
                            double alpha, const double* A, int lda, const double* B, int ldb,
                            double beta, double* C, int ldc);
namespace {
constexpr int CblasRowMajor = 101;
constexpr int CblasNoTrans = 111;
}
#endif

namespace GhostAlgebra {

bool GhostBLAS::has_blas_hardware() {
#ifdef GHOST_HAS_BLAS
    return true;
#else
    return false;
#endif
}

const char* GhostBLAS::backend_name() {
#ifdef GHOST_HAS_BLAS
    return "OpenBLAS";
#else
    return "Nativo (cache-blocked OpenMP)";
#endif
}

void GhostBLAS::dgemm(int m, int n, int k, double alpha,
                      const double* A, int lda,
                      const double* B, int ldb,
                      double beta, double* C, int ldc) {
#ifdef GHOST_HAS_BLAS
    if (m > 0 && n > 0 && k > 0) {
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
        return;
    }
#endif
    dgemm_native(m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
}

void GhostBLAS::dgemm_native(int m, int n, int k, double alpha,
                             const double* A, int lda,
                             const double* B, int ldb,
                             double beta, double* C, int ldc) {
    // Cache-blocked GEMM com OpenMP (bloco 32 para caber no cache L1)
    const int BLOCK = 32;

    // beta escala cada linha de C (respeita ldc)
    for (int i = 0; i < m; ++i) {
        double* row = C + static_cast<long long>(i) * ldc;
        if (beta == 0.0) std::fill(row, row + n, 0.0);
        else if (beta != 1.0) for (int j = 0; j < n; ++j) row[j] *= beta;
    }
    if (k <= 0) return;

    #pragma omp parallel for collapse(2) if(static_cast<long long>(m) * n * k > 4096)
    for (int bi = 0; bi < m; bi += BLOCK) {
        for (int bj = 0; bj < n; bj += BLOCK) {
            const int imax = std::min(bi + BLOCK, m);
            const int jmax = std::min(bj + BLOCK, n);
            for (int bk = 0; bk < k; bk += BLOCK) {
                const int kmax = std::min(bk + BLOCK, k);
                for (int i = bi; i < imax; ++i) {
                    for (int p = bk; p < kmax; ++p) {
                        const double a_ip = alpha * A[static_cast<long long>(i) * lda + p];
                        for (int j = bj; j < jmax; ++j)
                            C[static_cast<long long>(i) * ldc + j] += a_ip * B[static_cast<long long>(p) * ldb + j];
                    }
                }
            }
        }
    }
}

} // namespace GhostAlgebra

bool has_blas_hardware() { return GhostAlgebra::GhostBLAS::has_blas_hardware(); }
const char* blas_backend() { return GhostAlgebra::GhostBLAS::backend_name(); }
