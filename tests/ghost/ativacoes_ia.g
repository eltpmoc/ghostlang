function void test_ativacoes() {
    gate t1(2, 2);
    activate(t1, tanh);

    gate t2(2, 2);
    activate(t2, leaky_relu);

    gate t3(2, 2);
    activate(t3, gelu);

    gate t4(2, 2);
    activate(t4, softmax);

    print("test_ativacoes_ia: OK");
}

function void main() {
    test_ativacoes();
    print("todos os testes de ativacoes_ia passaram com sucesso!");
}
