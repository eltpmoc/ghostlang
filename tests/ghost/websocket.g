// WebSocket (RFC 6455) no servidor nativo e no cliente (16/11/2026 em diante): o servidor atende
// numa thread (spawn); /eco responde a quem mandou e /sala repassa a todos (chat). Mesma saída no
// compilador e no REPL (tests/repl/websocket.ghost). As linhas [GhostServer] vêm da outra thread
// e o CTest as ignora (a ordem delas não é fixa).
function string eco(string msg) {
    return "eco: " + msg;
}

function string sala(string msg) {
    return "[sala] " + msg;
}

function main() {
    server app = 18788;
    app.websocket("/eco", eco);
    app.websocket_broadcast("/sala", sala);
    spawn {
        for (int i = 0; i < 60; i++) {
            app.listen_step(50);
        }
    }
    sleep(200);

    // eco: a resposta volta só para quem mandou
    int a = ws_connect("ws://127.0.0.1:18788/eco");
    ws_send(a, "ola");
    print(ws_recv(a, 3000));
    ws_send(a, "segunda mensagem");
    print(ws_recv(a, 3000));
    ws_close(a);

    // sala: a mensagem de um chega aos dois
    int b = ws_connect("ws://127.0.0.1:18788/sala");
    int c = ws_connect("ws://127.0.0.1:18788/sala");
    sleep(200);
    ws_send(b, "bia entrou");
    print("b recebeu: ", ws_recv(b, 3000));
    print("c recebeu: ", ws_recv(c, 3000));
    ws_close(b);
    ws_close(c);

    // rota sem WebSocket: conexão recusada com erro claro
    try {
        int d = ws_connect("ws://127.0.0.1:18788/nada");
        print("nao devia conectar: ", d);
    } catch (e) {
        print("erro: ", e);
    }
}
