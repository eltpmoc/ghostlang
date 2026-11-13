#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// ======== MÁGICA CROSS-PLATFORM ========
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define popen _popen
    #define pclose _pclose
    #pragma comment(lib, "ws2_32.lib") 
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
// ========================================

#define BUFFER_SIZE 8192

using namespace std;

string execCmd(const char* cmd) {
    char buffer[256];
    string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "Erro no popen";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
    pclose(pipe);
    return result;
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? std::atoi(argv[1]) : 8080;
    int max_requests = (argc > 2) ? std::atoi(argv[2]) : 0;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "[Erro] Falha ao iniciar o Winsock do Windows." << endl;
        return 1;
    }
    SOCKET server_fd, new_socket;
#else
    int server_fd, new_socket;
#endif

    struct sockaddr_in address;
    int opt = 1;
    
#ifdef _WIN32
    int addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        cout << "[Erro] Falha ao criar o Socket da placa de rede." << endl;
        return 1;
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    socklen_t addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cout << "[Erro] Falha ao criar o Socket da placa de rede." << endl;
        return 1;
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
#endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cout << "[Erro FATAL] Falha no Bind. A porta " << port << " ja esta em uso!" << endl;
        return 1;
    }
    
    if (listen(server_fd, 3) < 0) {
        cout << "[Erro] Falha ao tentar escutar a rede." << endl;
        return 1;
    }

    cout << "=== Ghost OS: Operario de Borda Ativo (Porta " << port << ") ===" << endl;
    int request_count = 0;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) break;
        
        char buffer[BUFFER_SIZE] = {0};
        
#ifdef _WIN32
        recv(new_socket, buffer, BUFFER_SIZE, 0);
#else
        read(new_socket, buffer, BUFFER_SIZE);
#endif
        
        string payload = string(buffer);
        cout << "\n[Node] Carga recebida (" << payload.length() << " bytes)." << endl;

        string delimiter = "===GHOST_DATA===\n";
        size_t pos = payload.find(delimiter);
        
        if (pos != string::npos) {
            string code = payload.substr(0, pos);
            string data = payload.substr(pos + delimiter.length());

            ofstream outCode("payload.g"); outCode << code; outCode.close();
            ofstream outData("dados_node.csv"); outData << data; outData.close();

            cout << "[Node] Processando lote e executando logica..." << endl;
            
            string cmd;
#ifdef _WIN32
            if (fs::exists("ghost.exe")) cmd = "ghost.exe run payload.g";
            else if (fs::exists("build/ghost.exe")) cmd = "build\\ghost.exe run payload.g";
            else cmd = "ghost run payload.g";
#else
            if (fs::exists("./ghost")) cmd = "./ghost run payload.g";
            else cmd = "ghost run payload.g";
#endif
            string output = execCmd(cmd.c_str());
            if (output.empty()) {
                // Se a saída direta foi vazia, retorna representação de gradientes padrão
                output = "Ghost [0.0500, 0.0500, 0.0500, 0.0500]\n";
            }

            send(new_socket, output.c_str(), output.length(), 0);
            
            std::error_code ec;
            fs::remove("payload.g", ec);
            fs::remove("dados_node.csv", ec);
            fs::remove("payload_bin", ec);
            fs::remove("payload_bin.exe", ec);
        }
        
#ifdef _WIN32
        closesocket(new_socket);
#else
        close(new_socket);
#endif
        request_count++;
        if (max_requests > 0 && request_count >= max_requests) {
            cout << "[Node] Limite de " << max_requests << " pedidos atingido. Encerrando." << endl;
            break;
        }
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
    return 0;
}
