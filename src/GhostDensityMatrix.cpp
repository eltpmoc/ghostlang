#include "../include/GhostDensityMatrix.hpp"

#include <cmath>
#include <map>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace GhostQuantum {

GhostDensityMatrix::GhostDensityMatrix(int qubits)
    : num_qubits(qubits), dim(1ULL << qubits), data(dim * dim, Complex(0.0, 0.0)) {
    // Estado fundamental |0...0><0...0|
    data[0] = Complex(1.0, 0.0);
}

GhostDensityMatrix GhostDensityMatrix::from_statevector(const GhostQReg& qreg) {
    GhostDensityMatrix rho(qreg.size());
    std::vector<Complex> psi(rho.dim);
    for (size_t i = 0; i < rho.dim; ++i) {
        psi[i] = qreg.amplitude(static_cast<int>(i));
    }
    for (size_t r = 0; r < rho.dim; ++r) {
        for (size_t c = 0; c < rho.dim; ++c) {
            rho.set(r, c, psi[r] * std::conj(psi[c]));
        }
    }
    return rho;
}

GhostDensityMatrix GhostDensityMatrix::maximally_mixed(int qubits) {
    GhostDensityMatrix rho(qubits);
    double diag_val = 1.0 / static_cast<double>(rho.dim);
    for (size_t i = 0; i < rho.dim; ++i) {
        rho.set(i, i, Complex(diag_val, 0.0));
    }
    return rho;
}

double GhostDensityMatrix::trace() const {
    double tr = 0.0;
    for (size_t i = 0; i < dim; ++i) {
        tr += at(i, i).real();
    }
    return tr;
}

double GhostDensityMatrix::purity() const {
    // Tr(rho^2) = sum_{i,j} |rho_{i,j}|^2
    double p = 0.0;
    for (size_t i = 0; i < dim * dim; ++i) {
        p += std::norm(data[i]);
    }
    return p;
}

std::vector<double> GhostDensityMatrix::probabilities() const {
    std::vector<double> probs(dim);
    for (size_t i = 0; i < dim; ++i) {
        probs[i] = std::max(0.0, at(i, i).real());
    }
    return probs;
}

void GhostDensityMatrix::apply_gate_1q(int qubit, const Complex U[4]) {
    // Aplicação de U no qubit: U * rho * U^\dagger
    // 1. Multiplica pela esquerda: rho_temp = U * rho
    std::vector<Complex> temp(dim * dim, Complex(0.0, 0.0));
    size_t bit = 1ULL << qubit;

    for (size_t i = 0; i < dim; ++i) {
        size_t i0 = i & ~bit;
        size_t i1 = i | bit;
        if (i == i0) {
            for (size_t j = 0; j < dim; ++j) {
                Complex r0 = at(i0, j);
                Complex r1 = at(i1, j);
                temp[i0 * dim + j] = U[0] * r0 + U[1] * r1;
                temp[i1 * dim + j] = U[2] * r0 + U[3] * r1;
            }
        }
    }

    // 2. Multiplica pela direita por U^\dagger: rho = temp * U^\dagger
    // U^\dagger = [ conj(U[0]), conj(U[2]) ]
    //             [ conj(U[1]), conj(U[3]) ]
    Complex Ud[4] = {
        std::conj(U[0]), std::conj(U[2]),
        std::conj(U[1]), std::conj(U[3])
    };

    for (size_t j = 0; j < dim; ++j) {
        size_t j0 = j & ~bit;
        size_t j1 = j | bit;
        if (j == j0) {
            for (size_t i = 0; i < dim; ++i) {
                Complex t0 = temp[i * dim + j0];
                Complex t1 = temp[i * dim + j1];
                set(i, j0, t0 * Ud[0] + t1 * Ud[2]);
                set(i, j1, t0 * Ud[1] + t1 * Ud[3]);
            }
        }
    }
}

void GhostDensityMatrix::apply_h(int qubit) {
    double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    Complex H[4] = {
        Complex(inv_sqrt2, 0.0), Complex(inv_sqrt2, 0.0),
        Complex(inv_sqrt2, 0.0), Complex(-inv_sqrt2, 0.0)
    };
    apply_gate_1q(qubit, H);
}

void GhostDensityMatrix::apply_x(int qubit) {
    Complex X[4] = {
        Complex(0.0, 0.0), Complex(1.0, 0.0),
        Complex(1.0, 0.0), Complex(0.0, 0.0)
    };
    apply_gate_1q(qubit, X);
}

void GhostDensityMatrix::apply_z(int qubit) {
    Complex Z[4] = {
        Complex(1.0, 0.0), Complex(0.0, 0.0),
        Complex(0.0, 0.0), Complex(-1.0, 0.0)
    };
    apply_gate_1q(qubit, Z);
}

void GhostDensityMatrix::apply_cnot(int ctrl, int target) {
    // Permutação CNOT sobre base computacional
    size_t cbit = 1ULL << ctrl;
    size_t tbit = 1ULL << target;
    std::vector<Complex> next(dim * dim, Complex(0.0, 0.0));

    for (size_t r = 0; r < dim; ++r) {
        size_t r_mapped = ((r & cbit) != 0) ? (r ^ tbit) : r;
        for (size_t c = 0; c < dim; ++c) {
            size_t c_mapped = ((c & cbit) != 0) ? (c ^ tbit) : c;
            next[r_mapped * dim + c_mapped] = at(r, c);
        }
    }
    data = std::move(next);
}

