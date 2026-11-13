// Módulo de geometria (usado com import ... as)
double ESCALA = 2;
int criados = 0;

interface Medivel {
    function double medida();
}

class Ponto implements Medivel {
    double x;
    double y;
    int escala = 1;

    function Ponto(double x = 0, double y = 0) {
        this.x = x;
        this.y = y;
        criados++;
    }

    function double medida() {
        return distancia(this, Ponto(0, 0));
    }

    function double escalaDoModulo() {
        return ESCALA * escala;
    }
}

function double distancia(Ponto a, Ponto b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

function T maior<T>(T a, T b) {
    if (a > b) {
        return a;
    }
    return b;
}

function string descrever(Medivel m) {
    return f"medida ${m.medida():.2f} (escala ${ESCALA})";
}

function int escala(int k) {
    return k * 10;
}
