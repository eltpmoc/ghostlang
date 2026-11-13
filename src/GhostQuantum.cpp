// GhostQuantum.cpp - Simulador statevector nativo da GhostLang (F3).
// Veja as convenções em include/GhostQuantum.hpp.
#include "GhostQuantum.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <mutex>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {
using amp = GhostQReg::amp;
constexpr long long PAR_MIN = 1LL << 13; // abaixo disso o OpenMP custa mais do que ganha
constexpr std::size_t MAX_LOG = 5000;
const amp I(0.0, 1.0);

// Insere um bit 0 na posição q de i (enumera os pares |..0..>, |..1..>).
inline std::uint64_t insert_zero(std::uint64_t i, int q) {
    const std::uint64_t low = i & ((1ULL << q) - 1ULL);
    return ((i >> q) << (q + 1)) | low;
}

void apply_vec(std::vector<amp>& psi, std::uint64_t control_mask, int q, amp m00, amp m01, amp m10, amp m11) {
    const long long half = static_cast<long long>(psi.size() / 2);
    const std::uint64_t bit = 1ULL << q;
    #pragma omp parallel for if(half >= PAR_MIN)
    for (long long i = 0; i < half; ++i) {
        const std::uint64_t i0 = insert_zero(static_cast<std::uint64_t>(i), q);
        if ((i0 & control_mask) != control_mask) continue;
        const std::uint64_t i1 = i0 | bit;
        const amp a = psi[i0], b = psi[i1];
        psi[i0] = m00 * a + m01 * b;
        psi[i1] = m10 * a + m11 * b;
    }
}

std::string fmt(double v, int digits) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(digits) << v;
    std::string s = os.str();
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

std::string angle_label(const std::string& name, double v) {
    return name + "(" + fmt(v, 2) + ")";
}

// ---------------------------------------------------------------- Backend CUDA (carregado em tempo de execução)
// API em C de src/cuda/GhostQuantumCuda.cu. Sem a biblioteca ou sem GPU, ok = false e tudo roda na CPU.
struct CudaApi {
    bool ok = false;
    std::string info;
    int (*device_info)(char*, int) = nullptr;
    void* (*alloc)(int) = nullptr;
    void (*release)(void*) = nullptr;
    int (*set_basis)(void*, unsigned long long) = nullptr;
    int (*upload)(void*, const double*) = nullptr;
    int (*download)(void*, double*) = nullptr;
    int (*apply_1q)(void*, unsigned long long, int, const double*) = nullptr;
    int (*swap)(void*, unsigned long long, int, int) = nullptr;
    int (*rzz)(void*, int, int, double) = nullptr;
    int (*phase_flip)(void*, unsigned long long) = nullptr;
    int (*diffuser)(void*) = nullptr;
    double (*norm2)(void*, unsigned long long) = nullptr;
    int (*collapse)(void*, int, int, double) = nullptr;
    double (*expect_pauli)(void*, const char*) = nullptr;
    const char* (*last_error)() = nullptr;
};

template <typename F> void bind(F& fn, void* p) { fn = reinterpret_cast<F>(p); }

const CudaApi& cuda_api() {
    static CudaApi api;
    static std::once_flag once;
    std::call_once(once, [] {
        std::vector<std::string> names;
        if (const char* env = std::getenv("GHOST_QCUDA_LIB")) if (*env) names.push_back(env);
#if defined(_WIN32)
        names.push_back("ghost_quantum_cuda.dll"); // pasta do executável, depois o PATH
        HMODULE h = nullptr;
        for (const auto& nm : names) if ((h = LoadLibraryA(nm.c_str())) != nullptr) break;
        auto sym = [&](const char* s) -> void* { return h ? reinterpret_cast<void*>(GetProcAddress(h, s)) : nullptr; };
#else
        names.push_back("libghost_quantum_cuda.so");
        void* h = nullptr;
        for (const auto& nm : names) if ((h = dlopen(nm.c_str(), RTLD_NOW)) != nullptr) break;
        auto sym = [&](const char* s) -> void* { return h ? dlsym(h, s) : nullptr; };
#endif
        if (!h) { api.info = "CPU (backend CUDA ghost_quantum_cuda nao encontrado)"; return; }
        bind(api.device_info, sym("gqc_device_info"));
        bind(api.alloc, sym("gqc_alloc"));
        bind(api.release, sym("gqc_free"));
        bind(api.set_basis, sym("gqc_set_basis"));
        bind(api.upload, sym("gqc_upload"));
        bind(api.download, sym("gqc_download"));
        bind(api.apply_1q, sym("gqc_apply_1q"));
        bind(api.swap, sym("gqc_swap"));
        bind(api.rzz, sym("gqc_rzz"));
        bind(api.phase_flip, sym("gqc_phase_flip"));
        bind(api.diffuser, sym("gqc_diffuser"));
        bind(api.norm2, sym("gqc_norm2"));
        bind(api.collapse, sym("gqc_collapse"));
        bind(api.expect_pauli, sym("gqc_expect_pauli"));
        bind(api.last_error, sym("gqc_last_error"));
        if (!api.device_info || !api.alloc || !api.release || !api.set_basis || !api.upload || !api.download || !api.apply_1q ||
            !api.swap || !api.rzz || !api.phase_flip || !api.diffuser || !api.norm2 || !api.collapse || !api.expect_pauli || !api.last_error) {
            api.info = "CPU (backend CUDA incompleto ou de outra versao)";
            return;
        }
        char buf[256] = {0};
        if (!api.device_info(buf, sizeof buf)) { api.info = std::string("CPU (CUDA indisponivel: ") + buf + ")"; return; }
        api.ok = true;
        api.info = std::string("CUDA: ") + buf;
    });
    return api;
}

