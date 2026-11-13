#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    // Cabeçalho HTTP para caso seja executado via CGI (opcional, mas boa prática)
    cout << "Content-Type: application/json\n\n";

    if (argc < 3) {
        cout << "{\"error\": \"Parâmetros insuficientes. Uso: ghost_api <x0> <x1>\"}" << endl;
        return 1;
    }

    double x0 = 0.0, x1 = 0.0;
    try {
        x0 = stod(argv[1]);
        x1 = stod(argv[2]);
    } catch (...) {
        cout << "{\"error\": \"Parâmetros inválidos. Devem ser números.\"}" << endl;
        return 1;
    }

    vector<double> brain(4, 0.0);
    ifstream file("cerebro.weights");
    if (file.is_open()) {
        file >> brain[0] >> brain[1] >> brain[2] >> brain[3];
        file.close();
    } else {
        cout << "{\"error\": \"Memória (cerebro.weights) não encontrada.\"}" << endl;
        return 1;
    }

    // Inferência
    double out0 = (brain[0] * x0) + (brain[1] * x1);
    double out1 = (brain[2] * x0) + (brain[3] * x1);

    // Lógica de Negócio (Ex: Se a tendência de risco for fortemente negativa, dispara alerta)
    bool isAlert = (out0 < -0.1 || out1 < -0.1); 
    string status = isAlert ? "CRITICAL_GROWTH" : "STABLE_OR_IMPROVING";

    // Formatação JSON de saída
    cout << "{" << endl;
    cout << "  \"status\": \"success\"," << endl;
    cout << "  \"analysis\": {" << endl;
    cout << "    \"input\": [" << x0 << ", " << x1 << "]," << endl;
    cout << "    \"prediction_vector\": [" << out0 << ", " << out1 << "]," << endl;
    cout << "    \"condition\": \"" << status << "\"," << endl;
    cout << "    \"requires_attention\": " << (isAlert ? "true" : "false") << endl;
    cout << "  }" << endl;
    cout << "}" << endl;

    return 0;
}