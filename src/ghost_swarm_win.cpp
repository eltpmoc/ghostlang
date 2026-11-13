#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

string sendToNode(const string& ip, int port, const string& payload) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return "Erro de socket";
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return "Erro de conexao";
    }
    
    send(sock, payload.c_str(), payload.length(), 0);
    
    char buffer[4096] = {0};
    recv(sock, buffer, 4096, 0);
    closesocket(sock);
    
    return string(buffer);
}

double extractFeature(const string& raw) {
    string numStr = "";
    bool hasDot = false;
    for (char c : raw) {
        if (isdigit(c) || c == '-') numStr += c;
        else if ((c == '.' || c == ',') && !hasDot) { numStr += '.'; hasDot = true; }
    }
    if (numStr.empty() || numStr == "." || numStr == "-") return 0.0;
    try {
        double val = stod(numStr);
        while (val > 1.0 || val < -1.0) val /= 10.0; 
        return val;
    } catch (...) { return 0.0; }
}

vector<double> extractGhosts(const string& response) {
    vector<double> ghosts;
    size_t pos = 0;
    while ((pos = response.find("+", pos)) != string::npos) {
        size_t endPos = response.find("g", pos);
        if (endPos != string::npos && (endPos - pos) < 15) { 
            try { ghosts.push_back(stod(response.substr(pos + 1, endPos - pos - 1))); } catch (...) {}
            pos = endPos;
        } else pos++;
    }
    return ghosts;
}

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    cout << "=== Ghost Swarm: Cerebro Global (Native Windows) ===" << endl;
    
    string filename = (argc > 1) ? argv[1] : "planilha_chatbot.csv";
    ifstream file(filename);
    if (!file.is_open()) { cerr << "Erro ao abrir: " << filename << endl; WSACleanup(); return 1; }

    string line;
    getline(file, line);
    char delimiter = (line.find(';') != string::npos) ? ';' : ',';

    string ghostScript = "state entrada = [0.0, 0.0]\nstate alvo = [1.0, 0.0]\ngate cerebro(2, 2)\nload(entrada, \"dados_node.csv\")\ncerebro => optimize(entrada, alvo, epochs=3, lr=0.05)\nshow(cerebro)\n===GHOST_DATA===\n";
    
    vector<double> globalBrain = {0.0, 0.0, 0.0, 0.0}; 
    ifstream brainIn("cerebro.weights");
    if (brainIn.is_open()) {
        brainIn >> globalBrain[0] >> globalBrain[1] >> globalBrain[2] >> globalBrain[3];
        brainIn.close();
        cout << "[Swarm] Memoria carregada." << endl;
    }

    string currentBatch = "";
    int count = 0, loteId = 1;
    double learningRate = 0.01;

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
            string resposta = sendToNode("127.0.0.1", 8080, payload);

            if (resposta.find("Erro de conexao") != string::npos || resposta.empty()) {
                cerr << "\n[FALHA] No Operario caiu ou falhou!" << endl;
                WSACleanup();
                return 1; 
            }

            vector<double> ghosts = extractGhosts(resposta);
            if (!ghosts.empty()) {
                for (size_t i = 0; i < ghosts.size() && i < globalBrain.size(); ++i) {
                    globalBrain[i] += learningRate * ghosts[i];
                }
                cout << "Lote " << loteId << " | Pesos: [" << globalBrain[0] << ", " << globalBrain[1] << ", " << globalBrain[2] << ", " << globalBrain[3] << "]" << endl;
            }
            currentBatch = ""; count = 0; loteId++;
        }
    }
    file.close();

    ofstream brainOut("cerebro.weights");
    if(brainOut.is_open()){
        brainOut << globalBrain[0] << " " << globalBrain[1] << " " << globalBrain[2] << " " << globalBrain[3];
        brainOut.close();
    }

    WSACleanup();
    return 0;
}