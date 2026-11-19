// Matrizes e Álgebra Linear de Alta Performance: GhostMatrix
function void main() {
    // Matrizes de exemplo A
    matrix A(2, 2);
    A.set(0, 0, 4.0); A.set(0, 1, 7.0);
    A.set(1, 0, 2.0); A.set(1, 1, 6.0);

    matrix A_inv = A.inverse();
    matrix I = A * A_inv; // Matriz Identidade (I) = A * A_inv

    println("Matriz A original:");
    A.show();
    println("Matriz A inversa A^-1:");
    A_inv.show();
    println("Matriz Identidade Resultante (I) = A * A^-1:");
    I.show();
}
