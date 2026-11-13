function void main() {
    pragma auto;
    state entrada = [1.0, 2.0];
    gate pesos(2, 2);
    pesos(0, 0) = GhostNumberG(1.0);
    pesos(0, 1) = GhostNumberG(0.0);
    pesos(1, 0) = GhostNumberG(0.0);
    pesos(1, 1) = GhostNumberG(2.0);
    state saida = pesos * entrada;
    print(saida(0, 0).x);
    print(saida(1, 0).x);
}
