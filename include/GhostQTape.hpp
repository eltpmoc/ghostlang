#pragma once
// GhostQTape.hpp - gravador de portas usado para inverter blocos circuit { } (circuit nome_dg).
//
// Um circuit é executado sobre uma GhostQTape (que só anota as portas, sem simular nada) e
// depois as portas inversas são aplicadas em ordem reversa no registrador de verdade:
//   (G1 G2 ... Gk)† = Gk† ... G2† G1†
// Os inversos são exatos, inclusive a fase global: S† = Sdg, T† = Tdg, SX† = SX·SX·SX,
// U(θ,φ,λ)† = U(-θ,-λ,-φ), rotações e fases com o ângulo negado, QFT† = IQFT.
// Medição e reset não são reversíveis e geram erro dentro de um circuit invertido.
//
// Para qcircuit o gravador é uma cópia leve do próprio circuito (GhostQCircuit::inverse_recorder),
// e o inverso nega ângulos fixos, escalas de parâmetros e de features (GhostQCircuit::append_inverse).
#include "GhostQML.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class GhostQTape {
public:
    explicit GhostQTape(int qubits = 1);

    int size() const { return n_; }
    int num_gates() const { return static_cast<int>(ops_.size()); }

    // Mesmas portas do GhostQReg (apenas anotadas)
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
    void p(int q, double phi);
    void u(int q, double theta, double phi, double lambda);
    void h_all();
    void x_all();
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
    void rzz(int a, int b, double theta, const std::string& label = "");
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
    void qft();
    void iqft();
    void qft_range(int start, int count);
    void iqft_range(int start, int count);
    void phase_flip(long long basis);
    void diffuser();

    // Não reversíveis: lançam erro
    int measure(int q);
    long long measure_all();
    void reset();

    void apply(GhostQReg& reg) const;          // repete as portas gravadas
    void apply_inverse(GhostQReg& reg) const;  // aplica o inverso (ordem reversa)
    void apply_inverse(GhostQTape& tape) const; // grava o inverso em outro gravador (inverso aninhado)

    // circuit nome_ctrl: aplica as portas gravadas só quando o qubit `control` está em 1.
    // Tudo é validado antes de aplicar (nada muda em caso de erro): nenhuma porta pode usar o
    // controle, e portas sobre o registrador inteiro (h_all, x_all, qft(), iqft(), phase_flip,
    // diffuser) não podem ser controladas; use qft_range/iqft_range em vez de qft()/iqft().
    void apply_controlled(GhostQReg& reg, int control) const;
    void apply_controlled(GhostQTape& tape, int control) const; // controlado dentro de outro gravador
    std::uint64_t control_mask() const { return ctrl_mask_; }
    void set_control_mask(std::uint64_t mask);

    enum class Kind {
        H, X, Y, Z, S, SDG, T, TDG, SX, RX, RY, RZ, P, U, H_ALL, X_ALL,
        CX, CY, CZ, CH, SWAP, CP, CRX, CRY, CRZ, RZZ, CCX, CSWAP, MCX,
        QFT, IQFT, QFT_RANGE, IQFT_RANGE, PHASE_FLIP, DIFFUSER
    };
    struct Op {
        Kind kind;
        int a = -1, b = -1, c = -1;
        double v0 = 0, v1 = 0, v2 = 0;
        long long basis = 0;
        std::uint64_t cmask = 0; // controles extras (circuit controlado gravado)
    };
    const std::vector<Op>& ops() const { return ops_; }

private:
    int n_;
    std::vector<Op> ops_;
    std::uint64_t ctrl_mask_ = 0;
    void check(int q) const;
    void add(Op op); // grava com os controles extras atuais

public:
    static const char* kind_name(Kind k);
    std::uint64_t qubits_of(const Op& op) const; // qubits usados pela porta (inclusive controles)
    static bool whole_register(Kind k);           // atua em todos os qubits
};

// ---- Usados pelo código gerado para circuit nome_dg (o mesmo template serve para os três tipos) ----
inline GhostQTape ghost_begin_inverse(const GhostQReg& q) { return GhostQTape(q.size()); }
inline GhostQTape ghost_begin_inverse(const GhostQTape& t) { return GhostQTape(t.size()); }
inline GhostQCircuit ghost_begin_inverse(const GhostQCircuit& c) { return c.inverse_recorder(); }
inline void ghost_end_inverse(GhostQReg& q, const GhostQTape& rec) { rec.apply_inverse(q); }
inline void ghost_end_inverse(GhostQTape& q, const GhostQTape& rec) { rec.apply_inverse(q); }
inline void ghost_end_inverse(GhostQCircuit& c, const GhostQCircuit& rec) { c.append_inverse(rec, c.num_gates(), c.num_params()); }

// ---- Usados pelo código gerado para circuit nome_ctrl / nome_dg_ctrl ----
inline GhostQTape ghost_begin_record(const GhostQReg& q) { return GhostQTape(q.size()); }
inline GhostQTape ghost_begin_record(const GhostQTape& t) { return GhostQTape(t.size()); }
inline GhostQCircuit ghost_begin_record(const GhostQCircuit& c) { return c.inverse_recorder(); }
template <class C> void ghost_end_controlled(GhostQReg& q, const GhostQTape& rec, C control) { rec.apply_controlled(q, static_cast<int>(control)); }
template <class C> void ghost_end_controlled(GhostQTape& q, const GhostQTape& rec, C control) { rec.apply_controlled(q, static_cast<int>(control)); }
template <class C> void ghost_end_controlled(GhostQCircuit&, const GhostQCircuit&, C) {
    throw std::runtime_error("circuit controlado (nome_ctrl) ainda nao e suportado em qcircuit; use em um qreg");
}
