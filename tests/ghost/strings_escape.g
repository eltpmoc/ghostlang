// Sequencias de escape em strings: o REPL passou a decodificar como o C++ do compilado (13/11/2026).
function main() {
    print("a\"b|");
    print("x\ty|");
    print("l1\nl2|");
    print("barra \\ fim|");
    print(len("a\"b"));
}
