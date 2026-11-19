#pragma once
// WebSocket (RFC 6455) para a GhostLang, sem bibliotecas externas:
//  - funções do protocolo (SHA-1 e Base64 do handshake, quadros com e sem máscara);
//  - cliente: ws_connect(url), ws_send(h, texto), ws_recv(h, timeout_ms), ws_close(h).
// O servidor (app.websocket / app.websocket_broadcast) fica em GhostServer.hpp.
// Mensagens de texto de até 16 MB; ping vira pong; close fecha a conexão dos dois lados.

#ifndef __EMSCRIPTEN__

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#ifndef SOCKET
#define SOCKET int
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef closesocket
#define closesocket close
#endif
#endif

namespace ghost_ws {

// ---- SHA-1 (só para o Sec-WebSocket-Accept) ----
inline std::string sha1(const std::string& msg) {
    uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    std::string m = msg;
    const uint64_t bits = static_cast<uint64_t>(msg.size()) * 8u;
    m += static_cast<char>(0x80);
    while (m.size() % 64 != 56) m += static_cast<char>(0);
    for (int i = 7; i >= 0; --i) m += static_cast<char>((bits >> (i * 8)) & 0xFF);
    auto rol = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
    for (size_t off = 0; off < m.size(); off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (static_cast<uint32_t>(static_cast<unsigned char>(m[off + 4 * i])) << 24) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(m[off + 4 * i + 1])) << 16) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(m[off + 4 * i + 2])) << 8) |
                   static_cast<uint32_t>(static_cast<unsigned char>(m[off + 4 * i + 3]));
        for (int i = 16; i < 80; ++i) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6u; }
            const uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    std::string out;
    for (uint32_t v : h)
        for (int i = 3; i >= 0; --i) out += static_cast<char>((v >> (i * 8)) & 0xFF);
    return out;
}

