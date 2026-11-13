// Módulos (import "x.g" as m): mesma saída no programa compilado (CTest golden_modulos) e no REPL (tests/repl/modulos.ghost).
import "modulos/geometria.g" as geo;
import "modulos/geometria.g" as g2;
import "modulos/rotulos.g" as rot;
import "modulos/ciclo_a.g";

function double distancia(double a, double b) {
    return b - a;
}

function main() {
    print("== import ... as ==");
    geo.Ponto p = geo.Ponto(3, 4);
    geo.Ponto q = geo.Ponto();
    print("distancia ", geo.distancia(p, q), " | funcao local com o mesmo nome ", distancia(1, 5));
    print(geo.descrever(p), " | maior ", geo.maior(3, 9), " ", geo.maior<double>(2, 2.5));
    print("criados ", geo.criados, " | globais ", geo.ESCALA, " ", geo.escala(4), " | campo com o nome de funcao do modulo ", p.escala, " ", p.escalaDoModulo());
    geo.ESCALA = 5;
    g2.ESCALA = 7;
    print("dois apelidos, duas copias: ", geo.ESCALA, " ", g2.ESCALA, " ", g2.criados);
    geo.Medivel m = geo.Ponto(6, 8);
    print(m.medida(), " ", f"${geo.distancia(p, geo.Ponto(0, 0))}", " ", [geo.escala(k) for k in range(3)]);
    map<string, int> geoInfo = {"a": 1};
    print("variavel comum: ", geoInfo.keys());

    print("== modulo que importa modulo ==");
    print(rot.rotulo(3, 4), " ", rot.PREFIXO);

    print("== import sem apelido (com ciclo) ==");
    print(deA(), " ", deB());
}
