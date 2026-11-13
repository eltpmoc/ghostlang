// qml_rede_hibrida.g - Rede neural hibrida classico-quantica com network >> (F4)
// densa (2->2) >> camada quantica (2 qubits) >> densa (2->1), treinada por backpropagation:
// as camadas densas pela regra da cadeia e a qlayer pelo parameter-shift.
// Tarefa: separar os pontos dentro do circulo x^2 + y^2 < 0.45 (nao linearmente separavel).

function main() {
    gate X(2, 64);   // entradas: uma amostra por coluna
    gate Y(1, 64);   // alvo: 1 dentro do circulo, 0 fora
    for (var i = 0; i < 8; i = i + 1) {
        for (var j = 0; j < 8; j = j + 1) {
            int k = i * 8 + j;
            double a = -1.0 + 2.0 * i / 7.0;
            double b = -1.0 + 2.0 * j / 7.0;
            X(0, k) = a;
            X(1, k) = b;
            Y(0, k) = 0.0;
            if (a * a + b * b < 0.45) {
                Y(0, k) = 1.0;
            }
        }
    }

    gate W1(2, 2);
    qlayer Q(2, 2);
    gate W2(1, 2);
    Q.seed(3);
    network rede = W1 >> Q >> W2;
    print(Q.draw());

    print(f"Antes:  perda ${rede.loss(X, Y)}  acuracia ${rede.accuracy(X, Y) * 100.0}%");
    rede.set_verbose(50);
    rede => optimize(X, Y, epochs=150, lr=0.05, opt="adam");
    print(f"Depois: perda ${rede.loss(X, Y)}  acuracia ${rede.accuracy(X, Y) * 100.0}%");

    print(f"Centro (0, 0)        -> classe ${rede.classify(0.0, 0.0)}  saida ${rede.predict(0.0, 0.0)}");
    print(f"Canto (0.95, 0.95)   -> classe ${rede.classify(0.95, 0.95)}  saida ${rede.predict(0.95, 0.95)}");
}