std::string& default_device_ref() {
    static std::string d = [] {
        const char* e = std::getenv("GHOST_QDEVICE");
        return std::string(e && *e ? e : "auto");
    }();
    return d;
}

void validate_device(const std::string& name) {
    if (name != "cpu" && name != "cuda" && name != "auto") throw std::invalid_argument("qreg.device: use \"cpu\", \"cuda\" ou \"auto\" (recebido \"" + name + "\")");
}
} // namespace

// ---------------------------------------------------------------- Estado

// A semente aleatoria so e sorteada na primeira medicao (criar registradores no treino fica barato).
GhostQReg::GhostQReg(int qubits) : n_(qubits), rng_(0x9E3779B97F4A7C15ULL) {
    if (qubits < 1 || qubits > MAX_QUBITS) {
        throw std::invalid_argument("qreg: numero de qubits deve estar entre 1 e " + std::to_string(MAX_QUBITS) +
                                    " (recebido " + std::to_string(qubits) + ")");
    }
    psi_.assign(std::size_t(1) << n_, amp(0.0, 0.0));
    psi_[0] = 1.0;
    const std::string& dev = default_device_ref();
    if (dev == "cuda" || (dev == "auto" && n_ >= AUTO_GPU_MIN_QUBITS)) {
        attach_gpu(dev == "cuda");
        if (gpu_) dev_ok_ = true; // gqc_alloc já cria |0...0> na GPU
    }
}

GhostQReg::GhostQReg(const GhostQReg& o)
    : n_(o.n_), rng_(o.rng_), log_(o.log_), log_truncated_(o.log_truncated_), recording_(o.recording_), seeded_(o.seeded_) {
    o.sync_host();
    psi_ = o.psi_;
    if (o.gpu_) attach_gpu(false); // cópia própria na GPU; o estado sobe na próxima operação
}

GhostQReg& GhostQReg::operator=(const GhostQReg& o) {
    if (this == &o) return *this;
    GhostQReg tmp(o);
    *this = std::move(tmp);
    return *this;
}

GhostQReg::GhostQReg(GhostQReg&& o) noexcept
    : n_(o.n_), psi_(std::move(o.psi_)), rng_(o.rng_), log_(std::move(o.log_)), log_truncated_(o.log_truncated_),
      recording_(o.recording_), seeded_(o.seeded_), gpu_(o.gpu_), host_ok_(o.host_ok_), dev_ok_(o.dev_ok_) {
    o.gpu_ = nullptr;
    o.host_ok_ = true;
    o.dev_ok_ = false;
}

GhostQReg& GhostQReg::operator=(GhostQReg&& o) noexcept {
    if (this == &o) return *this;
    release_gpu(false);
    n_ = o.n_; psi_ = std::move(o.psi_); rng_ = o.rng_; log_ = std::move(o.log_); log_truncated_ = o.log_truncated_;
    recording_ = o.recording_; seeded_ = o.seeded_; gpu_ = o.gpu_; host_ok_ = o.host_ok_; dev_ok_ = o.dev_ok_;
    o.gpu_ = nullptr; o.host_ok_ = true; o.dev_ok_ = false;
    return *this;
}

GhostQReg::~GhostQReg() { release_gpu(false); }

void GhostQReg::attach_gpu(bool required) {
    if (gpu_) return;
    const CudaApi& api = cuda_api();
    if (!api.ok) {
        if (required) throw std::runtime_error("qreg: dispositivo \"cuda\" indisponivel - " + api.info);
        return;
    }
    void* g = api.alloc(n_);
    if (!g) {
        if (required) throw std::runtime_error(std::string("qreg: falha ao alocar o estado na GPU - ") + api.last_error());
        return; // auto: continua na CPU (ex.: memória insuficiente)
    }
    gpu_ = g;
    dev_ok_ = false;
}

void GhostQReg::release_gpu(bool keep_state) {
    if (!gpu_) return;
    if (keep_state) sync_host();
    cuda_api().release(gpu_);
    gpu_ = nullptr;
    dev_ok_ = false;
    host_ok_ = true;
}

void GhostQReg::gpu_check(int ok) const {
    if (!ok) throw std::runtime_error(std::string("qreg (GPU): ") + cuda_api().last_error());
}

