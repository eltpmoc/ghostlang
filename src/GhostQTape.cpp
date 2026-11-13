// GhostQTape.cpp - gravador de portas e inverso exato de circuits (ver include/GhostQTape.hpp).
#include "../include/GhostQTape.hpp"

#include <stdexcept>

using K = GhostQTape::Kind;

GhostQTape::GhostQTape(int qubits) : n_(qubits) {
    if (qubits < 1 || qubits > GhostQReg::MAX_QUBITS) {
        throw std::invalid_argument("circuit inverso: numero de qubits deve estar entre 1 e " + std::to_string(GhostQReg::MAX_QUBITS));
    }
}

void GhostQTape::check(int q) const {
    if (q < 0 || q >= n_) throw std::out_of_range("qreg: qubit " + std::to_string(q) + " fora do registrador de " + std::to_string(n_) + " qubits");
}

const char* GhostQTape::kind_name(Kind k) {
    static const char* names[] = {"h", "x", "y", "z", "s", "sdg", "t", "tdg", "sx", "rx", "ry", "rz", "p", "u", "h_all", "x_all",
                                  "cx", "cy", "cz", "ch", "swap", "cp", "crx", "cry", "crz", "rzz", "ccx", "cswap", "mcx",
                                  "qft", "iqft", "qft_range", "iqft_range", "phase_flip", "diffuser"};
    return names[static_cast<int>(k)];
}

bool GhostQTape::whole_register(Kind k) {
    return k == Kind::H_ALL || k == Kind::X_ALL || k == Kind::QFT || k == Kind::IQFT || k == Kind::PHASE_FLIP || k == Kind::DIFFUSER;
}

std::uint64_t GhostQTape::qubits_of(const Op& o) const {
    const std::uint64_t all = n_ >= 64 ? ~0ULL : ((1ULL << n_) - 1);
    std::uint64_t m = o.cmask;
    if (whole_register(o.kind)) return all;
    if (o.kind == Kind::QFT_RANGE || o.kind == Kind::IQFT_RANGE) {
        return m | ((o.b >= 64 ? ~0ULL : ((1ULL << o.b) - 1)) << o.a);
    }
    if (o.kind == Kind::MCX) return o.cmask | (1ULL << o.a);
    if (o.a >= 0) m |= 1ULL << o.a;
    if (o.b >= 0) m |= 1ULL << o.b;
    if (o.c >= 0) m |= 1ULL << o.c;
    return m;
}

void GhostQTape::set_control_mask(std::uint64_t mask) {
    if (n_ < 64 && (mask >> n_) != 0) throw std::out_of_range("qreg: qubit de controle fora do registrador de " + std::to_string(n_) + " qubits");
    ctrl_mask_ = mask;
}

void GhostQTape::add(Op op) {
    if (ctrl_mask_) {
        if (whole_register(op.kind)) {
            throw std::invalid_argument(std::string("circuit controlado: ") + kind_name(op.kind) + " atua no registrador inteiro (inclusive no controle) e nao pode ser controlado");
        }
        if (qubits_of(op) & ctrl_mask_) {
            throw std::invalid_argument(std::string("circuit controlado: ") + kind_name(op.kind) + " usa o qubit de controle");
        }
        op.cmask |= ctrl_mask_;
    }
    ops_.push_back(op);
}

void GhostQTape::h(int q) { check(q); add({K::H, q}); }
void GhostQTape::x(int q) { check(q); add({K::X, q}); }
void GhostQTape::y(int q) { check(q); add({K::Y, q}); }
void GhostQTape::z(int q) { check(q); add({K::Z, q}); }
void GhostQTape::s(int q) { check(q); add({K::S, q}); }
void GhostQTape::sdg(int q) { check(q); add({K::SDG, q}); }
void GhostQTape::t(int q) { check(q); add({K::T, q}); }
void GhostQTape::tdg(int q) { check(q); add({K::TDG, q}); }
void GhostQTape::sx(int q) { check(q); add({K::SX, q}); }
void GhostQTape::rx(int q, double th) { check(q); add({K::RX, q, -1, -1, th}); }
void GhostQTape::ry(int q, double th) { check(q); add({K::RY, q, -1, -1, th}); }
void GhostQTape::rz(int q, double th) { check(q); add({K::RZ, q, -1, -1, th}); }
void GhostQTape::p(int q, double phi) { check(q); add({K::P, q, -1, -1, phi}); }
void GhostQTape::u(int q, double th, double phi, double lam) { check(q); add({K::U, q, -1, -1, th, phi, lam}); }
void GhostQTape::h_all() { add({K::H_ALL}); }
void GhostQTape::x_all() { add({K::X_ALL}); }

namespace {
void check_pair(int a, int b, const char* what) {
    if (a == b) throw std::invalid_argument(std::string("qreg.") + what + ": qubits iguais");
}
} // namespace

