// GhostLang WebAssembly Demo
// Funcoes exportadas com EMSCRIPTEN_KEEPALIVE para o navegador

function int somar(int a, int b) {
    return a + b;
}

function int multiplicar(int a, int b) {
    return a * b;
}

function double distancia(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

function int main() {
    print("[Ghost WASM] Modulo carregado no navegador com sucesso!");
    return 0;
}