void GhostQReg::sync_host() const {
    if (gpu_ && !host_ok_) {
        gpu_check(cuda_api().download(gpu_, reinterpret_cast<double*>(psi_.data())));
        host_ok_ = true;
    }
}

bool GhostQReg::gpu_ready() {
    if (!gpu_) return false;
    if (!dev_ok_) {
        gpu_check(cuda_api().upload(gpu_, reinterpret_cast<const double*>(psi_.data())));
        dev_ok_ = true;
    }
    return true;
}

void GhostQReg::host_write() {
    sync_host();
    if (gpu_) dev_ok_ = false;
}

void GhostQReg::device(const std::string& name) {
    validate_device(name);
    if (name == "cpu") release_gpu(true);
    else if (name == "cuda") attach_gpu(true);
    else if (n_ >= AUTO_GPU_MIN_QUBITS) attach_gpu(false);
    else release_gpu(true);
}

void GhostQReg::set_default_device(const std::string& name) { validate_device(name); default_device_ref() = name; }
std::string GhostQReg::default_device() { return default_device_ref(); }
bool GhostQReg::cuda_available() { return cuda_api().ok; }
std::string GhostQReg::backend_info() { return cuda_api().info; }

const std::vector<GhostQReg::amp>& GhostQReg::state() const { sync_host(); return psi_; }

void GhostQReg::reset() {
    check_not_controlled("reset");
    std::fill(psi_.begin(), psi_.end(), amp(0.0, 0.0));
    psi_[0] = 1.0;
    host_ok_ = true;
    if (gpu_) { gpu_check(cuda_api().set_basis(gpu_, 0)); dev_ok_ = true; }
    log_.clear();
    log_truncated_ = false;
}

void GhostQReg::seed(unsigned long long value) { rng_.seed(value); seeded_ = true; }

void GhostQReg::set_basis(long long basis) {
    check_not_controlled("set_basis");
    if (basis < 0 || static_cast<std::size_t>(basis) >= psi_.size()) throw std::out_of_range("qreg.set_basis: estado fora do registrador");
    std::fill(psi_.begin(), psi_.end(), amp(0.0, 0.0));
    psi_[static_cast<std::size_t>(basis)] = 1.0;
    host_ok_ = true;
    if (gpu_) { gpu_check(cuda_api().set_basis(gpu_, static_cast<unsigned long long>(basis))); dev_ok_ = true; }
    record("|" + bits(basis) + ">", {}, {});
}

GhostQReg::amp GhostQReg::amplitude(long long basis) const {
    if (basis < 0 || static_cast<std::size_t>(basis) >= psi_.size()) throw std::out_of_range("qreg.amplitude: estado fora do registrador");
    sync_host();
    return psi_[static_cast<std::size_t>(basis)];
}

double GhostQReg::norm() const {
    if (gpu_ && dev_ok_) {
        const double s2 = cuda_api().norm2(gpu_, 0);
        gpu_check(s2 >= 0.0);
        return std::sqrt(s2);
    }
    sync_host();
    double s = 0.0;
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for reduction(+:s) if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) s += std::norm(psi_[i]);
    return std::sqrt(s);
}

void GhostQReg::check_qubit(int q, const char* what) const {
    if (q < 0 || q >= n_) {
        throw std::out_of_range(std::string("qreg.") + what + ": qubit " + std::to_string(q) +
                                " fora do registrador de " + std::to_string(n_) + " qubits");
    }
}

void GhostQReg::set_control_mask(std::uint64_t mask) {
    if (n_ < 64 && (mask >> n_) != 0) throw std::out_of_range("qreg: qubit de controle fora do registrador de " + std::to_string(n_) + " qubits");
    ctrl_mask_ = mask;
}

void GhostQReg::check_free_of_controls(std::uint64_t used, const char* what) const {
    if (!(used & ctrl_mask_)) return;
    int q = 0;
    while (!(((used & ctrl_mask_) >> q) & 1ULL)) ++q;
    throw std::invalid_argument(std::string("circuit controlado: ") + what + " usa o qubit " + std::to_string(q) + ", que e o controle");
}

void GhostQReg::check_not_controlled(const char* what) const {
    if (ctrl_mask_) {
        throw std::invalid_argument(std::string("circuit controlado: ") + what + " atua no registrador inteiro (inclusive no controle) e nao pode ser controlado");
    }
}

void GhostQReg::record(const std::string& label, std::vector<int> controls, std::vector<int> targets) {
    if (!recording_) return;
    for (int q = 0; ctrl_mask_ && q < n_; ++q) {
        if ((ctrl_mask_ >> q) & 1ULL) controls.push_back(q); // controles extras aparecem no draw()
    }
    if (log_.size() >= MAX_LOG) { log_truncated_ = true; return; }
    log_.push_back({label, std::move(controls), std::move(targets)});
}

double GhostQReg::uniform() {
    if (!seeded_) { rng_.seed(std::random_device{}()); seeded_ = true; }
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng_);
}

// ---------------------------------------------------------------- Genéricas

