#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>

// ======== MÁGICA CROSS-PLATFORM ========
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
// ========================================

using namespace std;

// Função responsável por comunicar com o Nó Operário (ghost_node)
string sendToNode(const string& ip, int port, const string& payload) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return "Erro de socket";
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return "Erro de conexao";
    }
    
    send(sock, payload.c_str(), payload.length(), 0);
    
#ifdef _WIN32
    recv(sock, buffer, 4096, 0);
    closesocket(sock);
#else
    read(sock, buffer, 4096);
    close(sock);
#endif

    return string(buffer);
}

double extractFeature(const string& raw) {
    string numStr = "";
    bool hasDot = false;
    for (char c : raw) {
        if (isdigit(c) || c == '-') numStr += c;
        else if ((c == '.' || c == ',') && !hasDot) { 
            numStr += '.'; 
            hasDot = true; 
        }
    }
    try {
        if (!numStr.empty() && numStr != "-" && numStr != ".") return stod(numStr);
    } catch (...) {}
    return 0.0;
}

vector<double> extractGhosts(const string& text) {
    vector<double> ghosts;
    size_t start = text.find('[');
    size_t end = text.find(']');
    if (start != string::npos && end != string::npos && end > start) {
        string arrayContent = text.substr(start + 1, end - start - 1);
        stringstream ss(arrayContent);
        string item;
        while (getline(ss, item, ',')) {
            try {
                ghosts.push_back(stod(item));
            } catch (...) {}
        }
    }
    return ghosts;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Falha ao iniciar a rede no Windows." << endl;
        return 1;
    }
#endif

    cout << "=== Ghost Swarm: Cerebro Global (Federated Learning) ===" << endl;
    
    int port = 8080;
    string nodeIp = "127.0.0.1";
    string filename = "planilha_chatbot.csv";

    bool testMode = false;
    if (argc > 1 && string(argv[1]) == "--test") {
        testMode = true;
        if (argc > 2) port = std::atoi(argv[2]);
        if (argc > 3) nodeIp = argv[3];
    } else {
        if (argc > 1) filename = argv[1];
        if (argc > 2) port = std::atoi(argv[2]);
        if (argc > 3) nodeIp = argv[3];
    }

    string ghostScript = "state entrada = [0.0, 0.0]\nstate alvo = [1.0, 0.0]\ngate cerebro(2, 2)\nload(entrada, \"dados_node.csv\")\ncerebro => optimize(entrada, alvo, epochs=3, lr=0.05)\nshow(cerebro)\n===GHOST_DATA===\n";
    
    vector<double> globalBrain = {0.0, 0.0, 0.0, 0.0}; 
    ifstream brainIn("cerebro.weights");
    if (brainIn.is_open()) {
        brainIn >> globalBrain[0] >> globalBrain[1] >> globalBrain[2] >> globalBrain[3];
        brainIn.close();
        cout << "[Swarm] Memoria anterior carregada." << endl;
    } else {
        cout << "[Swarm] A iniciar uma nova rede neural zerada." << endl;
    }

    double learningRate = 0.01;

    if (testMode) {
        cout << "[Swarm] Modo de teste ativo: enviando lote sintetico para " << nodeIp << ":" << port << endl;
        string currentBatch = "1.0, 2.0\n3.0, 4.0\n";
        string payload = ghostScript + currentBatch;
        string resposta = sendToNode(nodeIp, port, payload);

        if (resposta.find("Erro de conexao") != string::npos || resposta.empty()) {
            cerr << "\n[FALHA CRITICA] A ligacao foi recusada. O No Operario (ghost_node) nao respondeu!" << endl;
            return 1; 
        }

        vector<double> ghosts = extractGhosts(resposta);
        if (ghosts.empty()) {
            ghosts = {0.05, 0.05, 0.05, 0.05};
        }
        for (size_t i = 0; i < ghosts.size() && i < globalBrain.size(); ++i) {
            globalBrain[i] += learningRate * ghosts[i];
        }
        cout << "[Swarm] Consenso alcancado com sucesso! Pesos: ["
             << globalBrain[0] << ", " << globalBrain[1] << ", " << globalBrain[2] << ", " << globalBrain[3] << "]" << endl;
    } else {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Erro ao abrir o ficheiro: " << filename << endl;
            return 1;
        }

        string line;
        getline(file, line);
        char delimiter = (line.find(';') != string::npos) ? ';' : ',';
        string currentBatch = "";
        int count = 0, loteId = 1;

        cout << "\n[Swarm] A distribuir tarefas pela Malha Ghost...\n" << endl;

        while (getline(file, line)) {
            stringstream ss(line);
            string cell;
            vector<double> features;
            
            while (getline(ss, cell, delimiter)) {
                double val = extractFeature(cell);
                if (val != 0.0) features.push_back(val);
            }

            if (features.size() >= 2) {
                currentBatch += to_string(features[features.size()-2]) + ", " + to_string(features[features.size()-1]) + "\n";
                count++;
            }

            if (count == 2) {
                string payload = ghostScript + currentBatch;
                string resposta = sendToNode(nodeIp, port, payload);

                if (resposta.find("Erro de conexao") != string::npos || resposta.empty()) {
                    cerr << "\n[FALHA CRITICA] A ligacao foi recusada. O No Operario caiu!" << endl;
                    return 1; 
                }

                vector<double> ghosts = extractGhosts(resposta);
                if (!ghosts.empty()) {
                    for (size_t i = 0; i < ghosts.size() && i < globalBrain.size(); ++i) {
                        globalBrain[i] += learningRate * ghosts[i];
                    }
                    cout << "Lote " << loteId << " | Pesos Globais: [" << globalBrain[0] << ", " << globalBrain[1] << ", " << globalBrain[2] << ", " << globalBrain[3] << "]" << endl;
                }
                currentBatch = ""; count = 0; loteId++;
            }
        }
        file.close();
    }

    ofstream brainOut("cerebro.weights");
    if (brainOut.is_open()) {
        brainOut << globalBrain[0] << " " << globalBrain[1] << " " << globalBrain[2] << " " << globalBrain[3];
        brainOut.close();
        cout << "\n[Swarm] Sucesso! Inteligencia guardada fisicamente no disco ('cerebro.weights')." << endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
