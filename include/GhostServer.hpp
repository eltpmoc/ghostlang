#ifndef GHOST_SERVER_HPP
#define GHOST_SERVER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <sstream>
#include <fstream>

#ifdef __EMSCRIPTEN__
// Dummy class para WASM que não suporta TCP Sockets puros
class GhostServer {
public:
    GhostServer(int port) {
        std::cout << "[GhostServer] WASM Mode: Servidor não instanciado (Sockets TCP não suportados no browser)." << std::endl;
    }
    void route(const std::string& path, std::function<std::string(std::string)> handler) {}
    void get(const std::string& path, std::function<std::string(std::string)> handler) {}
    void post(const std::string& path, std::function<std::string(std::string)> handler) {}
    void put(const std::string& path, std::function<std::string(std::string)> handler) {}
    void del(const std::string& path, std::function<std::string(std::string)> handler) {}
    void listen() {}
};

#else
// Implementação nativa (Cross-Platform)
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>

    // Macros de compatibilidade para Linux
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    #define WSAGetLastError() errno
    #define SOCKADDR struct sockaddr
    #define SD_SEND SHUT_WR
#endif


class GhostServer {
private:
    int port;
    std::map<std::string, std::map<std::string, std::function<std::string(std::string)>>> routes;
    // Mapeamento de prefixo de URL para diretório local: ["/public"] -> "public"
    std::vector<std::pair<std::string, std::string>> staticPaths;
    
    SOCKET ListenSocket;
    bool is_initialized;

    std::string mimeType(const std::string& path) {
        auto ends = [&](const std::string& ext) {
            return path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext;
        };
        if (ends(".html") || ends(".htm")) return "text/html; charset=utf-8";
        if (ends(".js"))   return "application/javascript";
        if (ends(".css"))  return "text/css";
        if (ends(".json")) return "application/json";
        if (ends(".png"))  return "image/png";
        if (ends(".jpg") || ends(".jpeg")) return "image/jpeg";
        if (ends(".gif"))  return "image/gif";
        if (ends(".svg"))  return "image/svg+xml";
        if (ends(".ico"))  return "image/x-icon";
        if (ends(".woff") || ends(".woff2")) return "font/woff2";
        if (ends(".ttf"))  return "font/ttf";
        if (ends(".mp4"))  return "video/mp4";
        if (ends(".webm")) return "video/webm";
        if (ends(".txt"))  return "text/plain";
        return "application/octet-stream";
    }

    std::string serveFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return "";
        std::ostringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }

    std::string buildStaticResponse(const std::string& filepath) {
        std::string content = serveFile(filepath);
        if (content.empty()) {
            std::string body = "{\"error\": \"Not Found\"}";
            std::ostringstream r;
            r << "HTTP/1.1 404 Not Found\r\n";
            r << "Content-Type: application/json\r\n";
            r << "Content-Length: " << body.length() << "\r\n";
            r << "Connection: close\r\n\r\n" << body;
            return r.str();
        }
        std::string mime = mimeType(filepath);
        std::ostringstream r;
        r << "HTTP/1.1 200 OK\r\n";
        r << "Content-Type: " << mime << "\r\n";
        r << "Content-Length: " << content.length() << "\r\n";
        r << "Cache-Control: public, max-age=3600\r\n";
        r << "Connection: close\r\n\r\n";
        r << content;
        return r.str();
    }

    std::string buildHttpResponse(const std::string& rawResponse) {
        std::string status = "200 OK";
        std::string contentType = "application/json";
        std::string body = rawResponse;

        if (rawResponse.length() > 7 && rawResponse.substr(0, 7) == "STATUS:") {
            size_t typeDelim = rawResponse.find("|TYPE:");
            size_t delim = rawResponse.find("|BODY:");
            
            size_t endStatus = (typeDelim != std::string::npos && typeDelim < delim) ? typeDelim : delim;
            
            if (delim != std::string::npos) {
                std::string code = rawResponse.substr(7, endStatus - 7);
                if (code == "200") status = "200 OK";
                else if (code == "201") status = "201 Created";
                else if (code == "400") status = "400 Bad Request";
                else if (code == "401") status = "401 Unauthorized";
                else if (code == "404") status = "404 Not Found";
                else if (code == "500") status = "500 Internal Server Error";
                else status = code + " Unknown";
                
                if (typeDelim != std::string::npos && typeDelim < delim) {
                    contentType = rawResponse.substr(typeDelim + 6, delim - (typeDelim + 6));
                }
                
                body = rawResponse.substr(delim + 6);
            }
        }

        std::ostringstream response;
        response << "HTTP/1.1 " << status << "\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        return response.str();
    }

    std::string buildHttp404() {
        std::string body = "{\"error\": \"Not Found\"}";
        std::ostringstream response;
        response << "HTTP/1.1 404 Not Found\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
        return response.str();
    }

