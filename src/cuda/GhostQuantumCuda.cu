// GhostQuantumCuda.cu - Backend CUDA do simulador statevector (GhostQReg).
//
// Compilado como biblioteca dinâmica com API em C puro (ghost_quantum_cuda.dll / libghost_quantum_cuda.so),
// carregada em tempo de execução pelo runtime. Assim o runtime pode ser compilado com MinGW/g++ e
// o backend com nvcc + MSVC, sem misturar ABIs de C++. Sem a biblioteca ou sem GPU, tudo roda na CPU.
//
// Estado: vetor de 2^n amplitudes double2 (re, im) na GPU. Qubit 0 = bit menos significativo.
// Build Windows: scripts/build/build_quantum_cuda.ps1
#include <cuda_runtime.h>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#define GQC_API extern "C" __declspec(dllexport)
#else
#define GQC_API extern "C" __attribute__((visibility("default")))
#endif

typedef unsigned long long u64;

namespace {

struct Reg {
    double2* psi = nullptr;
    double* acc = nullptr; // 2 doubles para reduções
    u64 dim = 0;
    int n = 0;
};

thread_local std::string g_error;

bool ok(cudaError_t e, const char* where) {
    if (e == cudaSuccess) return true;
    g_error = std::string(where) + ": " + cudaGetErrorString(e);
    return false;
}

inline int blocks_for(u64 work) {
    u64 b = (work + 255) / 256;
    if (b > 4096) b = 4096;
    if (b < 1) b = 1;
    return static_cast<int>(b);
}

__device__ inline double2 cmul(double2 a, double2 b) { return make_double2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }
__device__ inline double2 cadd(double2 a, double2 b) { return make_double2(a.x + b.x, a.y + b.y); }

// Insere um bit 0 na posição q (enumera os pares |..0..>, |..1..>)
__device__ inline u64 insert_zero(u64 i, int q) { return ((i >> q) << (q + 1)) | (i & ((1ULL << q) - 1ULL)); }

__global__ void k_zero(double2* psi, u64 dim, u64 basis) {
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x)
        psi[i] = make_double2(i == basis ? 1.0 : 0.0, 0.0);
}

__global__ void k_apply_1q(double2* psi, u64 half, u64 cmask, int q, double2 m00, double2 m01, double2 m10, double2 m11) {
    const u64 bit = 1ULL << q;
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < half; i += (u64)gridDim.x * blockDim.x) {
        const u64 i0 = insert_zero(i, q);
        if ((i0 & cmask) != cmask) continue;
        const u64 i1 = i0 | bit;
        const double2 a = psi[i0], b = psi[i1];
        psi[i0] = cadd(cmul(m00, a), cmul(m01, b));
        psi[i1] = cadd(cmul(m10, a), cmul(m11, b));
    }
}

__global__ void k_swap(double2* psi, u64 dim, u64 cmask, u64 ma, u64 mb) {
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x) {
        if ((i & cmask) == cmask && (i & ma) && !(i & mb)) {
            const u64 j = i ^ (ma | mb);
            const double2 t = psi[i]; psi[i] = psi[j]; psi[j] = t;
        }
    }
}

__global__ void k_rzz(double2* psi, u64 dim, int a, int b, double2 same, double2 diff) {
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x) {
        const bool ba = (i >> a) & 1ULL, bb = (i >> b) & 1ULL;
        psi[i] = cmul(psi[i], ba == bb ? same : diff);
    }
}

__global__ void k_flip(double2* psi, u64 basis) { psi[basis] = make_double2(-psi[basis].x, -psi[basis].y); }

// Reduções: soma em shared memory e atomicAdd(double) (sm_60+).
__global__ void k_sum_complex(const double2* psi, u64 dim, double* acc) {
    __shared__ double sr[256], si[256];
    double r = 0, im = 0;
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x) { r += psi[i].x; im += psi[i].y; }
    sr[threadIdx.x] = r; si[threadIdx.x] = im;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) { sr[threadIdx.x] += sr[threadIdx.x + s]; si[threadIdx.x] += si[threadIdx.x + s]; }
        __syncthreads();
    }
    if (threadIdx.x == 0) { atomicAdd(&acc[0], sr[0]); atomicAdd(&acc[1], si[0]); }
}

__global__ void k_reflect(double2* psi, u64 dim, double2 twice_mean) {
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x)
        psi[i] = make_double2(twice_mean.x - psi[i].x, twice_mean.y - psi[i].y);
}

