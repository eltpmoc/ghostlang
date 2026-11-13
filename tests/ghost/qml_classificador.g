// qml_classificador.g - Classificador quantico-hibrido treinado com parameter-shift (F4)
// Modelo: saida = w * <Z0>(x) + b, com o circuito RY(x) -> 2 camadas RY/RZ treinaveis + CNOT.

function main() {
    qcircuit modelo(2);
    modelo.seed(5);
    modelo.encode_all();
    modelo.ansatz(2);

    // Dados: pontos (a, b) em [0, pi]^2; classe 1 quando a + b > pi
    for (var i = 0; i < 8; i = i + 1) {
        for (var j = 0; j < 8; j = j + 1) {
            double a = i / 7.0 * PI;
            double b = j / 7.0 * PI;
            int classe = 0;
            if (a + b > PI) {
                classe = 1;
            }
            modelo.add_sample(classe, a, b);
        }
    }

    print(modelo);
    print(modelo.draw());
    print(f"Antes do treino: acuracia ${modelo.accuracy() * 100.0}%  perda ${modelo.loss()}");

    modelo.set_verbose(20);
    double perda = modelo.fit(60, 0.1);
    print(f"Depois do treino: acuracia ${modelo.accuracy() * 100.0}%  perda ${perda}");

    print(f"Ponto (0.3, 0.5) -> classe ${modelo.predict(0.3, 0.5)}");
    print(f"Ponto (2.8, 2.6) -> classe ${modelo.predict(2.8, 2.6)}");
    print(f"Parametros: ${modelo.params()}");
}
