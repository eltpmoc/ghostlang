// Gerador pela AST (c40, etapa 2): recursos menos comuns, com o mesmo C++ do Parser (CTest codegen_paridade)
interface Medida<T> {
    function T valor();
    function string rotulo() { return "medida " + valor(); }
}

class Contador implements Medida<int> {
    protected:
    int n;
    public:
    static int criados = 0;
    function Contador(int inicio) { n = inicio; Contador.criados = Contador.criados + 1; }
    function int valor() { return n; }
    function void somar(int k) { n += k; }
    static function string descrever() { return "contadores: " + Contador.criados; }
}

class Dobrador extends Contador {
    function Dobrador(int inicio) { super(inicio * 2); }
    function int valor() { return n * 10; }
}

function T maior<T extends number>(T a, T b) { return a > b ? a : b; }
function result<int> metade(int x) {
    if (x % 2 != 0) { return err("impar: " + x); }
    return ok(x / 2);
}
function result<int> quarto(int x) {
    int m = metade(x)?;
    int q = metade(m)?;
    return ok(q);
}
function U aplicarDuas<U>(fn<U(U)> f, U v) { return f(f(v)); }

circuit preparar(a, b) {
    h(a);
    cx(a, b);
}

function main() {
    Contador c = Contador(3);
    c.somar(4);
    Dobrador d = Dobrador(5);
    print(c.valor(), " ", d.valor(), " ", c.rotulo(), " ", Contador.descrever());

    var j = {"nome": "ghost", "versao": 2, "tags": ["q", "ia"]};   // fora de map<K, V>: texto JSON
    print(j);
    set<int> vazio = {};
    map<string, int> quadrados = {s: len(s) for s in ["a", "bb", "ccc"]};
    var pares = [[x, y] for x in range(3) for y in range(3) if x < y];
    var somas = {k: v * 2 for k, v in quadrados if v > 1};
    set<int> restos = {n % 3 for n in range(10)};
    print(vazio, " ", quadrados, " ", pares, " ", somas, " ", restos);

    for (int i = 0; i < 5; i++) {
        switch (i) {
            case 0, 1: print("pouco ", i);
            case 2: print("dois ou queda ", i); break;
            default: print("muito ", i);
        }
    }
    try {
        try { throw "interno"; } catch (string e) { print("pegou ", e); throw; }
    } catch (string e2) { print("de novo ", e2); }
    try { print("sem catch"); } finally { print("finally"); }

    int melhor = 0;
    hypothesis (var t in [7, 3, 9]) {
        melhor = t;
        collapse_min(t);
    }
    print("hypothesis ", melhor);

    print(maior(3, 9), " ", maior(2.5, 1.5), " ", quarto(12), " ", quarto(6));
    int base = 10;
    int total = 0;
    var porValor = [base](int v) => v + base;
    var porRef = [&total](int v) => { total += v; return total; };
    var tudoRef = [&](int v) => base * v;
    var poli = (v) => v * 2;
    porRef(5);
    porRef(7);
    print(porValor(1), " ", total, " ", tudoRef(3), " ", poli(21), " ", aplicarDuas((int v) => v + 1, 40));

    qreg q(2);
    preparar(q, 0, 1);
    preparar_dg(q, 0, 1);
    print("depois de preparar e desfazer: q.prob(0) = ", q.prob(0));
}