// mask == 0: soma |psi|^2 de todos; senão só dos índices com o bit em 1
__global__ void k_norm2(const double2* psi, u64 dim, u64 mask, double* acc) {
    __shared__ double s[256];
    double v = 0;
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x)
        if (mask == 0 || (i & mask)) v += psi[i].x * psi[i].x + psi[i].y * psi[i].y;
    s[threadIdx.x] = v;
    __syncthreads();
    for (int k = blockDim.x / 2; k > 0; k >>= 1) { if (threadIdx.x < k) s[threadIdx.x] += s[threadIdx.x + k]; __syncthreads(); }
    if (threadIdx.x == 0) atomicAdd(&acc[0], s[0]);
}

__global__ void k_collapse(double2* psi, u64 dim, u64 bit, int outcome, double scale) {
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x) {
        const bool set = (i & bit) != 0;
        psi[i] = (set == (outcome == 1)) ? make_double2(psi[i].x * scale, psi[i].y * scale) : make_double2(0.0, 0.0);
    }
}

// <psi|P|psi> para P = produto de Paulis: P|j> = i^{nY} (-1)^{popcount(j & (ymask|zmask))} |j ^ xmask>
__global__ void k_pauli(const double2* psi, u64 dim, u64 xmask, u64 zymask, double2 iny, double* acc) {
    __shared__ double s[256];
    double v = 0;
    for (u64 i = blockIdx.x * (u64)blockDim.x + threadIdx.x; i < dim; i += (u64)gridDim.x * blockDim.x) {
        const u64 j = i ^ xmask;
        const double sign = (__popcll(j & zymask) & 1) ? -1.0 : 1.0;
        const double2 c = make_double2(iny.x * sign, iny.y * sign);
        const double2 pj = cmul(c, psi[j]);
        v += psi[i].x * pj.x + psi[i].y * pj.y; // Re(conj(psi_i) * (P psi)_i)
    }
    s[threadIdx.x] = v;
    __syncthreads();
    for (int k = blockDim.x / 2; k > 0; k >>= 1) { if (threadIdx.x < k) s[threadIdx.x] += s[threadIdx.x + k]; __syncthreads(); }
    if (threadIdx.x == 0) atomicAdd(&acc[0], s[0]);
}

double read_acc(Reg* r, int count, double* out) {
    double h[2] = {0, 0};
    if (!ok(cudaMemcpy(h, r->acc, count * sizeof(double), cudaMemcpyDeviceToHost), "reducao")) return 0.0;
    if (out) { out[0] = h[0]; if (count > 1) out[1] = h[1]; }
    return h[0];
}

bool clear_acc(Reg* r) { return ok(cudaMemset(r->acc, 0, 2 * sizeof(double)), "reducao"); }

} // namespace

GQC_API int gqc_version() { return 1; }

GQC_API const char* gqc_last_error() { return g_error.c_str(); }

// 1 se há GPU utilizável; escreve nome, memória e capacidade em info.
GQC_API int gqc_device_info(char* info, int len) {
    int count = 0;
    cudaError_t e = cudaGetDeviceCount(&count);
    if (e != cudaSuccess || count == 0) {
        if (info && len > 0) std::snprintf(info, len, "sem GPU CUDA (%s)", e != cudaSuccess ? cudaGetErrorString(e) : "nenhum dispositivo");
        return 0;
    }
    cudaDeviceProp p;
    if (cudaGetDeviceProperties(&p, 0) != cudaSuccess) return 0;
    if (info && len > 0) std::snprintf(info, len, "%s, %.0f MB, sm_%d%d", p.name, p.totalGlobalMem / 1048576.0, p.major, p.minor);
    return p.major >= 6 ? 1 : 0; // atomicAdd(double) exige sm_60+
}

GQC_API void* gqc_alloc(int n) {
    Reg* r = new Reg();
    r->n = n;
    r->dim = 1ULL << n;
    if (!ok(cudaMalloc(&r->psi, r->dim * sizeof(double2)), "cudaMalloc estado") ||
        !ok(cudaMalloc(&r->acc, 2 * sizeof(double)), "cudaMalloc reducao")) {
        if (r->psi) cudaFree(r->psi);
        delete r;
        return nullptr;
    }
    k_zero<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, 0);
    if (!ok(cudaGetLastError(), "zerar")) { cudaFree(r->psi); cudaFree(r->acc); delete r; return nullptr; }
    return r;
}

GQC_API void gqc_free(void* h) {
    Reg* r = static_cast<Reg*>(h);
    if (!r) return;
    cudaFree(r->psi);
    cudaFree(r->acc);
    delete r;
}

