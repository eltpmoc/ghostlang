// ============================================================
// Biblioteca Padrao: GhostOS v1.0
// Interacao com o Sistema Operacional e Variaveis de Ambiente
// ============================================================

function string os_getenv(string key) {
    return ghost_env_get(key);
}

function string os_exec(string cmd) {
    return ghost_system_exec(cmd);
}
