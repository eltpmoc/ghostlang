// GhostQML.cpp - Circuitos parametrizados, parameter-shift, VQE e classificador híbrido (F4).
#include "GhostQML.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using amp = GhostQReg::amp;
const amp I(0.0, 1.0);
constexpr double SHIFT = PI / 2.0;

struct Adam {
    std::vector<double> m, v;
    int t = 0;
    double b1 = 0.9, b2 = 0.999, eps = 1e-8;
    explicit Adam(std::size_t n) : m(n, 0.0), v(n, 0.0) {}
    void step(std::vector<double*>& params, const std::vector<double>& g, double lr) {
        ++t;
        const double c1 = 1.0 - std::pow(b1, t), c2 = 1.0 - std::pow(b2, t);
        for (std::size_t i = 0; i < g.size(); ++i) {
            m[i] = b1 * m[i] + (1 - b1) * g[i];
            v[i] = b2 * v[i] + (1 - b2) * g[i] * g[i];
            *params[i] -= lr * (m[i] / c1) / (std::sqrt(v[i] / c2) + eps);
        }
    }
};

std::string num(double v, int digits = 4) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(digits) << v;
    return os.str();
}
} // namespace

GhostQCircuit::GhostQCircuit(int qubits) : n_(qubits), rng_(std::random_device{}()) {
    if (qubits < 1 || qubits > GhostQReg::MAX_QUBITS) {
        throw std::invalid_argument("qcircuit: numero de qubits deve estar entre 1 e " + std::to_string(GhostQReg::MAX_QUBITS));
    }
}

void GhostQCircuit::seed(unsigned long long value) { rng_.seed(value); }

void GhostQCircuit::init_params() {
    std::uniform_real_distribution<double> d(-PI, PI);
    for (auto& t : theta_) t = d(rng_);
    w_ = 1.0; b_ = 0.0;
}

void GhostQCircuit::check_qubit(int q) const {
    if (q < 0 || q >= n_) throw std::out_of_range("qcircuit: qubit " + std::to_string(q) + " fora do circuito de " + std::to_string(n_) + " qubits");
}

int GhostQCircuit::new_param() {
    theta_.push_back(std::uniform_real_distribution<double>(-PI, PI)(rng_));
    return static_cast<int>(theta_.size()) - 1;
}

void GhostQCircuit::add_rot(Kind k, int q, Source src, int index, double value) {
    check_qubit(q);
    if (src == Source::Param && (index < 0 || index >= num_params())) throw std::out_of_range("qcircuit: parametro " + std::to_string(index) + " nao existe");
    if (src == Source::Feature) {
        if (index < 0) throw std::out_of_range("qcircuit: indice de feature negativo");
        features_ = std::max(features_, index + 1);
    }
    ops_.push_back({k, q, -1, src, index, value});
}

// ---------------------------------------------------------------- Construção

