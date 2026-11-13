#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <exception>

// Liga a biblioteca de rede do Windows
#pragma comment(lib, "ws2_32.lib")

// Adaptação dos pipes do Windows
#define popen _popen
#define pclose _pclose

using namespace std;

void printHelp() {
    cout << "========================================================\n";
    cout << "  GHOST NODE WIN - AJUDA (HELP) \n";
    cout << "========================================================\n";
    cout << "Uso: ghost_node_win.exe [OPCOES]\n\n";
    cout << "Descricao:\n";
    cout << "  Inicia um No Operario (Edge Computing) escutando na\n";
    cout << "  porta 8080. Ele recebe tarefas do ghost_swarm, executa\n";
    cout << "  os codigos Ghost usando o compilador local (ghost.exe)\n";
    cout << "  e retorna o resultado para o orquestrador.\n\n";
    cout << "Opcoes:\n";
    cout << "  -h, --help    Exibe esta tela de ajuda e sai.\n";
    cout << "========================================================\n";
}

string execCmd(const char* cmd) {
    char buffer[256];
    string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "Erro ao abrir o processo (popen)";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);
    return result;
}

int main(int argc, char* argv[]) {
    try {
        if (argc > 1) {
            string arg = argv[1];
            if (arg == "-h" || arg == "--help" || arg == "/?") {
                printHelp();
                return 0;
            }
        }

        cout << "[DEBUG] Iniciando o Ghost Node Win..." << endl;
        
        // 1. Ligar a Placa de Rede do Windows
        cout << "[DEBUG] Passo 1: Inicializando WSAStartup..." << endl;
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << "[Erro FATAL] Falha no WSAStartup. Erro: " << WSAGetLastError() << endl;
            system("pause");
            return 1;
        }
        cout << "[DEBUG] WSAStartup concluido com sucesso." << endl;

        // 2. Criar o Socket Nativo
        cout << "[DEBUG] Passo 2: Criando Socket TCP..." << endl;
        SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == INVALID_SOCKET) {
            cout << "[Erro] Falha ao criar o Socket. Erro: " << WSAGetLastError() << endl;
            WSACleanup();
            system("pause");
            return 1;
        }
        cout << "[DEBUG] Socket criado." << endl;

        // 3. Configurar a Porta (Evitar bloqueio)
        cout << "[DEBUG] Passo 3: Configurando SO_REUSEADDR..." << endl;
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
            cout << "[DEBUG] Aviso: falha ao setar SO_REUSEADDR. Erro: " << WSAGetLastError() << endl;
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(8080);

        // 4. Acoplar à Porta 8080
        cout << "[DEBUG] Passo 4: Fazendo Bind na porta 8080..." << endl;
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
            cout << "[Erro FATAL] Porta 8080 em uso ou bloqueada pelo Firewall. Erro: " << WSAGetLastError() << endl;
            closesocket(server_fd);
            WSACleanup();
            system("pause");
            return 1;
        }
        cout << "[DEBUG] Bind efetuado na porta 8080." << endl;
        
        cout << "[DEBUG] Passo 5: Iniciando Listen..." << endl;
        if (listen(server_fd, 3) == SOCKET_ERROR) {
            cout << "[Erro] Falha no Listen. Erro: " << WSAGetLastError() << endl;
            closesocket(server_fd);
            WSACleanup();
            system("pause");
            return 1;
        }
        cout << "[DEBUG] Listen ativado." << endl;

        cout << "\n=== Ghost OS (Windows Native): Operario Ativo na Porta 8080 ===" << endl;

        while (true) {
            int addrlen = sizeof(address);
            cout << "[DEBUG] Aguardando nova conexao (accept)..." << endl;
            SOCKET new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_socket == INVALID_SOCKET) {
                cout << "[DEBUG] Conexao invalida recebida. Erro: " << WSAGetLastError() << endl;
                continue;
            }
            cout << "[DEBUG] Cliente conectado! Recebendo dados..." << endl;

            char buffer[8192] = {0};
            int bytes_received = recv(new_socket, buffer, 8192, 0);
            
            if (bytes_received > 0) {
                string payload = string(buffer, bytes_received);
                cout << "\n[Node] Carga recebida (" << bytes_received << " bytes)." << endl;

                size_t pos = payload.find("===GHOST_DATA===\n");
                if (pos != string::npos) {
                    string code = payload.substr(0, pos);
                    string data = payload.substr(pos + 17); // 17 é o tamanho do delimitador

                    cout << "[DEBUG] Salvando payload.g e dados_node.csv..." << endl;
                    ofstream outCode("payload.g"); outCode << code; outCode.close();
                    ofstream outData("dados_node.csv"); outData << data; outData.close();

                    cout << "[Node] Processando matrizes e compilando..." << endl;
                    
                    cout << "[DEBUG] Executando compilador local..." << endl;
                    string output = execCmd(".\\ghost.exe payload.g -o payload_bin.exe && .\\payload_bin.exe");
                    cout << "[DEBUG] Resultado obtido. Enviando de volta ao orquestrador..." << endl;

                    send(new_socket, output.c_str(), output.length(), 0);
                    
                    cout << "[DEBUG] Limpando disco..." << endl;
                    remove("payload.g"); remove("dados_node.csv"); 
                    remove("payload_bin.exe");
                } else {
                    cout << "[DEBUG] Carga invalida (delimitador ===GHOST_DATA=== nao encontrado)." << endl;
                }
            } else {
                cout << "[DEBUG] Nenhum dado recebido ou conexao fechada prematuramente." << endl;
            }
            cout << "[DEBUG] Fechando socket do cliente atual." << endl;
            closesocket(new_socket);
        }

        WSACleanup();
        return 0;
    } catch (const exception& e) {
        cout << "[Erro EXCECAO] " << e.what() << endl;
        system("pause");
        return 1;
    } catch (...) {
        cout << "[Erro EXCECAO DESCONHECIDA] O programa sofreu um erro critico!" << endl;
        system("pause");
        return 1;
    }
}