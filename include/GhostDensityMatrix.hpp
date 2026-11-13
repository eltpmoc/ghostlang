#ifndef GHOST_DENSITY_MATRIX_HPP
#define GHOST_DENSITY_MATRIX_HPP

#include <vector>
#include <complex>
#include <string>
#include <memory>
#include "GhostQuantum.hpp"

namespace GhostQuantum {

class GhostDensityMatrix {
public:
    using Complex = std::complex<double>;
    int num_qubits = 1;
    size_t dim = 2; // 2^n
    std::vector<Complex> data; // dim x dim (row-major)

    GhostDensityMatrix(int qubits = 1);
    static GhostDensityMatrix from_statevector(const GhostQReg& qreg);
    static GhostDensityMatrix maximally_mixed(int qubits);

    Complex at(size_t r, size_t c) const {
        return data[r * dim + c];
    }
    void set(size_t r, size_t c, const Complex& val) {
        data[r * dim + c] = val;
    }

    double trace() const;
    double purity() const; // Tr(rho^2)
    std::vector<double> probabilities() const;

    // Portas unitarias
    void apply_gate_1q(int qubit, const Complex U[4]);
    void apply_h(int qubit);
    void apply_x(int qubit);
    void apply_z(int qubit);
    void apply_cnot(int ctrl, int target);

    // Canais de Kraus / Ruido Quantico
    void apply_kraus(int qubit, const std::vector<std::vector<Complex>>& kraus_ops);
    void depolarizing_channel(int qubit, double p);
    void bit_flip_channel(int qubit, double p);
    void phase_flip_channel(int qubit, double p);
    void amplitude_damping_channel(int qubit, double gamma);
    void phase_damping_channel(int qubit, double lambda);

    std::string to_string(int precision = 4) const;
};

using DensityPtr = std::shared_ptr<GhostDensityMatrix>;

// Handles para chamadas do interpretador e C++
int ghost_density_create(int qubits);
int ghost_density_maximally_mixed(int qubits);
double ghost_density_purity(int handle);
double ghost_density_trace(int handle);
double ghost_density_prob(int handle, int state);
void ghost_density_h(int handle, int qubit);
void ghost_density_x(int handle, int qubit);
void ghost_density_cnot(int handle, int ctrl, int target);
void ghost_density_depolarize(int handle, int qubit, double p);
void ghost_density_bit_flip(int handle, int qubit, double p);
void ghost_density_phase_flip(int handle, int qubit, double p);
void ghost_density_amplitude_damping(int handle, int qubit, double gamma);
void ghost_density_phase_damping(int handle, int qubit, double lambda);

} // namespace GhostQuantum

#endif // GHOST_DENSITY_MATRIX_HPP
