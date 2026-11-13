// ============================================================
// Biblioteca Padrao: GhostDB (SQLite) v1.0
// Banco de dados relacional embutido de alta performance
// ============================================================

function int db_open(string path) {
    return ghost_db_open(path);
}

function int db_exec(int handle, string sql) {
    return ghost_db_exec(handle, sql);
}

function string db_query(int handle, string sql) {
    return ghost_db_query(handle, sql);
}

function int db_close(int handle) {
    return ghost_db_close(handle);
}

function int db_last_id(int handle) {
    return ghost_db_last_id(handle);
}

function int db_changes(int handle) {
    return ghost_db_changes(handle);
}