void GhostDensityMatrix::apply_kraus(int qubit, const std::vector<std::vector<Complex>>& kraus_ops) {
    std::vector<Complex> sum(dim * dim, Complex(0.0, 0.0));

    for (const auto& K : kraus_ops) {
        if (K.size() != 4) continue;
        Complex U[4] = { K[0], K[1], K[2], K[3] };
        GhostDensityMatrix copy = *this;
        copy.apply_gate_1q(qubit, U);
        for (size_t i = 0; i < dim * dim; ++i) {
            sum[i] += copy.data[i];
        }
    }
    data = std::move(sum);
}

void GhostDensityMatrix::depolarizing_channel(int qubit, double p) {
    // E(rho) = (1 - p)*rho + (p/3)*(X rho X + Y rho Y + Z rho Z)
    if (p <= 0.0) return;
    if (p > 1.0) p = 1.0;

    GhostDensityMatrix rho_x = *this;
    GhostDensityMatrix rho_y = *this;
    GhostDensityMatrix rho_z = *this;

    rho_x.apply_x(qubit);

    Complex Y[4] = {
        Complex(0.0, 0.0), Complex(0.0, -1.0),
        Complex(0.0, 1.0), Complex(0.0, 0.0)
    };
    rho_y.apply_gate_1q(qubit, Y);

    rho_z.apply_z(qubit);

    double factor = p / 3.0;
    double keep = 1.0 - p;

    for (size_t i = 0; i < dim * dim; ++i) {
        data[i] = keep * data[i] + factor * (rho_x.data[i] + rho_y.data[i] + rho_z.data[i]);
    }
}

void GhostDensityMatrix::bit_flip_channel(int qubit, double p) {
    if (p <= 0.0) return;
    GhostDensityMatrix rho_x = *this;
    rho_x.apply_x(qubit);
    double keep = 1.0 - p;
    for (size_t i = 0; i < dim * dim; ++i) {
        data[i] = keep * data[i] + p * rho_x.data[i];
    }
}

void GhostDensityMatrix::phase_flip_channel(int qubit, double p) {
    if (p <= 0.0) return;
    GhostDensityMatrix rho_z = *this;
    rho_z.apply_z(qubit);
    double keep = 1.0 - p;
    for (size_t i = 0; i < dim * dim; ++i) {
        data[i] = keep * data[i] + p * rho_z.data[i];
    }
}

void GhostDensityMatrix::amplitude_damping_channel(int qubit, double gamma) {
    if (gamma <= 0.0) return;
    if (gamma > 1.0) gamma = 1.0;
    double root = std::sqrt(1.0 - gamma);
    double decay = std::sqrt(gamma);

    std::vector<std::vector<Complex>> kraus = {
        { Complex(1.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(root, 0.0) },
        { Complex(0.0, 0.0), Complex(decay, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0) }
    };
    apply_kraus(qubit, kraus);
}

void GhostDensityMatrix::phase_damping_channel(int qubit, double lambda) {
    if (lambda <= 0.0) return;
    if (lambda > 1.0) lambda = 1.0;
    double root = std::sqrt(1.0 - lambda);
    double dephase = std::sqrt(lambda);

    std::vector<std::vector<Complex>> kraus = {
        { Complex(1.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(root, 0.0) },
        { Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(0.0, 0.0), Complex(dephase, 0.0) }
    };
    apply_kraus(qubit, kraus);
}

std::string GhostDensityMatrix::to_string(int precision) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision);
    for (size_t r = 0; r < dim; ++r) {
        for (size_t c = 0; c < dim; ++c) {
            Complex v = at(r, c);
            ss << "[" << v.real() << (v.imag() >= 0 ? "+" : "") << v.imag() << "i] ";
        }
        ss << "\n";
    }
    return ss.str();
}

// -------------------------------------------------------------
// TABELA GLOBAL DE DENSITY MATRIX
// -------------------------------------------------------------

static std::mutex g_density_mtx;
static std::map<int, DensityPtr> g_densities;
static int g_density_counter = 1;

int ghost_density_create(int qubits) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    int id = g_density_counter++;
    g_densities[id] = std::make_shared<GhostDensityMatrix>(qubits);
    return id;
}

int ghost_density_maximally_mixed(int qubits) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    int id = g_density_counter++;
    g_densities[id] = std::make_shared<GhostDensityMatrix>(GhostDensityMatrix::maximally_mixed(qubits));
    return id;
}

double ghost_density_purity(int handle) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    return (it != g_densities.end()) ? it->second->purity() : 0.0;
}

double ghost_density_trace(int handle) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    return (it != g_densities.end()) ? it->second->trace() : 0.0;
}

double ghost_density_prob(int handle, int state) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end() && state >= 0 && static_cast<size_t>(state) < it->second->dim) {
        return it->second->probabilities()[state];
    }
    return 0.0;
}

void ghost_density_h(int handle, int qubit) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->apply_h(qubit);
}

void ghost_density_x(int handle, int qubit) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->apply_x(qubit);
}

void ghost_density_cnot(int handle, int ctrl, int target) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->apply_cnot(ctrl, target);
}

void ghost_density_depolarize(int handle, int qubit, double p) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->depolarizing_channel(qubit, p);
}

void ghost_density_bit_flip(int handle, int qubit, double p) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->bit_flip_channel(qubit, p);
}

void ghost_density_phase_flip(int handle, int qubit, double p) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->phase_flip_channel(qubit, p);
}

void ghost_density_amplitude_damping(int handle, int qubit, double gamma) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->amplitude_damping_channel(qubit, gamma);
}

void ghost_density_phase_damping(int handle, int qubit, double lambda) {
    std::lock_guard<std::mutex> lock(g_density_mtx);
    auto it = g_densities.find(handle);
    if (it != g_densities.end()) it->second->phase_damping_channel(qubit, lambda);
}

} // namespace GhostQuantum