void GhostQReg::kernel_1q(std::uint64_t control_mask, int q, amp m00, amp m01, amp m10, amp m11) {
    if (ctrl_mask_) {
        check_free_of_controls(control_mask | (1ULL << q), "uma porta");
        control_mask |= ctrl_mask_;
    }
    if (gpu_ready()) {
        const double m[8] = {m00.real(), m00.imag(), m01.real(), m01.imag(), m10.real(), m10.imag(), m11.real(), m11.imag()};
        gpu_check(cuda_api().apply_1q(gpu_, control_mask, q, m));
        host_ok_ = false;
        return;
    }
    host_write();
    apply_vec(psi_, control_mask, q, m00, m01, m10, m11);
}

void GhostQReg::apply_1q(int q, amp m00, amp m01, amp m10, amp m11, const std::string& label) {
    check_qubit(q, "porta");
    kernel_1q(0, q, m00, m01, m10, m11);
    record(label, {}, {q});
}

void GhostQReg::apply_controlled_1q(int control, int q, amp m00, amp m01, amp m10, amp m11, const std::string& label) {
    check_qubit(control, "controle");
    check_qubit(q, "alvo");
    if (control == q) throw std::invalid_argument("qreg: controle e alvo precisam ser qubits diferentes");
    kernel_1q(1ULL << control, q, m00, m01, m10, m11);
    record(label, {control}, {q});
}

// ---------------------------------------------------------------- 1 qubit

void GhostQReg::h(int q) { const double r = 1.0 / std::sqrt(2.0); apply_1q(q, r, r, r, -r, "H"); }
void GhostQReg::x(int q) { apply_1q(q, 0.0, 1.0, 1.0, 0.0, "X"); }
void GhostQReg::y(int q) { apply_1q(q, 0.0, -I, I, 0.0, "Y"); }
void GhostQReg::z(int q) { apply_1q(q, 1.0, 0.0, 0.0, -1.0, "Z"); }
void GhostQReg::s(int q) { apply_1q(q, 1.0, 0.0, 0.0, I, "S"); }
void GhostQReg::sdg(int q) { apply_1q(q, 1.0, 0.0, 0.0, -I, "Sdg"); }
void GhostQReg::t(int q) { apply_1q(q, 1.0, 0.0, 0.0, std::polar(1.0, PI / 4), "T"); }
void GhostQReg::tdg(int q) { apply_1q(q, 1.0, 0.0, 0.0, std::polar(1.0, -PI / 4), "Tdg"); }
void GhostQReg::sx(int q) { apply_1q(q, amp(0.5, 0.5), amp(0.5, -0.5), amp(0.5, -0.5), amp(0.5, 0.5), "SX"); }

void GhostQReg::rx(int q, double th) {
    const double c = std::cos(th / 2), s = std::sin(th / 2);
    apply_1q(q, c, -I * s, -I * s, c, (recording_ ? angle_label("RX", th) : std::string()));
}
void GhostQReg::ry(int q, double th) {
    const double c = std::cos(th / 2), s = std::sin(th / 2);
    apply_1q(q, c, -s, s, c, (recording_ ? angle_label("RY", th) : std::string()));
}
void GhostQReg::rz(int q, double th) {
    apply_1q(q, std::polar(1.0, -th / 2), 0.0, 0.0, std::polar(1.0, th / 2), (recording_ ? angle_label("RZ", th) : std::string()));
}
void GhostQReg::p(int q, double phi) { apply_1q(q, 1.0, 0.0, 0.0, std::polar(1.0, phi), (recording_ ? angle_label("P", phi) : std::string())); }

void GhostQReg::u(int q, double th, double phi, double lam) {
    const double c = std::cos(th / 2), s = std::sin(th / 2);
    apply_1q(q, c, -std::polar(1.0, lam) * s, std::polar(1.0, phi) * s, std::polar(1.0, phi + lam) * c, "U");
}

void GhostQReg::h_all() { check_not_controlled("h_all"); for (int q = 0; q < n_; ++q) h(q); }
void GhostQReg::x_all() { check_not_controlled("x_all"); for (int q = 0; q < n_; ++q) x(q); }

// ---------------------------------------------------------------- Controladas

void GhostQReg::cx(int c, int t) { apply_controlled_1q(c, t, 0.0, 1.0, 1.0, 0.0, "X"); }
void GhostQReg::cy(int c, int t) { apply_controlled_1q(c, t, 0.0, -I, I, 0.0, "Y"); }
void GhostQReg::cz(int c, int t) { apply_controlled_1q(c, t, 1.0, 0.0, 0.0, -1.0, "Z"); }
void GhostQReg::ch(int c, int t) { const double r = 1.0 / std::sqrt(2.0); apply_controlled_1q(c, t, r, r, r, -r, "H"); }
void GhostQReg::cp(int c, int t, double phi) { apply_controlled_1q(c, t, 1.0, 0.0, 0.0, std::polar(1.0, phi), (recording_ ? angle_label("P", phi) : std::string())); }
void GhostQReg::crx(int c, int t, double th) {
    const double co = std::cos(th / 2), s = std::sin(th / 2);
    apply_controlled_1q(c, t, co, -I * s, -I * s, co, (recording_ ? angle_label("RX", th) : std::string()));
}
void GhostQReg::cry(int c, int t, double th) {
    const double co = std::cos(th / 2), s = std::sin(th / 2);
    apply_controlled_1q(c, t, co, -s, s, co, (recording_ ? angle_label("RY", th) : std::string()));
}
void GhostQReg::crz(int c, int t, double th) {
    apply_controlled_1q(c, t, std::polar(1.0, -th / 2), 0.0, 0.0, std::polar(1.0, th / 2), (recording_ ? angle_label("RZ", th) : std::string()));
}