GQC_API int gqc_set_basis(void* h, u64 basis) {
    Reg* r = static_cast<Reg*>(h);
    k_zero<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, basis);
    return ok(cudaGetLastError(), "set_basis") ? 1 : 0;
}

GQC_API int gqc_upload(void* h, const double* host) {
    Reg* r = static_cast<Reg*>(h);
    return ok(cudaMemcpy(r->psi, host, r->dim * sizeof(double2), cudaMemcpyHostToDevice), "upload") ? 1 : 0;
}

GQC_API int gqc_download(void* h, double* host) {
    Reg* r = static_cast<Reg*>(h);
    return ok(cudaMemcpy(host, r->psi, r->dim * sizeof(double2), cudaMemcpyDeviceToHost), "download") ? 1 : 0;
}

// m = {re00, im00, re01, im01, re10, im10, re11, im11}
GQC_API int gqc_apply_1q(void* h, u64 cmask, int q, const double* m) {
    Reg* r = static_cast<Reg*>(h);
    const u64 half = r->dim / 2;
    k_apply_1q<<<blocks_for(half), 256>>>(r->psi, half, cmask, q, make_double2(m[0], m[1]), make_double2(m[2], m[3]),
                                         make_double2(m[4], m[5]), make_double2(m[6], m[7]));
    return ok(cudaGetLastError(), "porta") ? 1 : 0;
}

GQC_API int gqc_swap(void* h, u64 cmask, int a, int b) {
    Reg* r = static_cast<Reg*>(h);
    k_swap<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, cmask, 1ULL << a, 1ULL << b);
    return ok(cudaGetLastError(), "swap") ? 1 : 0;
}

GQC_API int gqc_rzz(void* h, int a, int b, double theta) {
    Reg* r = static_cast<Reg*>(h);
    const double2 same = make_double2(cos(-theta / 2), sin(-theta / 2)), diff = make_double2(cos(theta / 2), sin(theta / 2));
    k_rzz<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, a, b, same, diff);
    return ok(cudaGetLastError(), "rzz") ? 1 : 0;
}

GQC_API int gqc_phase_flip(void* h, u64 basis) {
    Reg* r = static_cast<Reg*>(h);
    k_flip<<<1, 1>>>(r->psi, basis);
    return ok(cudaGetLastError(), "phase_flip") ? 1 : 0;
}

GQC_API int gqc_diffuser(void* h) {
    Reg* r = static_cast<Reg*>(h);
    if (!clear_acc(r)) return 0;
    k_sum_complex<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, r->acc);
    double sum[2];
    read_acc(r, 2, sum);
    const double2 tm = make_double2(2.0 * sum[0] / r->dim, 2.0 * sum[1] / r->dim);
    k_reflect<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, tm);
    return ok(cudaGetLastError(), "diffuser") ? 1 : 0;
}

GQC_API double gqc_norm2(void* h, u64 mask) {
    Reg* r = static_cast<Reg*>(h);
    if (!clear_acc(r)) return -1.0;
    k_norm2<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, mask, r->acc);
    if (!ok(cudaGetLastError(), "norma")) return -1.0;
    return read_acc(r, 1, nullptr);
}

GQC_API int gqc_collapse(void* h, int q, int outcome, double scale) {
    Reg* r = static_cast<Reg*>(h);
    k_collapse<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, 1ULL << q, outcome, scale);
    return ok(cudaGetLastError(), "colapso") ? 1 : 0;
}

// pauli: uma letra por qubit, a mais à esquerda é o qubit mais alto (mesma convenção do GhostQReg)
GQC_API double gqc_expect_pauli(void* h, const char* pauli) {
    Reg* r = static_cast<Reg*>(h);
    u64 xmask = 0, zymask = 0;
    int ny = 0;
    for (int idx = 0; idx < r->n; ++idx) {
        const u64 bit = 1ULL << (r->n - 1 - idx);
        switch (pauli[idx]) {
            case 'X': case 'x': xmask |= bit; break;
            case 'Y': case 'y': xmask |= bit; zymask |= bit; ++ny; break;
            case 'Z': case 'z': zymask |= bit; break;
            default: break;
        }
    }
    const double2 iny[4] = {make_double2(1, 0), make_double2(0, 1), make_double2(-1, 0), make_double2(0, -1)};
    if (!clear_acc(r)) return 0.0;
    k_pauli<<<blocks_for(r->dim), 256>>>(r->psi, r->dim, xmask, zymask, iny[ny % 4], r->acc);
    if (!ok(cudaGetLastError(), "pauli")) return 0.0;
    return read_acc(r, 1, nullptr);
}
