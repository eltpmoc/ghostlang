// qml_qaoa.g - QAOA para MaxCut (F4)
// Grafo: anel de 6 vertices + 2 cordas. Cada qubit e um vertice; o bit diz o lado do corte.
// Circuito: H em todos; por camada, RZZ(-w*gamma) em cada aresta e RX(2*beta) em cada qubit.

function main() {
    qcircuit grafo(6);
    grafo.seed(4);
    grafo.edge(0, 1); grafo.edge(1, 2); grafo.edge(2, 3);
    grafo.edge(3, 4); grafo.edge(4, 5); grafo.edge(5, 0);
    grafo.edge(0, 3); grafo.edge(1, 4);

    grafo.qaoa(3);
    print(grafo);
    print(f"Otimo exato (forca bruta): ${grafo.maxcut_exact()}");
    print(f"Corte esperado antes:      ${grafo.expected_cut()}");

    grafo.set_verbose(50);   // energia = -corte esperado
    grafo.minimize(150, 0.05);

    print(f"Corte esperado depois:     ${grafo.expected_cut()}");
    print(f"Razao de aproximacao:      ${grafo.expected_cut() / grafo.maxcut_exact()}");
    string solucao = grafo.maxcut_solution(2000);
    print(f"Melhor solucao amostrada:  ${solucao} -> corte ${grafo.cut_of(solucao)}");
}
