#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Uso: ./ghost_ask <valor_ano_anterior> <valor_ano_atual>" << endl;
        cout << "Exemplo: ./ghost_ask 0.29 0.29" << endl;
        return 1;
    }

    double x0 = stod(argv[1]);
    double x1 = stod(argv[2]);

    // 1. Carregar a Memória Global
    vector<double> brain(4, 0.0);
    ifstream file("cerebro.weights");
    if (file.is_open()) {
        file >> brain[0] >> brain[1] >> brain[2] >> brain[3];
        file.close();
    } else {
        cerr << "[Erro] Ficheiro cerebro.weights nao encontrado. O Cérebro está vazio." << endl;
        return 1;
    }

    // 2. Inferência Matemática (Forward Pass Clássico baseado na nossa Matriz Fantasma)
    // Multiplicamos o vetor de entrada pela matriz de pesos treinada
    double out0 = (brain[0] * x0) + (brain[1] * x1);
    double out1 = (brain[2] * x0) + (brain[3] * x1);

    cout << "=== Ghost OS: Módulo de Inferência ===" << endl;
    cout << "Memória Carregada: [" << brain[0] << ", " << brain[1] << ", " << brain[2] << ", " << brain[3] << "]" << endl;
    cout << "Cenário Injetado : [" << x0 << " -> " << x1 << "]" << endl;
    cout << "--------------------------------------" << endl;
    cout << "Resposta do Modelo (Tendência Preditiva):" << endl;
    cout << "[ " << out0 << " ]" << endl;
    cout << "[ " << out1 << " ]" << endl;
    cout << "======================================" << endl;

    return 0;
}