void GhostQTape::cx(int c, int t) { check(c); check(t); check_pair(c, t, "cx"); add({K::CX, c, t}); }
void GhostQTape::cy(int c, int t) { check(c); check(t); check_pair(c, t, "cy"); add({K::CY, c, t}); }
void GhostQTape::cz(int c, int t) { check(c); check(t); check_pair(c, t, "cz"); add({K::CZ, c, t}); }
void GhostQTape::ch(int c, int t) { check(c); check(t); check_pair(c, t, "ch"); add({K::CH, c, t}); }
void GhostQTape::cp(int c, int t, double v) { check(c); check(t); check_pair(c, t, "cp"); add({K::CP, c, t, -1, v}); }
void GhostQTape::crx(int c, int t, double v) { check(c); check(t); check_pair(c, t, "crx"); add({K::CRX, c, t, -1, v}); }
void GhostQTape::cry(int c, int t, double v) { check(c); check(t); check_pair(c, t, "cry"); add({K::CRY, c, t, -1, v}); }
void GhostQTape::crz(int c, int t, double v) { check(c); check(t); check_pair(c, t, "crz"); add({K::CRZ, c, t, -1, v}); }
void GhostQTape::swap(int a, int b) { check(a); check(b); add({K::SWAP, a, b}); }
void GhostQTape::rzz(int a, int b, double v, const std::string&) { check(a); check(b); check_pair(a, b, "rzz"); add({K::RZZ, a, b, -1, v}); }
void GhostQTape::ccx(int c1, int c2, int t) { check(c1); check(c2); check(t); add({K::CCX, c1, c2, t}); }
void GhostQTape::cswap(int c, int a, int b) { check(c); check(a); check(b); add({K::CSWAP, c, a, b}); }
void GhostQTape::mcx(const std::vector<int>& controls, int t) {
    check(t);
    std::uint64_t mask = 0;
    for (int c : controls) {
        check(c);
        if (c == t) throw std::invalid_argument("qreg.mcx: o alvo nao pode ser controle");
        if (mask & (1ULL << c)) throw std::invalid_argument("qreg.mcx: controles duplicados");
        mask |= (1ULL << c);
    }
    Op op{K::MCX, t, -1, -1, 0, 0, 0, 0, mask};
    add(op);
}
void GhostQTape::qft() { add({K::QFT}); }
void GhostQTape::iqft() { add({K::IQFT}); }
void GhostQTape::qft_range(int start, int count) { check(start); check(start + count - 1); add({K::QFT_RANGE, start, count}); }
void GhostQTape::iqft_range(int start, int count) { check(start); check(start + count - 1); add({K::IQFT_RANGE, start, count}); }
void GhostQTape::phase_flip(long long basis) {
    if (basis < 0 || basis >= (1LL << n_)) throw std::out_of_range("qreg.phase_flip: estado fora do registrador");
    Op op{K::PHASE_FLIP};
    op.basis = basis;
    add(op);
}
void GhostQTape::diffuser() { add({K::DIFFUSER}); }

int GhostQTape::measure(int) { throw std::runtime_error("circuit inverso: measure nao e reversivel (tire a medicao do circuit para usar nome_dg)"); }
long long GhostQTape::measure_all() { throw std::runtime_error("circuit inverso: measure_all nao e reversivel (tire a medicao do circuit para usar nome_dg)"); }
void GhostQTape::reset() { throw std::runtime_error("circuit inverso: reset nao e reversivel"); }

