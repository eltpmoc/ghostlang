// GhostLang Quantum Hardware Integration Library (libs/hardware.g)

function void qhw_configure(string provider, string api_token, string default_backend) {
    ghost_qhw_configure(provider, api_token, default_backend);
}

function string qhw_submit(string qasm_code, string backend_name, int shots) {
    return ghost_qhw_submit(qasm_code, backend_name, shots);
}

function string qhw_status(string job_id) {
    return ghost_qhw_status(job_id);
}

function int qhw_get_count(string job_id, string bitstring) {
    return ghost_qhw_get_count(job_id, bitstring);
}

function string qhw_summary(string job_id) {
    return ghost_qhw_summary(job_id);
}
