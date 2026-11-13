function void main() {
    ghost a = GhostNumberG(1.0, 2.0, 3.0);
    ghost b = GhostNumberG(4.0, 5.0, 6.0);
    ghost soma = a + b;
    ghost produto = a * b;
    print("Soma: " + soma.x + ", " + soma.y + ", " + soma.p);
    print("Produto: " + produto.x + ", " + produto.y + ", " + produto.p);
}