namespace {
// Liga controles extras em um GhostQReg/GhostQTape e restaura ao sair (inclusive com exceção).
template <class R>
struct ControlScope {
    R& r;
    std::uint64_t old;
    ControlScope(R& reg, std::uint64_t extra) : r(reg), old(reg.control_mask()) { if (extra) r.set_control_mask(old | extra); }
    ~ControlScope() { r.set_control_mask(old); }
};

template <class R>
void play_raw(R& r, const GhostQTape::Op& o);

// Aplica uma porta gravada em um GhostQReg ou em outro GhostQTape (mesmos nomes de método),
// com os controles extras que ela tinha quando foi gravada.
template <class R>
void play(R& r, const GhostQTape::Op& o) {
    if (!o.cmask) { play_raw(r, o); return; }
    ControlScope<R> scope(r, o.cmask);
    play_raw(r, o);
}

template <class R>
void play_raw(R& r, const GhostQTape::Op& o) {
    switch (o.kind) {
        case K::H: r.h(o.a); break;
        case K::X: r.x(o.a); break;
        case K::Y: r.y(o.a); break;
        case K::Z: r.z(o.a); break;
        case K::S: r.s(o.a); break;
        case K::SDG: r.sdg(o.a); break;
        case K::T: r.t(o.a); break;
        case K::TDG: r.tdg(o.a); break;
        case K::SX: r.sx(o.a); break;
        case K::RX: r.rx(o.a, o.v0); break;
        case K::RY: r.ry(o.a, o.v0); break;
        case K::RZ: r.rz(o.a, o.v0); break;
        case K::P: r.p(o.a, o.v0); break;
        case K::U: r.u(o.a, o.v0, o.v1, o.v2); break;
        case K::H_ALL: r.h_all(); break;
        case K::X_ALL: r.x_all(); break;
        case K::CX: r.cx(o.a, o.b); break;
        case K::CY: r.cy(o.a, o.b); break;
        case K::CZ: r.cz(o.a, o.b); break;
        case K::CH: r.ch(o.a, o.b); break;
        case K::SWAP: r.swap(o.a, o.b); break;
        case K::CP: r.cp(o.a, o.b, o.v0); break;
        case K::CRX: r.crx(o.a, o.b, o.v0); break;
        case K::CRY: r.cry(o.a, o.b, o.v0); break;
        case K::CRZ: r.crz(o.a, o.b, o.v0); break;
        case K::RZZ: r.rzz(o.a, o.b, o.v0); break;
        case K::CCX: r.ccx(o.a, o.b, o.c); break;
        case K::CSWAP: r.cswap(o.a, o.b, o.c); break;
        case K::MCX: {
            std::vector<int> ctls;
            for (int bit = 0; bit < 64; ++bit) if (o.cmask & (1ULL << bit)) ctls.push_back(bit);
            r.mcx(ctls, o.a);
            break;
        }
        case K::QFT: r.qft(); break;
        case K::IQFT: r.iqft(); break;
        case K::QFT_RANGE: r.qft_range(o.a, o.b); break;
        case K::IQFT_RANGE: r.iqft_range(o.a, o.b); break;
        case K::PHASE_FLIP: r.phase_flip(o.basis); break;
        case K::DIFFUSER: r.diffuser(); break;
    }
}

// Inverso exato de uma porta (pode virar mais de uma porta, ex.: SX† = SX³).
template <class R>
void play_inverse(R& r, const GhostQTape::Op& o) {
    GhostQTape::Op inv = o;
    switch (o.kind) {
        case K::S: inv.kind = K::SDG; break;
        case K::SDG: inv.kind = K::S; break;
        case K::T: inv.kind = K::TDG; break;
        case K::TDG: inv.kind = K::T; break;
        case K::SX: play(r, o); play(r, o); break; // SX⁴ = I  =>  SX† = SX³
        case K::RX: case K::RY: case K::RZ: case K::P:
        case K::CP: case K::CRX: case K::CRY: case K::CRZ: case K::RZZ:
            inv.v0 = -o.v0; break;
        case K::U: inv.v0 = -o.v0; inv.v1 = -o.v2; inv.v2 = -o.v1; break;
        case K::QFT: inv.kind = K::IQFT; break;
        case K::IQFT: inv.kind = K::QFT; break;
        case K::QFT_RANGE: inv.kind = K::IQFT_RANGE; break;
        case K::IQFT_RANGE: inv.kind = K::QFT_RANGE; break;
        default: break; // H, X, Y, Z, CX, CY, CZ, CH, SWAP, CCX, CSWAP, oráculo, difusor: auto-inversas
    }
    play(r, inv);
}

template <class R>
void check_size(const GhostQTape& tape, const R& r) {
    if (r.size() != tape.size()) {
        throw std::invalid_argument("circuit inverso: gravado com " + std::to_string(tape.size()) + " qubits, aplicado em " + std::to_string(r.size()));
    }
}
} // namespace

void GhostQTape::apply(GhostQReg& reg) const {
    check_size(*this, reg);
    for (const auto& o : ops_) play(reg, o);
}

void GhostQTape::apply_inverse(GhostQReg& reg) const {
    check_size(*this, reg);
    for (auto it = ops_.rbegin(); it != ops_.rend(); ++it) play_inverse(reg, *it);
}

void GhostQTape::apply_inverse(GhostQTape& tape) const {
    check_size(*this, tape);
    for (auto it = ops_.rbegin(); it != ops_.rend(); ++it) play_inverse(tape, *it);
}

namespace {
template <class R>
void apply_controlled_impl(const GhostQTape& tape, R& r, int control) {
    check_size(tape, r);
    if (control < 0 || control >= tape.size()) {
        throw std::out_of_range("circuit controlado: qubit de controle " + std::to_string(control) + " fora do registrador de " + std::to_string(tape.size()) + " qubits");
    }
    const std::uint64_t cm = 1ULL << control;
    // Valida tudo antes: em caso de erro o registrador não muda.
    for (const auto& o : tape.ops()) {
        if (GhostQTape::whole_register(o.kind)) {
            throw std::invalid_argument(std::string("circuit controlado: ") + GhostQTape::kind_name(o.kind) +
                                        " atua no registrador inteiro (inclusive no controle) e nao pode ser controlado" +
                                        (o.kind == K::QFT || o.kind == K::IQFT ? "; use qft_range/iqft_range sem o qubit de controle" : ""));
        }
        if (tape.qubits_of(o) & cm) {
            throw std::invalid_argument(std::string("circuit controlado: ") + GhostQTape::kind_name(o.kind) + " usa o qubit " +
                                        std::to_string(control) + ", que e o controle");
        }
    }
    ControlScope<R> scope(r, cm);
    for (const auto& o : tape.ops()) play(r, o);
}
} // namespace

void GhostQTape::apply_controlled(GhostQReg& reg, int control) const { apply_controlled_impl(*this, reg, control); }
void GhostQTape::apply_controlled(GhostQTape& tape, int control) const { apply_controlled_impl(*this, tape, control); }
