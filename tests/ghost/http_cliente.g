// Cliente HTTP (http_get, http_post) contra o servidor nativo da propria GhostLang, na mesma
// maquina: o servidor atende numa thread (spawn) e o programa principal faz as requisicoes.
import "../../libs/http.g"

function string rota_status(string req) {
    return http_ok("{\"status\": \"online\"}");
}

function string rota_eco(string req) {
    return http_ok("{\"recebido\": \"sim\"}");
}

function main() {
    server app = 18777;
    app.route("/status", rota_status);
    app.route("/eco", rota_eco);
    spawn {
        for (int i = 0; i < 40; i++) {
            app.listen_step(100);
        }
    }
    sleep(200);
    string r1 = http_get("http://127.0.0.1:18777/status");
    print("GET: ", json_extract(r1, "status"));
    string r2 = http_post("http://127.0.0.1:18777/eco", "{\"x\": 1}");
    print("POST: ", json_extract(r2, "recebido"));
    string r3 = http_get("http://127.0.0.1:18777/nao_existe");
    print("404: ", json_extract(r3, "error"));
    print("find: ", r1.find("online") != -1, " ", r1.find("xyz") != -1);
}
