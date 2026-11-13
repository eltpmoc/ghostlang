#pragma once
// GhostQuantum.hpp - Simulador quântico statevector nativo da GhostLang (F3).
//
// Convenções:
//  - Amplitudes em std::complex<double> (física exata; a álgebra G não é associativa).
//  - Qubit 0 é o bit menos significativo. Kets são escritos q(n-1)...q0, ex.: |01> = q0 em 1.
//  - Strings de Pauli seguem a mesma ordem: "XZ" = X no q1 e Z no q0.
//  - Portas custam O(2^n) e são aplicadas direto no vetor de estado (sem matriz 2^n x 2^n).
//
// Na GhostLang:
//   qreg q(2);
//   q.h(0); q.cx(0, 1);
//   print(q);                 // 0.7071|00> + 0.7071|11>
//   print(q.histogram(1000)); // contagens de medição sem colapsar
//   int r = q.measure(0);     // mede e colapsa
#include <complex>
#include <cstdint>
#include <map>
#include <ostream>
#include <random>
#include <string>
#include <vector>

constexpr double PI = 3.14159265358979323846;

class GhostQReg {
public:
    using amp = std::complex<double>;
    static constexpr int MAX_QUBITS = 28; // 2^28 amplitudes = 4 GB

    explicit GhostQReg(int qubits = 1);
    GhostQReg(const GhostQReg& other);
    GhostQReg& operator=(const GhostQReg& other);
    GhostQReg(GhostQReg&& other) noexcept;
    GhostQReg& operator=(GhostQReg&& other) noexcept;
    ~GhostQReg();

    // ---- Dispositivo: CPU (OpenMP) ou GPU (CUDA, via ghost_quantum_cuda.dll/.so) ----
    // "auto" (padrão): GPU a partir de AUTO_GPU_MIN_QUBITS qubits se houver backend; "cuda": exige GPU; "cpu".
    // O padrão para novos registradores vem de GHOST_QDEVICE ou de set_default_device().
    static constexpr int AUTO_GPU_MIN_QUBITS = 14;
    void device(const std::string& name);
    std::string device_name() const { return gpu_ ? "cuda" : "cpu"; }
    static void set_default_device(const std::string& name);
    static std::string default_device();
    static bool cuda_available();
    static std::string backend_info();

    // ---- Estado ----
    int size() const { return n_; }
    struct QubitIterator {
        int cur;
        int& operator*() { return cur; }
        const int& operator*() const { return cur; }
        QubitIterator& operator++() { ++cur; return *this; }
        bool operator!=(const QubitIterator& o) const { return cur != o.cur; }
    };
    QubitIterator begin() const { return {0}; }
    QubitIterator end() const { return {n_}; }
    std::size_t dimension() const { return psi_.size(); }
    void reset();                         // volta para |0...0> e limpa o histórico
    void seed(unsigned long long value);  // medições reprodutíveis
    void set_recording(bool on) { recording_ = on; } // false: não guarda histórico (treino rápido)
    void set_basis(long long basis);      // prepara |basis>
    const std::vector<amp>& state() const;
    amp amplitude(long long basis) const;
    double norm() const;                  // deve ser 1

    // ---- Portas de 1 qubit ----
    void h(int q);
    void x(int q);
    void y(int q);
    void z(int q);
    void s(int q);
    void sdg(int q);
    void t(int q);
    void tdg(int q);
    void sx(int q);
    void rx(int q, double theta);
    void ry(int q, double theta);
    void rz(int q, double theta);
    void p(int q, double phi);                            // fase: diag(1, e^{i phi})
    void u(int q, double theta, double phi, double lambda); // U3 genérica
    void h_all();
    void x_all();

    // ---- Portas controladas e de vários qubits ----
    void cx(int control, int target);
    void cnot(int control, int target) { cx(control, target); }
    void cy(int control, int target);
    void cz(int control, int target);
    void ch(int control, int target);
    void cp(int control, int target, double phi);
    void crx(int control, int target, double theta);
    void cry(int control, int target, double theta);
    void crz(int control, int target, double theta);
    void swap(int a, int b);
    void rzz(int a, int b, double theta, const std::string& label = ""); // exp(-i θ/2 Z⊗Z): fase e^{-iθ/2} se q_a == q_b, e^{+iθ/2} se diferentes
    void ccx(int c1, int c2, int target);
    void toffoli(int c1, int c2, int target) { ccx(c1, c2, target); }
    void cswap(int control, int a, int b);
    void mcx(const std::vector<int>& controls, int target);
    void mcx(std::initializer_list<int> controls, int target) { mcx(std::vector<int>(controls), target); }
    void mcx(int c1, int c2, int c3, int target) { mcx(std::vector<int>{c1, c2, c3}, target); }
    void mcx(int c1, int c2, int c3, int c4, int target) { mcx(std::vector<int>{c1, c2, c3, c4}, target); }
    template <typename Container, typename std::enable_if<!std::is_integral<Container>::value, int>::type = 0>
    void mcx(const Container& controls, int target) {
        std::vector<int> ctls(controls.begin(), controls.end());
        mcx(ctls, target);
    }