inline std::string base64(const std::string& in) {
    static const char* tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        const uint32_t n = (static_cast<unsigned char>(in[i]) << 16) | (static_cast<unsigned char>(in[i + 1]) << 8) |
                           static_cast<unsigned char>(in[i + 2]);
        out += tab[(n >> 18) & 63]; out += tab[(n >> 12) & 63]; out += tab[(n >> 6) & 63]; out += tab[n & 63];
    }
    if (i < in.size()) {
        uint32_t n = static_cast<unsigned char>(in[i]) << 16;
        if (i + 1 < in.size()) n |= static_cast<unsigned char>(in[i + 1]) << 8;
        out += tab[(n >> 18) & 63]; out += tab[(n >> 12) & 63];
        out += i + 1 < in.size() ? tab[(n >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

// Sec-WebSocket-Accept = base64(SHA-1(chave + GUID fixo do RFC 6455))
inline std::string accept_key(const std::string& client_key) {
    return base64(sha1(client_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
}

enum : int { OP_CONT = 0x0, OP_TEXT = 0x1, OP_BIN = 0x2, OP_CLOSE = 0x8, OP_PING = 0x9, OP_PONG = 0xA };

// Quadro único (FIN=1). O cliente mascara (obrigatório pelo RFC); o servidor não.
inline std::string encode_frame(int opcode, const std::string& payload, bool masked) {
    std::string f;
    f += static_cast<char>(0x80 | (opcode & 0x0F));
    const uint64_t n = payload.size();
    const unsigned char mbit = masked ? 0x80 : 0x00;
    if (n < 126) f += static_cast<char>(mbit | n);
    else if (n < 65536) { f += static_cast<char>(mbit | 126); f += static_cast<char>((n >> 8) & 0xFF); f += static_cast<char>(n & 0xFF); }
    else { f += static_cast<char>(mbit | 127); for (int i = 7; i >= 0; --i) f += static_cast<char>((n >> (i * 8)) & 0xFF); }
    if (!masked) return f + payload;
    static std::mt19937 rng{std::random_device{}()};
    unsigned char key[4];
    for (auto& k : key) k = static_cast<unsigned char>(rng() & 0xFF);
    for (auto k : key) f += static_cast<char>(k);
    for (size_t i = 0; i < payload.size(); ++i) f += static_cast<char>(payload[i] ^ key[i % 4]);
    return f;
}

inline bool send_all(SOCKET s, const std::string& data) {
    size_t off = 0;
    while (off < data.size()) {
        const int n = ::send(s, data.data() + off, static_cast<int>(data.size() - off), 0);
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

inline bool recv_exact(SOCKET s, char* buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        const int n = ::recv(s, buf + off, static_cast<int>(len - off), 0);
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

// Espera dados no socket por até timeout_ms (-1: sem limite)
inline bool wait_readable(SOCKET s, int timeout_ms) {
    fd_set rf;
    FD_ZERO(&rf);
    FD_SET(s, &rf);
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(static_cast<int>(s) + 1, &rf, nullptr, nullptr, timeout_ms < 0 ? nullptr : &tv) > 0;
}

// Lê um quadro inteiro (desmascara se vier mascarado). false: conexão caiu ou quadro inválido.
inline bool read_frame(SOCKET s, int& opcode, std::string& payload) {
    unsigned char h[2];
    if (!recv_exact(s, reinterpret_cast<char*>(h), 2)) return false;
    opcode = h[0] & 0x0F;
    const bool masked = (h[1] & 0x80) != 0;
    uint64_t n = h[1] & 0x7F;
    if (n == 126) {
        unsigned char e[2];
        if (!recv_exact(s, reinterpret_cast<char*>(e), 2)) return false;
        n = (static_cast<uint64_t>(e[0]) << 8) | e[1];
    } else if (n == 127) {
        unsigned char e[8];
        if (!recv_exact(s, reinterpret_cast<char*>(e), 8)) return false;
        n = 0;
        for (int i = 0; i < 8; ++i) n = (n << 8) | e[i];
    }
    if (n > (16u << 20)) return false; // limite de 16 MB por mensagem
    unsigned char key[4] = {0, 0, 0, 0};
    if (masked && !recv_exact(s, reinterpret_cast<char*>(key), 4)) return false;
    payload.assign(static_cast<size_t>(n), '\0');
    if (n && !recv_exact(s, &payload[0], static_cast<size_t>(n))) return false;
    if (masked)
        for (size_t i = 0; i < payload.size(); ++i) payload[i] = static_cast<char>(payload[i] ^ key[i % 4]);
    return true;
}

inline void net_init() {
#ifdef _WIN32
    static bool ok = [] { WSADATA d; return WSAStartup(MAKEWORD(2, 2), &d) == 0; }();
    (void)ok;
#endif
}

// ---- cliente ----
struct Client {
    std::map<int, SOCKET> conns;
    int next = 1;
    std::mutex mx;
    static Client& get() { static Client c; return c; }
};

inline SOCKET client_socket(int h) {
    Client& c = Client::get();
    std::lock_guard<std::mutex> g(c.mx);
    auto it = c.conns.find(h);
    if (it == c.conns.end()) throw std::runtime_error("ws: conexao " + std::to_string(h) + " nao existe ou ja foi fechada");
    return it->second;
}

} // namespace ghost_ws

// ws_connect("ws://host:porta/caminho"): faz o handshake e devolve o número da conexão
inline int ghost_ws_connect(const std::string& url) {
    using namespace ghost_ws;
    net_init();
    if (url.rfind("ws://", 0) != 0) throw std::runtime_error("ws_connect: use ws://host:porta/caminho (wss:// ainda nao e suportado)");
    std::string resto = url.substr(5), hostport = resto, path = "/";
    const size_t barra = resto.find('/');
    if (barra != std::string::npos) { hostport = resto.substr(0, barra); path = resto.substr(barra); }
    std::string host = hostport, port = "80";
    const size_t dp = hostport.rfind(':');
    if (dp != std::string::npos) { host = hostport.substr(0, dp); port = hostport.substr(dp + 1); }
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0 || !res) throw std::runtime_error("ws_connect: nao achei o host " + host);
    SOCKET s = INVALID_SOCKET;
    for (addrinfo* p = res; p; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        if (connect(s, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) break;
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    if (s == INVALID_SOCKET) throw std::runtime_error("ws_connect: conexao recusada em " + host + ":" + port);
    std::mt19937 rng{std::random_device{}()};
    std::string nonce;
    for (int i = 0; i < 16; ++i) nonce += static_cast<char>(rng() & 0xFF);
    const std::string key = base64(nonce);
    const std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + hostport + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                            "Sec-WebSocket-Key: " + key + "\r\nSec-WebSocket-Version: 13\r\n\r\n";
    if (!send_all(s, req)) { closesocket(s); throw std::runtime_error("ws_connect: falha ao enviar o handshake"); }
    std::string resp;
    char ch;
    while (resp.find("\r\n\r\n") == std::string::npos && resp.size() < 8192) {
        if (!wait_readable(s, 5000) || ::recv(s, &ch, 1, 0) != 1) { closesocket(s); throw std::runtime_error("ws_connect: o servidor nao respondeu ao handshake"); }
        resp += ch;
    }
    if (resp.find(" 101 ") == std::string::npos || resp.find(accept_key(key)) == std::string::npos) {
        closesocket(s);
        throw std::runtime_error("ws_connect: o servidor recusou o WebSocket em " + path);
    }
    Client& c = Client::get();
    std::lock_guard<std::mutex> g(c.mx);
    const int h = c.next++;
    c.conns[h] = s;
    return h;
}

inline bool ghost_ws_send(int h, const std::string& text) {
    return ghost_ws::send_all(ghost_ws::client_socket(h), ghost_ws::encode_frame(ghost_ws::OP_TEXT, text, true));
}

// Próxima mensagem de texto; "" se nada chegar em timeout_ms ou se a conexão fechar
inline std::string ghost_ws_recv(int h, int timeout_ms) {
    using namespace ghost_ws;
    const SOCKET s = client_socket(h);
    while (wait_readable(s, timeout_ms)) {
        int op = 0;
        std::string payload;
        if (!read_frame(s, op, payload)) return "";
        if (op == OP_TEXT || op == OP_BIN) return payload;
        if (op == OP_PING) send_all(s, encode_frame(OP_PONG, payload, true));
        if (op == OP_CLOSE) return "";
    }
    return "";
}

inline void ghost_ws_close(int h) {
    using namespace ghost_ws;
    Client& c = Client::get();
    SOCKET s;
    {
        std::lock_guard<std::mutex> g(c.mx);
        auto it = c.conns.find(h);
        if (it == c.conns.end()) return;
        s = it->second;
        c.conns.erase(it);
    }
    send_all(s, encode_frame(OP_CLOSE, "", true));
    closesocket(s);
}

// Nomes curtos usados pelos programas GhostLang
inline int ws_connect(const std::string& url) { return ghost_ws_connect(url); }
inline bool ws_send(int h, const std::string& text) { return ghost_ws_send(h, text); }
inline std::string ws_recv(int h, int timeout_ms) { return ghost_ws_recv(h, timeout_ms); }
inline void ws_close(int h) { ghost_ws_close(h); }

#endif // __EMSCRIPTEN__