void GhostQReg::swap(int a, int b) {
    check_qubit(a, "swap");
    check_qubit(b, "swap");
    if (a == b) return;
    check_free_of_controls((1ULL << a) | (1ULL << b), "swap");
    const std::uint64_t cm = ctrl_mask_;
    if (gpu_ready()) {
        gpu_check(cuda_api().swap(gpu_, cm, a, b));
        host_ok_ = false;
        record("x", {}, {a, b});
        return;
    }
    host_write();
    const std::uint64_t ma = 1ULL << a, mb = 1ULL << b;
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) {
        const std::uint64_t u = static_cast<std::uint64_t>(i);
        if ((u & cm) == cm && (u & ma) && !(u & mb)) std::swap(psi_[u], psi_[u ^ (ma | mb)]);
    }
    record("x", {}, {a, b});
}

void GhostQReg::rzz(int a, int b, double th, const std::string& label) {
    check_qubit(a, "rzz");
    check_qubit(b, "rzz");
    if (a == b) throw std::invalid_argument("qreg.rzz: os dois qubits precisam ser diferentes");
    if (ctrl_mask_) {
        // Controlado: RZZ = CX(a,b) · RZ_b(θ) · CX(a,b); kernel_1q acrescenta os controles extras.
        check_free_of_controls((1ULL << a) | (1ULL << b), "rzz");
        kernel_1q(1ULL << a, b, 0.0, 1.0, 1.0, 0.0);
        kernel_1q(0, b, std::polar(1.0, -th / 2), 0.0, 0.0, std::polar(1.0, th / 2));
        kernel_1q(1ULL << a, b, 0.0, 1.0, 1.0, 0.0);
        record(!recording_ ? std::string() : (label.empty() ? angle_label("ZZ", th) : label), {}, {a, b});
        return;
    }
    if (gpu_ready()) {
        gpu_check(cuda_api().rzz(gpu_, a, b, th));
        host_ok_ = false;
        record(!recording_ ? std::string() : (label.empty() ? angle_label("ZZ", th) : label), {}, {a, b});
        return;
    }
    host_write();
    const amp same = std::polar(1.0, -th / 2), diff = std::polar(1.0, th / 2);
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) {
        const bool ba = (i >> a) & 1LL, bb = (i >> b) & 1LL;
        psi_[i] *= (ba == bb) ? same : diff;
    }
    record(!recording_ ? std::string() : (label.empty() ? angle_label("ZZ", th) : label), {}, {a, b});
}

void GhostQReg::ccx(int c1, int c2, int t) {
    check_qubit(c1, "ccx");
    check_qubit(c2, "ccx");
    check_qubit(t, "ccx");
    if (c1 == c2 || c1 == t || c2 == t) throw std::invalid_argument("qreg.ccx: os tres qubits precisam ser diferentes");
    kernel_1q((1ULL << c1) | (1ULL << c2), t, 0.0, 1.0, 1.0, 0.0);
    record("X", {c1, c2}, {t});
}

void GhostQReg::mcx(const std::vector<int>& controls, int t) {
    check_qubit(t, "mcx");
    std::uint64_t mask = 0;
    for (int c : controls) {
        check_qubit(c, "mcx");
        if (c == t) throw std::invalid_argument("qreg.mcx: o alvo nao pode ser controle");
        if (mask & (1ULL << c)) throw std::invalid_argument("qreg.mcx: controles duplicados");
        mask |= (1ULL << c);
    }
    kernel_1q(mask, t, 0.0, 1.0, 1.0, 0.0);
    record("X", controls, {t});
}

void GhostQReg::cswap(int c, int a, int b) {
    check_qubit(c, "cswap");
    check_qubit(a, "cswap");
    check_qubit(b, "cswap");
    if (c == a || c == b || a == b) throw std::invalid_argument("qreg.cswap: os tres qubits precisam ser diferentes");
    check_free_of_controls((1ULL << c) | (1ULL << a) | (1ULL << b), "cswap");
    const std::uint64_t mc = (1ULL << c) | ctrl_mask_;
    if (gpu_ready()) {
        gpu_check(cuda_api().swap(gpu_, mc, a, b));
        host_ok_ = false;
        record("x", {c}, {a, b});
        return;
    }
    host_write();
    const std::uint64_t ma = 1ULL << a, mb = 1ULL << b;
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) {
        const std::uint64_t u = static_cast<std::uint64_t>(i);
        if ((u & mc) == mc && (u & ma) && !(u & mb)) std::swap(psi_[u], psi_[u ^ (ma | mb)]);
    }
    record("x", {c}, {a, b});
}

