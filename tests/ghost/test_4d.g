function main() {
    print("=== Iniciando Teste de Tensores Hiper-Complexos 4D ===\n");

    gate tensor_a(3, 1);

    print("Tensor A original:\n");
    show(tensor_a);

    print("\nAplicando ReLU no Tensor A:\n");
    activate(tensor_a, relu);
    show(tensor_a);

    gate tensor_b(3, 1);

    print("\nAplicando Sigmoid no Tensor B:\n");
    activate(tensor_b, sigmoid);
    show(tensor_b);

    gate tensor_c(3, 1);

    print("\nAplicando Seno (sin) no Tensor C:\n");
    activate(tensor_c, sin);
    show(tensor_c);

    print("\n=== Teste Finalizado ===\n");
}
