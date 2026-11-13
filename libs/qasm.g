// libs/qasm.g - interoperabilidade OpenQASM 3.0 para GhostLang (exportacao e leitura)
import "libs/quantum.g";

function string to_qasm(qcircuit c) {
    return c.to_qasm();
}

function string to_qasm(qreg q) {
    return q.to_qasm();
}
