// libs/test.g - framework de testes unitarios da GhostLang (asserts e verificacoes)

function void assert_true(bool cond, string msg = "Esperado true, obtido false") {
    if (!cond) {
        print("[FALHA DE TESTE] " + msg);
        throw msg;
    }
}

function void assert_false(bool cond, string msg = "Esperado false, obtido true") {
    if (cond) {
        print("[FALHA DE TESTE] " + msg);
        throw msg;
    }
}

function void assert_eq_int(int atual, int esperado, string msg = "") {
    if (atual != esperado) {
        string erro = "[FALHA DE TESTE] " + msg + " (esperado: " + esperado + ", obtido: " + atual + ")";
        print(erro);
        throw erro;
    }
}

function void assert_eq_str(string atual, string esperado, string msg = "") {
    if (atual != esperado) {
        string erro = "[FALHA DE TESTE] " + msg + " (esperado: \"" + esperado + "\", obtido: \"" + atual + "\")";
        print(erro);
        throw erro;
    }
}

function void assert_eq_double(double atual, double esperado, double tol = 0.0001, string msg = "") {
    double diff = atual - esperado;
    if (diff < 0.0) {
        diff = -diff;
    }
    if (diff > tol) {
        string erro = "[FALHA DE TESTE] " + msg + " (esperado: " + esperado + ", obtido: " + atual + ")";
        print(erro);
        throw erro;
    }
}
