// Rede neural clássica (gate >> gate) treinada de verdade com cada otimizador (16/11/2026 em diante):
// adam, sgd (uma atualização por amostra, ordem embaralhada com semente fixa), momentum, rmsprop e gd.
// Tarefa XOR (não linear). Pesos iniciais fixos para o compilador e o REPL partirem do mesmo ponto;
// mesma saída nos dois caminhos (tests/repl/redes_otimizadores.ghost).
function main() {
    gate X(2, 4);
    gate Y(1, 4);
    X(0, 0) = 0.0; X(1, 0) = 0.0; Y(0, 0) = 0.0;
    X(0, 1) = 0.0; X(1, 1) = 1.0; Y(0, 1) = 1.0;
    X(0, 2) = 1.0; X(1, 2) = 0.0; Y(0, 2) = 1.0;
    X(0, 3) = 1.0; X(1, 3) = 1.0; Y(0, 3) = 0.0;

    // adam
    gate A1(6, 2);
    gate A2(1, 6);
    for (int r = 0; r < 6; r++) { for (int c = 0; c < 2; c++) { A1(r, c) = ((r * 7 + c * 3) % 11) / 10.0 - 0.5; } }
    for (int c = 0; c < 6; c++) { A2(0, c) = ((c * 5) % 7) / 7.0 - 0.4; }
    network ra = A1 >> A2;
    print("adam     antes: ", ra.loss(X, Y));
    ra => optimize(X, Y, epochs=800, lr=0.02, opt="adam");
    print("adam     depois: ", ra.loss(X, Y) < 0.01, " acuracia ", ra.accuracy(X, Y));

    // sgd
    gate S1(6, 2);
    gate S2(1, 6);
    for (int r = 0; r < 6; r++) { for (int c = 0; c < 2; c++) { S1(r, c) = ((r * 7 + c * 3) % 11) / 10.0 - 0.5; } }
    for (int c = 0; c < 6; c++) { S2(0, c) = ((c * 5) % 7) / 7.0 - 0.4; }
    network rs = S1 >> S2;
    rs => optimize(X, Y, epochs=1500, lr=0.05, opt="sgd");
    print("sgd      depois: ", rs.loss(X, Y) < 0.01, " acuracia ", rs.accuracy(X, Y));

    // momentum
    gate M1(6, 2);
    gate M2(1, 6);
    for (int r = 0; r < 6; r++) { for (int c = 0; c < 2; c++) { M1(r, c) = ((r * 7 + c * 3) % 11) / 10.0 - 0.5; } }
    for (int c = 0; c < 6; c++) { M2(0, c) = ((c * 5) % 7) / 7.0 - 0.4; }
    network rm = M1 >> M2;
    rm => optimize(X, Y, epochs=1500, lr=0.02, opt="momentum");
    print("momentum depois: ", rm.loss(X, Y) < 0.01, " acuracia ", rm.accuracy(X, Y));

    // rmsprop
    gate P1(6, 2);
    gate P2(1, 6);
    for (int r = 0; r < 6; r++) { for (int c = 0; c < 2; c++) { P1(r, c) = ((r * 7 + c * 3) % 11) / 10.0 - 0.5; } }
    for (int c = 0; c < 6; c++) { P2(0, c) = ((c * 5) % 7) / 7.0 - 0.4; }
    network rp = P1 >> P2;
    rp => optimize(X, Y, epochs=800, lr=0.01, opt="rmsprop");
    print("rmsprop  depois: ", rp.loss(X, Y) < 0.01, " acuracia ", rp.accuracy(X, Y));

    // gd (lote inteiro)
    gate G1(6, 2);
    gate G2(1, 6);
    for (int r = 0; r < 6; r++) { for (int c = 0; c < 2; c++) { G1(r, c) = ((r * 7 + c * 3) % 11) / 10.0 - 0.5; } }
    for (int c = 0; c < 6; c++) { G2(0, c) = ((c * 5) % 7) / 7.0 - 0.4; }
    network rg = G1 >> G2;
    rg => optimize(X, Y, epochs=4000, lr=0.05, opt="gd");
    print("gd       depois: ", rg.loss(X, Y) < 0.05, " acuracia ", rg.accuracy(X, Y));

    // previsões da rede treinada com adam (classify: limiar 0,5)
    print("xor(0,0)=", ra.classify(0.0, 0.0), " xor(0,1)=", ra.classify(0.0, 1.0), " xor(1,0)=", ra.classify(1.0, 0.0), " xor(1,1)=", ra.classify(1.0, 1.0));
    show(ra);

    // otimizador inexistente: erro claro (antes caia em silencio no adam)
    try {
        ra => optimize(X, Y, epochs=1, lr=0.1, opt="newton");
    } catch (e) {
        print("erro: ", e);
    }
}