    // ---- Blocos de algoritmos ----
    void qft();                       // QFT em todos os qubits
    void iqft();
    void qft_range(int start, int count);
    void iqft_range(int start, int count);
    void phase_flip(long long basis); // oráculo: |basis> -> -|basis>
    void diffuser();                  // difusão de Grover em todos os qubits: 2|s><s| - I

    // ---- Medição ----
    int measure(int q);               // mede 1 qubit e colapsa
    long long measure_all();          // mede todos e colapsa; retorna o índice da base
    std::map<std::string, int> counts(int shots); // amostra sem colapsar
    std::string histogram(int shots); // contagens em texto com barras

    // ---- Observáveis ----
    double prob(int q) const;                 // P(q = 1)
    double prob_state(long long basis) const; // |<basis|psi>|^2
    double expect_z(int q) const;             // <Z_q>
    double expect(const std::string& pauli) const; // ex.: "ZZ", "XIY"

    // ---- Visualização e Interoperabilidade ----
    std::string ket(int digits = 4) const;    // 0.7071|00> + 0.7071|11>
    std::string to_string() const { return ket(); }
    std::string probabilities(int digits = 4) const;
    std::string draw() const;                 // circuito em ASCII
    std::string to_qasm() const;              // OpenQASM 3.0
    void display() const;                     // usado por show(q)
    void show() const { display(); }          // q.show() direto no objeto
    std::string bits(long long value) const;  // índice -> "0101"

    // ---- Genéricas ----
    void apply_1q(int q, amp m00, amp m01, amp m10, amp m11, const std::string& label = "U");
    void apply_controlled_1q(int control, int q, amp m00, amp m01, amp m10, amp m11, const std::string& label = "U");

    // ---- Controles extras (circuit nome_ctrl) ----
    // Com a máscara ativa, as portas de 1 e 2 qubits, swap, cswap, rzz e ccx só agem quando todos os
    // qubits da máscara estão em 1. Portas sobre o registrador inteiro (h_all, qft, oráculo,
    // difusor) e medições não podem ser controladas. Normalmente usado via GhostQTape::apply_controlled.
    std::uint64_t control_mask() const { return ctrl_mask_; }
    void set_control_mask(std::uint64_t mask);

private:
    struct Op {
        std::string label;
        std::vector<int> controls;
        std::vector<int> targets;
    };

    int n_;
    mutable std::vector<amp> psi_;   // cópia na CPU (válida quando host_ok_)
    std::mt19937_64 rng_;
    std::vector<Op> log_;
    bool log_truncated_ = false;
    bool recording_ = true;
    bool seeded_ = false;
    void* gpu_ = nullptr;            // estado na GPU (válido quando dev_ok_)
    mutable bool host_ok_ = true;
    bool dev_ok_ = false;
    std::uint64_t ctrl_mask_ = 0;    // controles extras de circuit nome_ctrl
    void check_free_of_controls(std::uint64_t used, const char* what) const;
    void check_not_controlled(const char* what) const;

    void attach_gpu(bool required);
    void release_gpu(bool keep_state);
    void sync_host() const;          // baixa da GPU se a CPU estiver desatualizada
    bool gpu_ready();                // GPU com o estado atual, pronta para a próxima operação
    void host_write();               // a próxima operação altera psi_ na CPU
    void gpu_check(int ok) const;

    void check_qubit(int q, const char* what) const;
    void record(const std::string& label, std::vector<int> controls, std::vector<int> targets);
    void kernel_1q(std::uint64_t control_mask, int q, amp m00, amp m01, amp m10, amp m11);
    double uniform();
};

std::ostream& operator<<(std::ostream& os, const GhostQReg& reg);

// Na GhostLang: print(quantum_backend());  quantum_device("cuda");  (padrão para os próximos qreg/qcircuit)
inline std::string quantum_backend() { return GhostQReg::backend_info(); }
inline void quantum_device(const std::string& name) { GhostQReg::set_default_device(name); }