void GhostQCircuit::h(int q) { check_qubit(q); ops_.push_back({Kind::H, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::x(int q) { check_qubit(q); ops_.push_back({Kind::X, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::y(int q) { check_qubit(q); ops_.push_back({Kind::Y, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::z(int q) { check_qubit(q); ops_.push_back({Kind::Z, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::s(int q) { check_qubit(q); ops_.push_back({Kind::S, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::t(int q) { check_qubit(q); ops_.push_back({Kind::T, q, -1, Source::Fixed, -1, 0}); }
void GhostQCircuit::cx(int c, int t) {
    check_qubit(c); check_qubit(t);
    if (c == t) throw std::invalid_argument("qcircuit.cx: controle e alvo iguais");
    ops_.push_back({Kind::CX, c, t, Source::Fixed, -1, 0});
}
void GhostQCircuit::cz(int c, int t) {
    check_qubit(c); check_qubit(t);
    if (c == t) throw std::invalid_argument("qcircuit.cz: controle e alvo iguais");
    ops_.push_back({Kind::CZ, c, t, Source::Fixed, -1, 0});
}
void GhostQCircuit::swap(int a, int b) { check_qubit(a); check_qubit(b); ops_.push_back({Kind::SWAP, a, b, Source::Fixed, -1, 0}); }
void GhostQCircuit::rx(int q, double a) { add_rot(Kind::RX, q, Source::Fixed, -1, a); }
void GhostQCircuit::ry(int q, double a) { add_rot(Kind::RY, q, Source::Fixed, -1, a); }
void GhostQCircuit::rz(int q, double a) { add_rot(Kind::RZ, q, Source::Fixed, -1, a); }

int GhostQCircuit::rx_train(int q) { check_qubit(q); int p = new_param(); add_rot(Kind::RX, q, Source::Param, p, 1.0); return p; }
int GhostQCircuit::ry_train(int q) { check_qubit(q); int p = new_param(); add_rot(Kind::RY, q, Source::Param, p, 1.0); return p; }
int GhostQCircuit::rz_train(int q) { check_qubit(q); int p = new_param(); add_rot(Kind::RZ, q, Source::Param, p, 1.0); return p; }
void GhostQCircuit::rx_param(int q, int i) { add_rot(Kind::RX, q, Source::Param, i, 1.0); }
void GhostQCircuit::ry_param(int q, int i) { add_rot(Kind::RY, q, Source::Param, i, 1.0); }
void GhostQCircuit::rz_param(int q, int i) { add_rot(Kind::RZ, q, Source::Param, i, 1.0); }

void GhostQCircuit::ansatz(int layers) {
    if (layers < 1) throw std::invalid_argument("qcircuit.ansatz: use pelo menos 1 camada");
    for (int l = 0; l < layers; ++l) {
        for (int q = 0; q < n_; ++q) { ry_train(q); rz_train(q); }
        for (int q = 0; q + 1 < n_; ++q) cx(q, q + 1);
        if (n_ > 2) cx(n_ - 1, 0);
    }
}

GhostQCircuit GhostQCircuit::inverse_recorder() const {
    GhostQCircuit r(n_);
    r.ops_ = ops_;
    r.theta_ = theta_;
    r.features_ = features_;
    r.rng_ = rng_;
    return r;
}

void GhostQCircuit::append_inverse(const GhostQCircuit& rec, int op_start, int param_start) {
    if (rec.n_ != n_) {
        throw std::invalid_argument("qcircuit: inverso gravado com " + std::to_string(rec.n_) + " qubits, aplicado em " + std::to_string(n_));
    }
    if (op_start < 0 || op_start > rec.num_gates() || param_start < 0 || param_start > rec.num_params() || param_start > num_params()) {
        throw std::invalid_argument("qcircuit: marcas invalidas para o inverso");
    }
    std::vector<int> remap(rec.theta_.size(), -1);
    std::vector<Op> inv;
    for (int i = rec.num_gates() - 1; i >= op_start; --i) {
        Op op = rec.ops_[static_cast<std::size_t>(i)];
        int repeat = 1;
        switch (op.kind) {
            case Kind::S: repeat = 3; break; // S⁴ = I
            case Kind::T: repeat = 7; break; // T⁸ = I
            case Kind::RX: case Kind::RY: case Kind::RZ: case Kind::RZZ:
                op.value = -op.value; // ângulo fixo, ou escala de parâmetro/feature
                break;
            default: break; // H, X, Y, Z, CX, CZ, SWAP: auto-inversas
        }
        if (op.source == Source::Param && op.index >= param_start) {
            int& m = remap[static_cast<std::size_t>(op.index)];
            if (m < 0) {
                theta_.push_back(rec.theta_[static_cast<std::size_t>(op.index)]);
                m = static_cast<int>(theta_.size()) - 1;
            }
            op.index = m;
        }
        for (int k = 0; k < repeat; ++k) inv.push_back(op);
    }
    ops_.insert(ops_.end(), inv.begin(), inv.end());
    features_ = std::max(features_, rec.features_);
}

void GhostQCircuit::encode_rx(int q, int f, double scale) { add_rot(Kind::RX, q, Source::Feature, f, scale); }
void GhostQCircuit::encode_ry(int q, int f, double scale) { add_rot(Kind::RY, q, Source::Feature, f, scale); }
void GhostQCircuit::encode_rz(int q, int f, double scale) { add_rot(Kind::RZ, q, Source::Feature, f, scale); }
void GhostQCircuit::encode_all(double scale) { for (int q = 0; q < n_; ++q) encode_ry(q, q, scale); }

void GhostQCircuit::observable(const std::string& pauli, double coef) {
    if (static_cast<int>(pauli.size()) != n_) {
        throw std::invalid_argument("qcircuit.observable: \"" + pauli + "\" precisa ter " + std::to_string(n_) + " letras (I, X, Y, Z)");
    }
    for (char c : pauli) if (std::string("IXYZixyz").find(c) == std::string::npos) throw std::invalid_argument(std::string("qcircuit.observable: letra invalida '") + c + "'");
    obs_.push_back({coef, pauli});
}
void GhostQCircuit::clear_observable() { obs_.clear(); }

std::string GhostQCircuit::observable_string() const {
    if (obs_.empty()) return "Z(q0)";
    std::ostringstream os;
    for (std::size_t i = 0; i < obs_.size(); ++i) {
        const double c = obs_[i].first;
        if (i) os << (c < 0 ? " - " : " + ");
        else if (c < 0) os << "-";
        os << std::abs(c) << "*" << obs_[i].second;
    }
    return os.str();
}

// ---------------------------------------------------------------- Execução

void GhostQCircuit::run_into(GhostQReg& reg, const std::vector<double>& x, int shift_op, double shift, bool labels) const {
    if (static_cast<int>(x.size()) < features_) {
        throw std::invalid_argument("qcircuit: o circuito usa " + std::to_string(features_) + " features, mas recebeu " + std::to_string(x.size()));
    }
    for (std::size_t k = 0; k < ops_.size(); ++k) {
        const Op& op = ops_[k];
        switch (op.kind) {
            case Kind::H: reg.h(op.a); continue;
            case Kind::X: reg.x(op.a); continue;
            case Kind::Y: reg.y(op.a); continue;
            case Kind::Z: reg.z(op.a); continue;
            case Kind::S: reg.s(op.a); continue;
            case Kind::T: reg.t(op.a); continue;
            case Kind::CX: reg.cx(op.a, op.b); continue;
            case Kind::CZ: reg.cz(op.a, op.b); continue;
            case Kind::SWAP: reg.swap(op.a, op.b); continue;
            default: break;
        }
        double angle = op_angle(op, x);
        if (static_cast<int>(k) == shift_op) angle += shift;
        std::string label;
        const char* name = op.kind == Kind::RX ? "RX" : op.kind == Kind::RY ? "RY" : op.kind == Kind::RZ ? "RZ" : "ZZ";
        if (labels) {
            const std::string sc = (op.value != 1.0 && op.source != Source::Fixed) ? num(op.value, 2) + "*" : "";
            if (op.source == Source::Param) label = std::string(name) + "(" + sc + "t" + std::to_string(op.index) + ")";
            else if (op.source == Source::Feature) label = std::string(name) + "(" + sc + "x" + std::to_string(op.index) + ")";
            else if (op.source == Source::Feature2) label = std::string(name) + "(" + sc + "x" + std::to_string(op.index) + "x" + std::to_string(op.index2) + ")";
            else label = std::string(name) + "(" + num(angle, 2) + ")";
        }
        if (op.kind == Kind::RZZ) { reg.rzz(op.a, op.b, angle, label); continue; }
        const double c = std::cos(angle / 2), s = std::sin(angle / 2);
        if (op.kind == Kind::RX) reg.apply_1q(op.a, c, -I * s, -I * s, c, label);
        else if (op.kind == Kind::RY) reg.apply_1q(op.a, c, -s, s, c, label);
        else reg.apply_1q(op.a, std::polar(1.0, -angle / 2), 0.0, 0.0, std::polar(1.0, angle / 2), label);
    }
}

double GhostQCircuit::measure_obs(const GhostQReg& reg) const {
    if (obs_.empty()) return reg.expect_z(0);
    double e = 0.0;
    for (const auto& term : obs_) e += term.first * reg.expect(term.second);
    return e;
}

double GhostQCircuit::eval(const std::vector<double>& x, int shift_op, double shift) const {
    GhostQReg reg(n_);
    reg.set_recording(false);
    run_into(reg, x, shift_op, shift, false);
    return measure_obs(reg);
}

double GhostQCircuit::expect_vec(const std::vector<double>& x) const { return eval(x, -1, 0.0); }

// Parameter-shift: cada ocorrência de θ_p é deslocada separadamente (regra do produto).
std::vector<double> GhostQCircuit::param_gradient(const std::vector<double>& x) const {
    std::vector<double> g(theta_.size(), 0.0);
    for (std::size_t k = 0; k < ops_.size(); ++k) {
        if (ops_[k].source != Source::Param) continue;
        const double plus = eval(x, static_cast<int>(k), SHIFT);
        const double minus = eval(x, static_cast<int>(k), -SHIFT);
        g[ops_[k].index] += ops_[k].value * 0.5 * (plus - minus); // ângulo = escala * θ
    }
    return g;
}

std::vector<double> GhostQCircuit::gradient_vec(const std::vector<double>& x) const { return param_gradient(x); }

GhostQReg GhostQCircuit::state_vec(const std::vector<double>& x) const {
    GhostQReg reg(n_);
    reg.set_recording(false);
    run_into(reg, x, -1, 0.0, false);
    return reg;
}

// ---------------------------------------------------------------- VQE

double GhostQCircuit::energy() const { return expect_vec(zeros_input()); }

double GhostQCircuit::minimize(int steps, double lr) {
    if (theta_.empty()) throw std::logic_error("qcircuit.minimize: o circuito nao tem parametros treinaveis (use ry_train/ansatz)");
    Adam adam(theta_.size());
    std::vector<double*> ptrs;
    for (auto& t : theta_) ptrs.push_back(&t);
    const std::vector<double> x = zeros_input();
    for (int s = 1; s <= steps; ++s) {
        const std::vector<double> g = param_gradient(x);
        adam.step(ptrs, g, lr);
        if (verbose_ > 0 && (s == 1 || s % verbose_ == 0 || s == steps)) {
            std::cout << "  passo " << s << "/" << steps << "  energia=" << num(energy(), 6) << std::endl;
        }
    }
    return energy();
}

// ---------------------------------------------------------------- Classificador

void GhostQCircuit::add_sample_vec(const std::vector<double>& x, double label) {
    if (static_cast<int>(x.size()) < features_) {
        throw std::invalid_argument("qcircuit.add_sample: amostra com " + std::to_string(x.size()) + " valores, o circuito usa " + std::to_string(features_));
    }
    if (label == 0.0) zero_one_labels_ = true;
    data_x_.push_back(x);
    data_y_.push_back(label > 0 ? 1.0 : -1.0);
}

int GhostQCircuit::load_csv(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("qcircuit.load_csv: nao foi possivel abrir " + path);
    std::string line;
    int added = 0;
    while (std::getline(in, line)) {
        for (char& c : line) if (c == ';') c = ',';
        std::stringstream ss(line);
        std::string cell;
        std::vector<double> row;
        bool ok = true;
        while (std::getline(ss, cell, ',')) {
            try { std::size_t used = 0; row.push_back(std::stod(cell, &used)); }
            catch (...) { ok = false; break; } // cabeçalho ou linha inválida
        }
        if (!ok || row.size() < 2) continue;
        const double label = row.back();
        row.pop_back();
        add_sample_vec(row, label);
        ++added;
    }
    return added;
}

void GhostQCircuit::clear_data() { data_x_.clear(); data_y_.clear(); zero_one_labels_ = false; }

double GhostQCircuit::score_vec(const std::vector<double>& x) const { return w_ * expect_vec(x) + b_; }

int GhostQCircuit::predict_vec(const std::vector<double>& x) const {
    if (score_vec(x) >= 0.0) return 1;
    return zero_one_labels_ ? 0 : -1;
}

double GhostQCircuit::loss() const {
    if (data_x_.empty()) return 0.0;
    double s = 0.0;
    for (std::size_t i = 0; i < data_x_.size(); ++i) { const double d = score_vec(data_x_[i]) - data_y_[i]; s += d * d; }
    return s / data_x_.size();
}

double GhostQCircuit::accuracy() const {
    if (data_x_.empty()) return 0.0;
    int hits = 0;
    for (std::size_t i = 0; i < data_x_.size(); ++i) hits += ((score_vec(data_x_[i]) >= 0.0) == (data_y_[i] > 0)) ? 1 : 0;
    return double(hits) / data_x_.size();
}

double GhostQCircuit::fit(int epochs, double lr) {
    if (data_x_.empty()) throw std::logic_error("qcircuit.fit: nenhuma amostra (use add_sample ou load_csv)");
    if (theta_.empty()) throw std::logic_error("qcircuit.fit: o circuito nao tem parametros treinaveis (use ry_train/ansatz)");
    const std::size_t P = theta_.size();
    Adam adam(P + 2);
    std::vector<double*> ptrs;
    for (auto& t : theta_) ptrs.push_back(&t);
    ptrs.push_back(&w_);
    ptrs.push_back(&b_);
    const long long N = static_cast<long long>(data_x_.size());

    for (int epoch = 1; epoch <= epochs; ++epoch) {
        std::vector<double> grad(P + 2, 0.0);
        double total_loss = 0.0;
        int hits = 0;
        #pragma omp parallel
        {
            std::vector<double> local(P + 2, 0.0);
            double local_loss = 0.0;
            int local_hits = 0;
            #pragma omp for
            for (long long i = 0; i < N; ++i) {
                const auto& x = data_x_[i];
                const double e = expect_vec(x);
                const double f = w_ * e + b_;
                const double d = 2.0 * (f - data_y_[i]) / N;
                const std::vector<double> ge = param_gradient(x);
                for (std::size_t p = 0; p < P; ++p) local[p] += d * w_ * ge[p];
                local[P] += d * e;
                local[P + 1] += d;
                local_loss += (f - data_y_[i]) * (f - data_y_[i]) / N;
                local_hits += ((f >= 0.0) == (data_y_[i] > 0)) ? 1 : 0;
            }
            #pragma omp critical
            {
                for (std::size_t p = 0; p < P + 2; ++p) grad[p] += local[p];
                total_loss += local_loss;
                hits += local_hits;
            }
        }
        if (verbose_ > 0 && (epoch == 1 || epoch % verbose_ == 0 || epoch == epochs)) {
            std::cout << "  epoca " << epoch << "/" << epochs << "  perda=" << num(total_loss) << "  acuracia="
                      << num(100.0 * hits / N, 1) << "%" << std::endl;
        }
        adam.step(ptrs, grad, lr);
    }
    return loss();
}

// ---------------------------------------------------------------- Parâmetros

double GhostQCircuit::param(int i) const {
    if (i < 0 || i >= num_params()) throw std::out_of_range("qcircuit.param: indice invalido");
    return theta_[i];
}
void GhostQCircuit::set_param(int i, double v) {
    if (i < 0 || i >= num_params()) throw std::out_of_range("qcircuit.set_param: indice invalido");
    theta_[i] = v;
}

std::string GhostQCircuit::params() const {
    std::ostringstream os;
    os << "[";
    for (std::size_t i = 0; i < theta_.size(); ++i) os << (i ? ", " : "") << num(theta_[i]);
    os << "] w=" << num(w_) << " b=" << num(b_);
    return os.str();
}

void GhostQCircuit::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("qcircuit.save: nao foi possivel escrever " + path);
    out << "qcircuit 1\nqubits " << n_ << "\nparams " << theta_.size() << "\n" << std::setprecision(17);
    for (double t : theta_) out << t << "\n";
    out << w_ << "\n" << b_ << "\n";
}

void GhostQCircuit::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("qcircuit.load: nao foi possivel abrir " + path);
    std::string tag; int version = 0, qubits = 0; std::size_t count = 0;
    in >> tag >> version;
    std::string k1, k2;
    in >> k1 >> qubits >> k2 >> count;
    if (tag != "qcircuit" || k1 != "qubits" || k2 != "params") throw std::runtime_error("qcircuit.load: arquivo invalido");
    if (qubits != n_ || count != theta_.size()) {
        throw std::runtime_error("qcircuit.load: o arquivo tem " + std::to_string(qubits) + " qubits/" + std::to_string(count) +
                                 " parametros; o circuito tem " + std::to_string(n_) + "/" + std::to_string(theta_.size()));
    }
    for (auto& t : theta_) in >> t;
    in >> w_ >> b_;
    if (!in) throw std::runtime_error("qcircuit.load: arquivo incompleto");
}

// ---------------------------------------------------------------- Visualização

std::string GhostQCircuit::draw() const {
    GhostQReg reg(n_);
    run_into(reg, zeros_input(), -1, 0.0, true);
    return reg.draw();
}

std::string GhostQCircuit::to_string() const {
    std::ostringstream os;
    os << "qcircuit: " << n_ << " qubits, " << ops_.size() << " portas, " << theta_.size() << " parametros, "
       << features_ << " features, " << data_x_.size() << " amostras; observavel " << observable_string();
    return os.str();
}

void GhostQCircuit::display() const { std::cout << to_string() << "\n" << draw(); }

std::ostream& operator<<(std::ostream& os, const GhostQCircuit& c) { return os << c.to_string(); }

// ================================================================ RZZ, parâmetros compartilhados e features de produto

double GhostQCircuit::op_angle(const Op& op, const std::vector<double>& x) const {
    switch (op.source) {
        case Source::Fixed: return op.value;
        case Source::Param: return op.value * theta_[op.index];
        case Source::Feature: return op.value * x[op.index];
        default: return op.value * x[op.index] * x[op.index2];
    }
}

void GhostQCircuit::add_op2(Kind k, int a, int b, Source src, int index, double value, int index2) {
    check_qubit(a);
    check_qubit(b);
    if (a == b) throw std::invalid_argument("qcircuit: porta de 2 qubits precisa de qubits diferentes");
    if (src == Source::Param && (index < 0 || index >= num_params())) throw std::out_of_range("qcircuit: parametro " + std::to_string(index) + " nao existe");
    if (src == Source::Feature || src == Source::Feature2) {
        if (index < 0 || (src == Source::Feature2 && index2 < 0)) throw std::out_of_range("qcircuit: indice de feature negativo");
        features_ = std::max(features_, std::max(index, index2) + 1);
    }
    Op op{k, a, b, src, index, value};
    op.index2 = index2;
    ops_.push_back(op);
}

int GhostQCircuit::add_param(double initial) { theta_.push_back(initial); return num_params() - 1; }
void GhostQCircuit::rx_param_scaled(int q, int i, double scale) { add_rot(Kind::RX, q, Source::Param, i, scale); }
void GhostQCircuit::rzz(int a, int b, double angle) { add_op2(Kind::RZZ, a, b, Source::Fixed, -1, angle); }
int GhostQCircuit::rzz_train(int a, int b) { const int p = new_param(); add_op2(Kind::RZZ, a, b, Source::Param, p, 1.0); return p; }
void GhostQCircuit::rzz_param(int a, int b, int i, double scale) { add_op2(Kind::RZZ, a, b, Source::Param, i, scale); }
void GhostQCircuit::encode_rzz(int a, int b, int fi, int fj, double scale) { add_op2(Kind::RZZ, a, b, Source::Feature2, fi, scale, fj); }

void GhostQCircuit::zz_feature_map(int reps, double scale) {
    if (reps < 1) throw std::invalid_argument("qcircuit.zz_feature_map: use reps >= 1");
    for (int r = 0; r < reps; ++r) {
        for (int q = 0; q < n_; ++q) h(q);
        for (int q = 0; q < n_; ++q) encode_rz(q, q, 2.0 * scale);
        for (int i = 0; i < n_; ++i) for (int j = i + 1; j < n_; ++j) encode_rzz(i, j, i, j, 2.0 * scale);
    }
}

// ================================================================ Kernel quântico

namespace {
// Resolve A x = b para A simétrica definida positiva (Cholesky).
std::vector<double> cholesky_solve(std::vector<std::vector<double>> A, std::vector<double> b) {
    const std::size_t n = A.size();
    for (std::size_t j = 0; j < n; ++j) {
        double d = A[j][j];
        for (std::size_t k = 0; k < j; ++k) d -= A[j][k] * A[j][k];
        if (d <= 0) throw std::runtime_error("qcircuit.kernel_fit: matriz de kernel nao e definida positiva (aumente lambda)");
        A[j][j] = std::sqrt(d);
        for (std::size_t i = j + 1; i < n; ++i) {
            double s = A[i][j];
            for (std::size_t k = 0; k < j; ++k) s -= A[i][k] * A[j][k];
            A[i][j] = s / A[j][j];
        }
    }
    for (std::size_t i = 0; i < n; ++i) { for (std::size_t k = 0; k < i; ++k) b[i] -= A[i][k] * b[k]; b[i] /= A[i][i]; }
    for (std::size_t i = n; i-- > 0;) { for (std::size_t k = i + 1; k < n; ++k) b[i] -= A[k][i] * b[k]; b[i] /= A[i][i]; }
    return b;
}

double state_fidelity(const std::vector<amp>& a, const std::vector<amp>& b) {
    amp s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) s += std::conj(a[i]) * b[i];
    return std::norm(s);
}
} // namespace

double GhostQCircuit::kernel_vec(const std::vector<double>& x, const std::vector<double>& y) const {
    return state_fidelity(state_vec(x).state(), state_vec(y).state());
}

double GhostQCircuit::kernel_split(const std::vector<double>& xy) const {
    if (xy.size() % 2 != 0) throw std::invalid_argument("qcircuit.kernel: passe x e y com o mesmo numero de valores, ex.: kernel(x0, x1, y0, y1)");
    const std::size_t h = xy.size() / 2;
    return kernel_vec(std::vector<double>(xy.begin(), xy.begin() + h), std::vector<double>(xy.begin() + h, xy.end()));
}

double GhostQCircuit::kernel_fit(double lambda) {
    if (data_x_.empty()) throw std::logic_error("qcircuit.kernel_fit: nenhuma amostra (use add_sample)");
    const long long N = static_cast<long long>(data_x_.size());
    kernel_states_.assign(N, {});
    #pragma omp parallel for
    for (long long i = 0; i < N; ++i) kernel_states_[i] = state_vec(data_x_[i]).state();
    std::vector<std::vector<double>> K(N, std::vector<double>(N));
    #pragma omp parallel for
    for (long long i = 0; i < N; ++i)
        for (long long j = 0; j <= i; ++j) K[i][j] = K[j][i] = state_fidelity(kernel_states_[i], kernel_states_[j]);
    for (long long i = 0; i < N; ++i) K[i][i] += lambda;
    alpha_ = cholesky_solve(K, data_y_);
    return kernel_accuracy();
}

double GhostQCircuit::kernel_score_vec(const std::vector<double>& x) const {
    if (alpha_.empty()) throw std::logic_error("qcircuit: chame kernel_fit() antes de kernel_predict/kernel_score");
    const auto psi = state_vec(x).state();
    double s = 0.0;
    for (std::size_t i = 0; i < alpha_.size(); ++i) s += alpha_[i] * state_fidelity(psi, kernel_states_[i]);
    return s;
}

int GhostQCircuit::kernel_predict_vec(const std::vector<double>& x) const {
    if (kernel_score_vec(x) >= 0.0) return 1;
    return zero_one_labels_ ? 0 : -1;
}

double GhostQCircuit::kernel_accuracy() const {
    if (data_x_.empty()) return 0.0;
    int hits = 0;
    for (std::size_t i = 0; i < data_x_.size(); ++i) hits += ((kernel_score_vec(data_x_[i]) >= 0) == (data_y_[i] > 0)) ? 1 : 0;
    return double(hits) / data_x_.size();
}

double GhostQCircuit::kernel_test_accuracy() const {
    if (test_x_.empty()) throw std::logic_error("qcircuit: nenhum dado de teste (use add_test)");
    int hits = 0;
    for (std::size_t i = 0; i < test_x_.size(); ++i) hits += ((kernel_score_vec(test_x_[i]) >= 0) == (test_y_[i] > 0)) ? 1 : 0;
    return double(hits) / test_x_.size();
}

double GhostQCircuit::kernel_alignment() const {
    if (data_x_.empty() || data_y_.empty()) return 0.0;
    const std::size_t N = data_x_.size();
    double ky = 0.0;
    double kk = 0.0;
    double yy = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            double k_ij = kernel_vec(data_x_[i], data_x_[j]);
            double y_ij = data_y_[i] * data_y_[j];
            ky += k_ij * y_ij;
            kk += k_ij * k_ij;
            yy += y_ij * y_ij;
        }
    }
    if (kk <= 0.0 || yy <= 0.0) return 0.0;
    return ky / (std::sqrt(kk) * std::sqrt(yy));
}

std::string GhostQCircuit::kernel_matrix(int max_rows) const {
    const std::size_t m = std::min<std::size_t>(data_x_.size(), static_cast<std::size_t>(std::max(1, max_rows)));
    std::ostringstream os;
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < m; ++j) os << (j ? " " : "") << num(kernel_vec(data_x_[i], data_x_[j]), 3);
        if (i + 1 < m) os << "\n";
    }
    return os.str();
}

void GhostQCircuit::add_test_vec(const std::vector<double>& x, double label) {
    if (static_cast<int>(x.size()) < features_) throw std::invalid_argument("qcircuit.add_test: amostra com poucas features");
    if (label == 0.0) zero_one_labels_ = true;
    test_x_.push_back(x);
    test_y_.push_back(label > 0 ? 1.0 : -1.0);
}

double GhostQCircuit::test_accuracy() const {
    if (test_x_.empty()) throw std::logic_error("qcircuit: nenhum dado de teste (use add_test)");
    int hits = 0;
    for (std::size_t i = 0; i < test_x_.size(); ++i) hits += ((score_vec(test_x_[i]) >= 0) == (test_y_[i] > 0)) ? 1 : 0;
    return double(hits) / test_x_.size();
}

// ================================================================ QAOA (MaxCut)

void GhostQCircuit::edge(int a, int b, double weight) {
    check_qubit(a);
    check_qubit(b);
    if (a == b) throw std::invalid_argument("qcircuit.edge: aresta precisa ligar vertices diferentes");
    edges_.emplace_back(a, b, weight);
}

// Custo: C = soma w (1 - Z_a Z_b)/2. O observável é -C, então minimize() maximiza o corte esperado.
// Camada p: exp(-i γ_p C) ~ RZZ(-w γ_p) em cada aresta; mixer exp(-i β_p ΣX) = RX(2 β_p) em cada qubit.
void GhostQCircuit::qaoa(int layers) {
    if (edges_.empty()) throw std::logic_error("qcircuit.qaoa: defina o grafo com edge(a, b, peso)");
    if (layers < 1) throw std::invalid_argument("qcircuit.qaoa: use pelo menos 1 camada");
    ops_.clear(); theta_.clear(); obs_.clear(); features_ = 0;
    for (int q = 0; q < n_; ++q) h(q);
    for (int l = 0; l < layers; ++l) {
        const double frac = (l + 0.5) / layers;               // início "adiabático": γ cresce, β decresce
        const int g = add_param(0.8 * frac);
        for (const auto& e : edges_) rzz_param(std::get<0>(e), std::get<1>(e), g, -std::get<2>(e));
        const int b = add_param(0.8 * (1.0 - frac));
        for (int q = 0; q < n_; ++q) rx_param_scaled(q, b, 2.0);
    }
    double total = 0.0;
    for (const auto& e : edges_) {
        std::string p(n_, 'I');
        p[n_ - 1 - std::get<0>(e)] = 'Z';
        p[n_ - 1 - std::get<1>(e)] = 'Z';
        observable(p, 0.5 * std::get<2>(e));
        total += std::get<2>(e);
    }
    observable(std::string(n_, 'I'), -0.5 * total);
}

double GhostQCircuit::expected_cut() const { return -energy(); }

double GhostQCircuit::cut_of(const std::string& bits) const {
    if (static_cast<int>(bits.size()) != n_) throw std::invalid_argument("qcircuit.cut_of: a solucao precisa ter " + std::to_string(n_) + " bits");
    double cut = 0.0;
    for (const auto& e : edges_) {
        const char ba = bits[n_ - 1 - std::get<0>(e)], bb = bits[n_ - 1 - std::get<1>(e)];
        if (ba != bb) cut += std::get<2>(e);
    }
    return cut;
}

double GhostQCircuit::maxcut_exact() const {
    if (n_ > 24) throw std::logic_error("qcircuit.maxcut_exact: grande demais para forca bruta (max 24 vertices)");
    double best = 0.0;
    for (long long m = 0; m < (1LL << n_); ++m) {
        double cut = 0.0;
        for (const auto& e : edges_) if (((m >> std::get<0>(e)) & 1) != ((m >> std::get<1>(e)) & 1)) cut += std::get<2>(e);
        best = std::max(best, cut);
    }
    return best;
}

std::string GhostQCircuit::maxcut_solution(int shots) {
    GhostQReg reg = state_vec(zeros_input());
    reg.seed(rng_());
    const auto counts = reg.counts(shots);
    std::string best;
    double best_cut = -1.0;
    int best_count = 0;
    for (const auto& kv : counts) {
        const double c = cut_of(kv.first);
        if (c > best_cut || (c == best_cut && kv.second > best_count)) { best = kv.first; best_cut = c; best_count = kv.second; }
    }
    return best;
}

// ================================================================ GhostQLayer

GhostQLayer::GhostQLayer(int qubits, int layers) : c_(qubits) {
    c_.encode_all();
    c_.ansatz(layers);
}

void GhostQLayer::seed(unsigned long long value) {
    c_.seed(value);
    c_.init_params();
}

std::vector<double> GhostQLayer::z_all(const std::vector<double>& x, int shift_op, double shift) const {
    GhostQReg reg(c_.n_);
    reg.set_recording(false);
    c_.run_into(reg, x, shift_op, shift, false);
    std::vector<double> z(c_.n_);
    for (int q = 0; q < c_.n_; ++q) z[q] = reg.expect_z(q);
    return z;
}

std::vector<double> GhostQLayer::forward(const std::vector<double>& x) const { return z_all(x, -1, 0.0); }

void GhostQLayer::backward(const std::vector<double>& x, const std::vector<double>& gout,
                           std::vector<double>& gin, std::vector<double>& gtheta) const {
    // Cada porta de rotação (treinável ou de codificação) é deslocada em ±π/2.
    for (std::size_t k = 0; k < c_.ops_.size(); ++k) {
        const auto& op = c_.ops_[k];
        if (op.source == GhostQCircuit::Source::Fixed) continue;
        const std::vector<double> plus = z_all(x, static_cast<int>(k), SHIFT);
        const std::vector<double> minus = z_all(x, static_cast<int>(k), -SHIFT);
        double dot = 0.0;
        for (std::size_t i = 0; i < gout.size(); ++i) dot += gout[i] * 0.5 * (plus[i] - minus[i]);
        if (op.source == GhostQCircuit::Source::Param) gtheta[op.index] += op.value * dot; // ângulo = escala * θ
        else if (op.source == GhostQCircuit::Source::Feature) gin[op.index] += op.value * dot; // ângulo = escala * x
        else { // Feature2: ângulo = escala * x_i * x_j
            gin[op.index] += op.value * x[op.index2] * dot;
            gin[op.index2] += op.value * x[op.index] * dot;
        }
    }
}

std::string GhostQLayer::to_string() const {
    return "qlayer: " + std::to_string(c_.n_) + " qubits, " + std::to_string(c_.num_params()) + " parametros, saida <Z> por qubit";
}

void GhostQLayer::display() const { std::cout << to_string() << "\n" << draw(); }

std::ostream& operator<<(std::ostream& os, const GhostQLayer& layer) { return os << layer.to_string(); }

// ---------------------------------------------------------------------------
// OpenQASM 3.0 Interoperabilidade (GhostQCircuit)
// ---------------------------------------------------------------------------
std::string GhostQCircuit::to_qasm() const {
    std::ostringstream ss;
    ss << "OPENQASM 3.0;\n";
    ss << "include \"stdgates.inc\";\n";
    ss << "qubit[" << n_ << "] q;\n";
    for (const auto& op : ops_) {
        double val = op.value;
        if (op.source == Source::Param && op.index >= 0 && op.index < static_cast<int>(theta_.size())) {
            val = op.value * theta_[op.index];
        }
        switch (op.kind) {
            case Kind::H: ss << "h q[" << op.a << "];\n"; break;
            case Kind::X: ss << "x q[" << op.a << "];\n"; break;
            case Kind::Y: ss << "y q[" << op.a << "];\n"; break;
            case Kind::Z: ss << "z q[" << op.a << "];\n"; break;
            case Kind::S: ss << "s q[" << op.a << "];\n"; break;
            case Kind::T: ss << "t q[" << op.a << "];\n"; break;
            case Kind::CX: ss << "cx q[" << op.a << "], q[" << op.b << "];\n"; break;
            case Kind::CZ: ss << "cz q[" << op.a << "], q[" << op.b << "];\n"; break;
            case Kind::SWAP: ss << "swap q[" << op.a << "], q[" << op.b << "];\n"; break;
            case Kind::RX: ss << "rx(" << val << ") q[" << op.a << "];\n"; break;
            case Kind::RY: ss << "ry(" << val << ") q[" << op.a << "];\n"; break;
            case Kind::RZ: ss << "rz(" << val << ") q[" << op.a << "];\n"; break;
            case Kind::RZZ: ss << "rzz(" << val << ") q[" << op.a << "], q[" << op.b << "];\n"; break;
        }
    }
    return ss.str();
}

GhostQCircuit GhostQCircuit::from_qasm(const std::string& qasm) {
    std::stringstream ss(qasm);
    std::string line;
    int numQubits = 1;

    std::vector<std::string> lines;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos) {
            line = line.substr(start);
            lines.push_back(line);
            size_t qbPos = line.find("qubit[");
            if (qbPos == std::string::npos) qbPos = line.find("qreg ");
            if (qbPos != std::string::npos) {
                size_t bStart = line.find('[', qbPos);
                size_t bEnd = line.find(']', bStart);
                if (bStart != std::string::npos && bEnd != std::string::npos) {
                    try {
                        numQubits = std::max(numQubits, std::stoi(line.substr(bStart + 1, bEnd - bStart - 1)));
                    } catch (...) {}
                }
            }
        }
    }

    GhostQCircuit c(numQubits);

    auto extractQubit = [](const std::string& s) -> int {
        size_t b1 = s.find('[');
        size_t b2 = s.find(']', b1);
        if (b1 != std::string::npos && b2 != std::string::npos) {
            try { return std::stoi(s.substr(b1 + 1, b2 - b1 - 1)); } catch (...) {}
        }
        return 0;
    };

    auto extractAngle = [](const std::string& s) -> double {
        size_t p1 = s.find('(');
        size_t p2 = s.find(')', p1);
        if (p1 != std::string::npos && p2 != std::string::npos) {
            try { return std::stod(s.substr(p1 + 1, p2 - p1 - 1)); } catch (...) {}
        }
        return 0.0;
    };

    for (const auto& l : lines) {
        if (l.rfind("OPENQASM", 0) == 0 || l.rfind("include", 0) == 0 ||
            l.rfind("qubit[", 0) == 0 || l.rfind("qreg", 0) == 0) {
            continue;
        }

        if (l.rfind("h ", 0) == 0) {
            c.h(extractQubit(l));
        } else if (l.rfind("x ", 0) == 0) {
            c.x(extractQubit(l));
        } else if (l.rfind("y ", 0) == 0) {
            c.y(extractQubit(l));
        } else if (l.rfind("z ", 0) == 0) {
            c.z(extractQubit(l));
        } else if (l.rfind("s ", 0) == 0) {
            c.s(extractQubit(l));
        } else if (l.rfind("t ", 0) == 0) {
            c.t(extractQubit(l));
        } else if (l.rfind("cx ", 0) == 0 || l.rfind("cnot ", 0) == 0) {
            size_t comma = l.find(',');
            if (comma != std::string::npos) {
                c.cx(extractQubit(l.substr(0, comma)), extractQubit(l.substr(comma + 1)));
            }
        } else if (l.rfind("cz ", 0) == 0) {
            size_t comma = l.find(',');
            if (comma != std::string::npos) {
                c.cz(extractQubit(l.substr(0, comma)), extractQubit(l.substr(comma + 1)));
            }
        } else if (l.rfind("swap ", 0) == 0) {
            size_t comma = l.find(',');
            if (comma != std::string::npos) {
                c.swap(extractQubit(l.substr(0, comma)), extractQubit(l.substr(comma + 1)));
            }
        } else if (l.rfind("rx", 0) == 0) {
            c.rx(extractQubit(l), extractAngle(l));
        } else if (l.rfind("ry", 0) == 0) {
            c.ry(extractQubit(l), extractAngle(l));
        } else if (l.rfind("rz", 0) == 0) {
            c.rz(extractQubit(l), extractAngle(l));
        } else if (l.rfind("rzz", 0) == 0) {
            size_t comma = l.find(',');
            if (comma != std::string::npos) {
                c.rzz(extractQubit(l.substr(0, comma)), extractQubit(l.substr(comma + 1)), extractAngle(l));
            }
        }
    }

    return c;
}
