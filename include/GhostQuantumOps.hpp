// GhostQuantumOps.hpp - nomes das operações de qreg/qcircuit que podem ser chamadas sem
// objeto dentro de um bloco circuit { } (compilador e REPL usam a mesma lista).
#ifndef GHOST_QUANTUM_OPS_HPP
#define GHOST_QUANTUM_OPS_HPP

#include <set>
#include <string>

inline bool ghost_is_quantum_op(const std::string& n) {
    static const std::set<std::string> ops = {
        // portas (qreg e qcircuit)
        "h", "x", "y", "z", "s", "sdg", "t", "tdg", "sx", "rx", "ry", "rz", "p", "u", "h_all", "x_all",
        "cx", "cnot", "cy", "cz", "ch", "swap", "cp", "crx", "cry", "crz", "rzz", "ccx", "toffoli", "cswap",
        "qft", "iqft", "qft_range", "iqft_range", "phase_flip", "diffuser", "reset", "measure", "measure_all",
        // qcircuit: treináveis, parâmetros compartilhados e codificação de dados
        "rx_train", "ry_train", "rz_train", "rx_param", "ry_param", "rz_param", "rx_param_scaled",
        "rzz_train", "rzz_param", "add_param", "ansatz", "encode_rx", "encode_ry", "encode_rz",
        "encode_all", "encode_rzz", "zz_feature_map"};
    return ops.count(n) > 0;
}

#endif