// ---------------------------------------------------------------- Algoritmos

void GhostQReg::qft_range(int start, int count) {
    if (count < 1 || start < 0 || start + count > n_) throw std::out_of_range("qreg.qft: intervalo de qubits invalido");
    check_free_of_controls(((count >= 64) ? ~0ULL : ((1ULL << count) - 1)) << start, "qft");
    for (int j = count - 1; j >= 0; --j) {
        h(start + j);
        for (int k = j - 1; k >= 0; --k) cp(start + k, start + j, PI / double(1LL << (j - k)));
    }
    for (int i = 0; i < count / 2; ++i) swap(start + i, start + count - 1 - i);
}

void GhostQReg::iqft_range(int start, int count) {
    if (count < 1 || start < 0 || start + count > n_) throw std::out_of_range("qreg.iqft: intervalo de qubits invalido");
    check_free_of_controls(((count >= 64) ? ~0ULL : ((1ULL << count) - 1)) << start, "iqft");
    for (int i = count / 2 - 1; i >= 0; --i) swap(start + i, start + count - 1 - i);
    for (int j = 0; j < count; ++j) {
        for (int k = 0; k < j; ++k) cp(start + k, start + j, -PI / double(1LL << (j - k)));
        h(start + j);
    }
}

void GhostQReg::qft() { qft_range(0, n_); }
void GhostQReg::iqft() { iqft_range(0, n_); }

void GhostQReg::phase_flip(long long basis) {
    if (basis < 0 || static_cast<std::size_t>(basis) >= psi_.size()) throw std::out_of_range("qreg.phase_flip: estado fora do registrador");
    check_not_controlled("phase_flip");
    if (gpu_ready()) {
        gpu_check(cuda_api().phase_flip(gpu_, static_cast<unsigned long long>(basis)));
        host_ok_ = false;
    } else {
        host_write();
        psi_[static_cast<std::size_t>(basis)] = -psi_[static_cast<std::size_t>(basis)];
    }
    std::vector<int> all(n_);
    for (int q = 0; q < n_; ++q) all[q] = q;
    record("Of", {}, all);
}

void GhostQReg::diffuser() {
    check_not_controlled("diffuser");
    if (gpu_ready()) {
        gpu_check(cuda_api().diffuser(gpu_));
        host_ok_ = false;
        std::vector<int> all(n_);
        for (int q = 0; q < n_; ++q) all[q] = q;
        record("D", {}, all);
        return;
    }
    host_write();
    const long long dim = static_cast<long long>(psi_.size());
    double re = 0.0, im = 0.0;
    #pragma omp parallel for reduction(+:re,im) if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) { re += psi_[i].real(); im += psi_[i].imag(); }
    const amp twice_mean = 2.0 * amp(re, im) / double(dim);
    #pragma omp parallel for if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) psi_[i] = twice_mean - psi_[i];
    std::vector<int> all(n_);
    for (int q = 0; q < n_; ++q) all[q] = q;
    record("D", {}, all);
}

// ---------------------------------------------------------------- Medição

double GhostQReg::prob(int q) const {
    check_qubit(q, "prob");
    if (gpu_ && dev_ok_) {
        const double p1 = cuda_api().norm2(gpu_, 1ULL << q);
        gpu_check(p1 >= 0.0);
        return p1;
    }
    sync_host();
    const std::uint64_t bit = 1ULL << q;
    const long long dim = static_cast<long long>(psi_.size());
    double s = 0.0;
    #pragma omp parallel for reduction(+:s) if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) if (static_cast<std::uint64_t>(i) & bit) s += std::norm(psi_[i]);
    return s;
}

double GhostQReg::prob_state(long long basis) const { return std::norm(amplitude(basis)); }

int GhostQReg::measure(int q) {
    check_not_controlled("measure");
    const double p1 = prob(q);
    const int outcome = uniform() < p1 ? 1 : 0;
    const double keep = outcome ? p1 : 1.0 - p1;
    const double scale = keep > 1e-300 ? 1.0 / std::sqrt(keep) : 0.0;
    if (gpu_ready()) {
        gpu_check(cuda_api().collapse(gpu_, q, outcome, scale));
        host_ok_ = false;
        record("M", {}, {q});
        return outcome;
    }
    host_write();
    const std::uint64_t bit = 1ULL << q;
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) {
        const bool set = (static_cast<std::uint64_t>(i) & bit) != 0;
        psi_[i] = (set == (outcome == 1)) ? psi_[i] * scale : amp(0.0, 0.0);
    }
    record("M", {}, {q});
    return outcome;
}

