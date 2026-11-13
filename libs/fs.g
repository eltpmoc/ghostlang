// ============================================================
// Biblioteca Padrao: GhostFS v1.0
// Modulo de Sistema de Arquivos (File System)
// ============================================================

function string fs_read(string path) {
    return readfile(path);
}

function void fs_write(string path, string content) {
    writefile(path, content);
}

function void fs_delete(string path) {
    deletefile(path);
}

function void fs_copy(string src, string dest) {
    copyfile(src, dest);
}

function void fs_mkdir(string path) {
    dir_create(path);
}
