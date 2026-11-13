// GhostLang Quantum Density Matrix & Noise Library (libs/quantum_density.g)

function int qdensity_create(int qubits) {
    return ghost_density_create(qubits);
}

function int qdensity_maximally_mixed(int qubits) {
    return ghost_density_maximally_mixed(qubits);
}

function double qdensity_purity(int handle) {
    return ghost_density_purity(handle);
}

function double qdensity_trace(int handle) {
    return ghost_density_trace(handle);
}

function double qdensity_prob(int handle, int estado) {
    return ghost_density_prob(handle, estado);
}

function void qdensity_h(int handle, int qubit) {
    ghost_density_h(handle, qubit);
}

function void qdensity_x(int handle, int qubit) {
    ghost_density_x(handle, qubit);
}

function void qdensity_cnot(int handle, int ctrl, int target) {
    ghost_density_cnot(handle, ctrl, target);
}

function void qdensity_depolarize(int handle, int qubit, double p) {
    ghost_density_depolarize(handle, qubit, p);
}

function void qdensity_bit_flip(int handle, int qubit, double p) {
    ghost_density_bit_flip(handle, qubit, p);
}

function void qdensity_phase_flip(int handle, int qubit, double p) {
    ghost_density_phase_flip(handle, qubit, p);
}

function void qdensity_amplitude_damping(int handle, int qubit, double gamma) {
    ghost_density_amplitude_damping(handle, qubit, gamma);
}

function void qdensity_phase_damping(int handle, int qubit, double taxa_lambda) {
    ghost_density_phase_damping(handle, qubit, taxa_lambda);
}