long long GhostQReg::measure_all() {
    check_not_controlled("measure_all");
    host_write();
    const double r = uniform() * norm() * norm();
    double acc = 0.0;
    std::size_t pick = psi_.size() - 1;
    for (std::size_t i = 0; i < psi_.size(); ++i) {
        acc += std::norm(psi_[i]);
        if (r < acc) { pick = i; break; }
    }
    std::fill(psi_.begin(), psi_.end(), amp(0.0, 0.0));
    psi_[pick] = 1.0;
    std::vector<int> all(n_);
    for (int q = 0; q < n_; ++q) all[q] = q;
    record("M", {}, all);
    return static_cast<long long>(pick);
}

std::map<std::string, int> GhostQReg::counts(int shots) {
    if (shots < 1) throw std::invalid_argument("qreg.counts: shots deve ser >= 1");
    sync_host();
    std::vector<double> cdf(psi_.size());
    double acc = 0.0;
    for (std::size_t i = 0; i < psi_.size(); ++i) { acc += std::norm(psi_[i]); cdf[i] = acc; }
    std::map<long long, int> raw;
    for (int s = 0; s < shots; ++s) {
        const double r = uniform() * acc;
        auto it = std::upper_bound(cdf.begin(), cdf.end(), r);
        if (it == cdf.end()) --it;
        raw[static_cast<long long>(it - cdf.begin())]++;
    }
    std::map<std::string, int> out;
    for (const auto& kv : raw) out[bits(kv.first)] = kv.second;
    return out;
}

std::string GhostQReg::histogram(int shots) {
    const auto c = counts(shots);
    std::vector<std::pair<std::string, int>> rows(c.begin(), c.end());
    const std::size_t limit = 32;
    if (rows.size() > limit) {
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        rows.resize(limit);
        std::sort(rows.begin(), rows.end());
    }
    int maxc = 1;
    for (const auto& r : rows) maxc = std::max(maxc, r.second);
    std::ostringstream os;
    os << "Medicoes (" << shots << " shots):";
    for (const auto& r : rows) {
        const int bar = static_cast<int>(std::round(30.0 * r.second / maxc));
        os << "\n  |" << r.first << "> " << std::string(bar, '#') << std::string(30 - bar, ' ') << " "
           << r.second << " (" << fmt(100.0 * r.second / shots, 1) << "%)";
    }
    if (c.size() > limit) os << "\n  ... +" << (c.size() - limit) << " estados com menos contagens";
    return os.str();
}

// ---------------------------------------------------------------- Observáveis

// Ruido de ponto flutuante (ex.: 2.2e-16) vira 0 para nao poluir a saida.
static double clean(double v) { return std::abs(v) < 1e-12 ? 0.0 : v; }

double GhostQReg::expect_z(int q) const { return clean(1.0 - 2.0 * prob(q)); }

double GhostQReg::expect(const std::string& pauli) const {
    if (static_cast<int>(pauli.size()) != n_) {
        throw std::invalid_argument("qreg.expect: a string de Pauli precisa ter " + std::to_string(n_) + " letras (I, X, Y, Z)");
    }
    for (char c : pauli) {
        if (std::string("IXYZixyz").find(c) == std::string::npos) throw std::invalid_argument(std::string("qreg.expect: letra invalida '") + c + "'");
    }
    if (gpu_ && dev_ok_) return clean(cuda_api().expect_pauli(gpu_, pauli.c_str())); // na GPU, sem copiar o estado
    sync_host();
    std::vector<amp> phi = psi_;
    for (int idx = 0; idx < n_; ++idx) {
        const int q = n_ - 1 - idx; // a letra mais a esquerda e o qubit mais alto
        switch (pauli[idx]) {
            case 'X': case 'x': apply_vec(phi, 0, q, 0.0, 1.0, 1.0, 0.0); break;
            case 'Y': case 'y': apply_vec(phi, 0, q, 0.0, -I, I, 0.0); break;
            case 'Z': case 'z': apply_vec(phi, 0, q, 1.0, 0.0, 0.0, -1.0); break;
            default: break;
        }
    }
    double re = 0.0;
    const long long dim = static_cast<long long>(psi_.size());
    #pragma omp parallel for reduction(+:re) if(dim >= PAR_MIN)
    for (long long i = 0; i < dim; ++i) re += (std::conj(psi_[i]) * phi[i]).real();
    return clean(re);
}

// ---------------------------------------------------------------- Visualização

std::string GhostQReg::bits(long long value) const {
    std::string s(n_, '0');
    for (int q = 0; q < n_; ++q) if ((value >> q) & 1LL) s[n_ - 1 - q] = '1';
    return s;
}

