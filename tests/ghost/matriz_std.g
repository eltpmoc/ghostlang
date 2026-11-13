// matriz_std.g - Teste completo de operacoes matriciais e vetoriais em lote
function void main() {
    print("=== Operacoes Matriciais GhostMatrix (F1) ===");
    gate h_gate = hadamard;
    print("Hadamard:");
    show(h_gate);

    gate py_gate = pauliy;
    print("Pauli-Y:");
    show(py_gate);

    gate m1(2, 2);
    gate m2(2, 2);
    gate m3(2, 2);
    m3 = m1 + m2;
    m3 = m1 - m2;
    m3 = m1 * m2;
    m3 = m1 * 2.0;
    m3 = m1 / 2.0;

    gate t(2, 2);
    t = m1.transpose();
    print("Transposicao:");
    show(t);

    print("=== Operacoes Vetoriais em Lote (Core) ===");
    var mA = [1, 2, 3, 4];
    var mB = [5, 6, 7, 8];
    print("mat_mul: ", mat_mul(mA, mB, 2, 2, 2));
    print("mat_transpose: ", mat_transpose(mA, 2, 2));

    var A2D = [[1, 2], [3, 4]];
    var B2D = [[2, 0], [1, 2]];
    print("mat2d_mul: ", mat2d_mul(A2D, B2D));

    print("=== Ativacoes Vetoriais em Lote ===");
    gate act(2, 2);
    activate(act, tanh);
    activate(act, leaky_relu);
    activate(act, gelu);
    activate(act, softmax);

    print("matriz_std: OK");
}
