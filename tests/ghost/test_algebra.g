function main() {
    print("=== Algebra Hiper-Complexa GhostNumberG (4D) ===\n");

    print("1. Criacao de Porta Quantica Customizada (com incerteza Ghost / Nilpotente)\n");
    gate q1(2, 2);
    show(q1);

    print("2. Adicao (q1 + q1)\n");
    gate res(2, 2);
    res = q1 + q1;
    show(res);

    print("3. Multiplicacao Hiper-Complexa (q1 * q1) - Verificando o componente p\n");
    res = q1 * q1;
    show(res);

    print("4. Portas Nativas com fases complexas e reais\n");
    gate h_gate = hadamard;
    print("Gate Hadamard:\n");
    show(h_gate);

    gate py_gate = pauliy;
    print("Gate Pauli-Y (Fase Imaginaria -i e i):\n");
    show(py_gate);

    print("=== Fim da Demonstracao ===\n");
}