public:
    GhostServer(int port) : port(port), ListenSocket(INVALID_SOCKET), is_initialized(false) {}

    void route(const std::string& path, std::function<std::string(std::string)> handler) {
        routes["GET"][path] = handler;
        routes["POST"][path] = handler;
        routes["PUT"][path] = handler;
        routes["DELETE"][path] = handler;
    }
    void get(const std::string& path, std::function<std::string(std::string)> handler) {
        routes["GET"][path] = handler;
    }
    void post(const std::string& path, std::function<std::string(std::string)> handler) {
        routes["POST"][path] = handler;
    }
    void put(const std::string& path, std::function<std::string(std::string)> handler) {
        routes["PUT"][path] = handler;
    }
    void del(const std::string& path, std::function<std::string(std::string)> handler) {
        routes["DELETE"][path] = handler;
    }
    // Registra um prefixo de URL para servir arquivos de um diretorio local
    // Exemplo: serve("/public", "public") faz /public/style.css ler de public/style.css
    void serve(const std::string& urlPrefix, const std::string& localDir) {
        staticPaths.push_back({urlPrefix, localDir});
        std::cout << "[GhostServer] Pasta estatica: " << urlPrefix << " -> " << localDir << std::endl;
    }

    void init_socket() {
        if (is_initialized) return;
        
#ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "[GhostServer] Erro no WSAStartup: " << iResult << std::endl;
            return;
        }
#else
        int iResult = 0;
#endif

        ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ListenSocket == INVALID_SOCKET) {
            std::cerr << "[GhostServer] Erro na criacao do socket: " << WSAGetLastError() << std::endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        iResult = bind(ListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
        if (iResult == SOCKET_ERROR) {
            std::cerr << "[GhostServer] Erro de Bind na porta " << port << " : " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        iResult = ::listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "[GhostServer] Erro no listen: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        std::cout << "[Ghost OS] Servidor HTTP nativo rodando na porta " << port << "..." << std::endl;
        is_initialized = true;
    }

    void process_client(SOCKET ClientSocket) {
        char recvbuf[4096];
        int recvbuflen = 4096;
        int iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

        if (iResult > 0) {
            std::string requestStr(recvbuf, iResult);
            
            size_t methodEnd = requestStr.find(' ');
            size_t pathEnd = requestStr.find(' ', methodEnd + 1);
            if (methodEnd != std::string::npos && pathEnd != std::string::npos) {
                std::string method = requestStr.substr(0, methodEnd);
                std::string fullPath = requestStr.substr(methodEnd + 1, pathEnd - methodEnd - 1);
                
                std::string path = fullPath;
                size_t queryPos = fullPath.find('?');
                if (queryPos != std::string::npos) {
                    path = fullPath.substr(0, queryPos);
                }
                
                std::string response;
                
                // 1. Checar rotas dinamicas registradas
                if (routes.find(method) != routes.end() && routes[method].find(path) != routes[method].end()) {
                    std::cout << "[GhostServer] " << method << " " << path << std::endl;
                    std::string body = "";
                    size_t bodyStart = requestStr.find("\r\n\r\n");
                    if (bodyStart != std::string::npos && bodyStart + 4 < requestStr.length()) {
                        body = requestStr.substr(bodyStart + 4);
                    }
                    std::string handlerResult = routes[method][path](body);
                    response = buildHttpResponse(handlerResult);
                }
                // 2. Checar prefixos de arquivos estaticos
                else if (method == "GET") {
                    bool servedStatic = false;
                    for (const auto& sp : staticPaths) {
                        const std::string& prefix = sp.first;
                        const std::string& dir    = sp.second;
                        if (path.size() >= prefix.size() && path.substr(0, prefix.size()) == prefix) {
                            std::string relativePath = path.substr(prefix.size());
                            // Segurança: bloquear path traversal (/../)
                            if (relativePath.find("..") != std::string::npos) {
                                response = buildHttp404();
                            } else {
                                // Montar caminho local: dir + relativePath
                                std::string sep = (dir.empty() || dir.back() == '/' || dir.back() == '\\') ? "" : "/";
                                std::string filepath = dir + sep + (relativePath.empty() ? "" : (relativePath[0] == '/' ? relativePath.substr(1) : relativePath));
                                std::cout << "[GhostServer] Static: " << path << " -> " << filepath << std::endl;
                                response = buildStaticResponse(filepath);
                            }
                            servedStatic = true;
                            break;
                        }
                    }
                    if (!servedStatic) {
                        response = buildHttp404();
                    }
                }
                else {
                    response = buildHttp404();
                }

                send(ClientSocket, response.c_str(), (int)response.length(), 0);
            }
        }
        
        shutdown(ClientSocket, SD_SEND);
        closesocket(ClientSocket);
    }

    void listen() {
        init_socket();
        if (!is_initialized) return;

        while (true) {
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                std::cerr << "[GhostServer] Erro no accept: " << WSAGetLastError() << std::endl;
                continue;
            }
            process_client(ClientSocket);
        }

        closesocket(ListenSocket);
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void listen_step(int timeout_ms) {
        init_socket();
        if (!is_initialized) return;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(ListenSocket, &readfds);
        
        timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        // ListenSocket + 1 é necessário em POSIX, ignorado no Windows
        int activity = select(ListenSocket + 1, &readfds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            return;
        }
        if (activity > 0) {
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket != INVALID_SOCKET) {
                process_client(ClientSocket);
            }
        }
    }
};

#endif
#endif
