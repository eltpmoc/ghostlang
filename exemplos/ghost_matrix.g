// Exemplos com GhostMatrix (Álgebra Hipercomplexa G e IA)
function void main() {
    // 1. Instanciação com GhostMatrix
    GhostMatrix G(2, 2);

    // 2. Definindo elementos hipercomplexos 4D
    ghost g1 = ghost(2.0, 1.0, 0.5, 0.0);
    ghost g2 = ghost(-3.0, 0.0, 1.2, 0.0);
    ghost g3 = ghost(1.5, -0.5, 0.0, 0.0);
    ghost g4 = ghost(4.0, 2.0, -1.0, 0.0);

    G.set(0, 0, g1);
    G.set(0, 1, g2);
    G.set(1, 0, g3);
    G.set(1, 1, g4);

    println("Matriz Hipercomplexa GhostMatrix G:");
    G.show();

    // 3. Transposição de GhostMatrix
    GhostMatrix G_t = G.transpose();
    println("Matriz Transposta G^T:");
    G_t.show();

    // 4. Operações aritméticas escalares e matriciais
    GhostMatrix G_dobro = G * 2.0;
    println("Matriz G multiplicada pelo escalar 2.0:");
    G_dobro.show();

    // 5. Ativação não-linear ReLU sobre o componente real
    GhostMatrix G_relu = G.relu();
    println("Matriz apos ativacao ReLU:");
    G_relu.show();
}
