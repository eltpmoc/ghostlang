// Módulo que usa outro módulo com apelido próprio
import "geometria.g" as geom;

string PREFIXO = "#";

function string rotulo(double x, double y) {
    geom.Ponto p = geom.Ponto(x, y);
    return PREFIXO + f"(${p.x}, ${p.y}) a ${geom.distancia(p, geom.Ponto(0, 0)):.1f}";
}

function main() {
    print("o main do modulo nao roda quando ele e importado");
}