std::string GhostQReg::ket(int digits) const {
    sync_host();
    const double eps = 0.5 * std::pow(10.0, -digits);
    std::ostringstream os;
    int shown = 0, hidden = 0;
    for (std::size_t i = 0; i < psi_.size(); ++i) {
        const amp a = psi_[i];
        if (std::abs(a) < eps) continue;
        if (shown >= 64) { ++hidden; continue; }
        const bool re0 = std::abs(a.real()) < eps, im0 = std::abs(a.imag()) < eps;
        std::string coef;
        bool negative = false;
        if (im0) { negative = a.real() < 0; coef = fmt(std::abs(a.real()), digits); }
        else if (re0) { negative = a.imag() < 0; coef = fmt(std::abs(a.imag()), digits) + "i"; }
        else coef = "(" + fmt(a.real(), digits) + (a.imag() < 0 ? "-" : "+") + fmt(std::abs(a.imag()), digits) + "i)";
        if (shown == 0) os << (negative ? "-" : "");
        else os << (negative ? " - " : " + ");
        os << coef << "|" << bits(static_cast<long long>(i)) << ">";
        ++shown;
    }
    if (shown == 0) return "0";
    if (hidden) os << " + ... (" << hidden << " termos)";
    return os.str();
}

std::string GhostQReg::probabilities(int digits) const {
    sync_host();
    std::ostringstream os;
    bool first = true;
    for (std::size_t i = 0; i < psi_.size(); ++i) {
        const double pr = std::norm(psi_[i]);
        if (pr < 0.5 * std::pow(10.0, -digits)) continue;
        if (!first) os << "\n";
        os << "|" << bits(static_cast<long long>(i)) << ">: " << fmt(pr, digits);
        first = false;
    }
    return first ? "(estado nulo)" : os.str();
}

std::string GhostQReg::draw() const {
    const std::size_t max_cols = 60;
    const std::size_t cols = std::min(log_.size(), max_cols);
    const int rows = 2 * n_ - 1;
    std::vector<std::string> lines(rows);
    const std::size_t name_w = 1 + std::to_string(n_ - 1).size();
    for (int q = 0; q < n_; ++q) {
        std::string name = "q" + std::to_string(q);
        lines[2 * q] = name + std::string(name_w - name.size(), ' ') + ": -";
        if (q < n_ - 1) lines[2 * q + 1] = std::string(lines[2 * q].size(), ' ');
    }
    for (std::size_t k = 0; k < cols; ++k) {
        const Op& op = log_[k];
        std::vector<int> all = op.controls;
        all.insert(all.end(), op.targets.begin(), op.targets.end());
        if (all.empty()) continue; // preparação de estado: sem coluna
        const int lo = *std::min_element(all.begin(), all.end());
        const int hi = *std::max_element(all.begin(), all.end());
        const int w = static_cast<int>(std::max<std::size_t>(op.label.size(), 1)) + 2;
        auto center = [&](const std::string& s, char fill) {
            const int left = (w - static_cast<int>(s.size())) / 2;
            return std::string(left, fill) + s + std::string(w - left - static_cast<int>(s.size()), fill);
        };
        for (int q = 0; q < n_; ++q) {
            std::string cell;
            if (std::find(op.targets.begin(), op.targets.end(), q) != op.targets.end()) cell = center(op.label, '-');
            else if (std::find(op.controls.begin(), op.controls.end(), q) != op.controls.end()) cell = center("*", '-');
            else if (q > lo && q < hi) cell = center("+", '-');
            else cell = std::string(w, '-');
            lines[2 * q] += cell + "-";
            if (q < n_ - 1) lines[2 * q + 1] += (q >= lo && q < hi ? center("|", ' ') : std::string(w, ' ')) + " ";
        }
    }
    std::ostringstream os;
    for (int r = 0; r < rows; ++r) {
        std::string l = lines[r];
        while (!l.empty() && l.back() == ' ') l.pop_back();
        os << l << "\n";
    }
    if (log_.size() > max_cols || log_truncated_) {
        os << "(+" << (log_.size() - cols) << " operacoes" << (log_truncated_ ? ", historico truncado" : "") << ")\n";
    }
    return os.str();
}

void GhostQReg::display() const { std::cout << ket() << std::endl; }

std::ostream& operator<<(std::ostream& os, const GhostQReg& reg) { return os << reg.ket(); }

std::string GhostQReg::to_qasm() const {
    std::ostringstream ss;
    ss << "OPENQASM 3.0;\n";
    ss << "include \"stdgates.inc\";\n";
    ss << "qubit[" << n_ << "] q;\n";
    for (const auto& op : log_) {
        std::string l = op.label;
        std::transform(l.begin(), l.end(), l.begin(), [](unsigned char c) { return std::tolower(c); });
        if (op.controls.empty() && op.targets.size() == 1) {
            int q = op.targets[0];
            if (l == "h" || l == "x" || l == "y" || l == "z" || l == "s" || l == "t") {
                ss << l << " q[" << q << "];\n";
            } else if (l.rfind("rx", 0) == 0 || l.rfind("ry", 0) == 0 || l.rfind("rz", 0) == 0) {
                ss << l << " q[" << q << "];\n";
            }
        } else if (op.controls.size() == 1 && op.targets.size() == 1) {
            int c = op.controls[0], t = op.targets[0];
            if (l == "x" || l == "cx" || l == "cnot") ss << "cx q[" << c << "], q[" << t << "];\n";
            else if (l == "z" || l == "cz") ss << "cz q[" << c << "], q[" << t << "];\n";
        }
    }
    return ss.str();
}
