import "libs/test.g";

function void test_fs() {
    string path1 = "tmp_fs_test1.txt";
    string path2 = "tmp_fs_test2.txt";

    writefile(path1, "GhostFS Dados 123");
    assert_true(file_exists(path1), "arquivo criado existe");

    string conteudo = readfile(path1);
    assert_eq_str(conteudo, "GhostFS Dados 123", "conteudo do arquivo lido");

    copyfile(path1, path2);
    assert_true(file_exists(path2), "arquivo copiado existe");

    string conteudo2 = readfile(path2);
    assert_eq_str(conteudo2, "GhostFS Dados 123", "conteudo da copia");

    deletefile(path1);
    assert_false(file_exists(path1), "arquivo 1 removido com sucesso");

    deletefile(path2);
    assert_false(file_exists(path2), "arquivo 2 removido com sucesso");

    print("test_fs: OK");
}

function void test_os() {
    string vazio = ghost_env_get("VARIAVEL_INEXISTENTE_GHOST_XYZ");
    assert_eq_str(vazio, "", "variavel inexistente vazia");

    string cmd_out = ghost_system_exec("echo GhostOS_Exec_OK");
    bool achou = regex_search(cmd_out, "GhostOS_Exec_OK");
    assert_true(achou, "exec echo produziu GhostOS_Exec_OK");

    print("test_os: OK");
}

function void main() {
    test_fs();
    test_os();
    print("todos os testes de fs_os passaram com sucesso!");
}
