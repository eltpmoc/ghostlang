// Lambdas, closures e fn<Ret(Args)>: mesmo resultado compilado (CTest golden_lambdas) e no REPL (tests/repl/lambdas.ghost)

interface Descritivel {
    function string descrever();
}

class Etiqueta implements Descritivel {
    string texto;
    function Etiqueta(string t) {
        texto = t;
    }
    function string descrever() {
        return f"<${texto}>";
    }
}

class Botao {
    string rotulo;
    fn<string(int)> aoClicar;
    static fn<int(int)> globalOp = (int x) => x * 10;

    function Botao(string r, fn<string(int)> cb) {
        rotulo = r;
        aoClicar = cb;
    }

    function string clicar(int cliques) {
        return f"${rotulo}->${aoClicar(cliques)}";
    }
}

class Escalador {
    int fator;
    int deslocamento;

    function Escalador(int f, int d) {
        fator = f;
        deslocamento = d;
    }

    function int ajuste(int v) {
        return v + deslocamento;
    }

    function fn<int(int)> criarAcumulador() {
        return (int x) => {
            int res = ajuste(x * fator);
            deslocamento += 1;
            return res;
        };
    }
}

class Pipeline<T> {
    list<fn<T(T)>> etapas = [];

    function void adicionar(fn<T(T)> f) {
        etapas.push(f);
    }

    function T executar(T valor) {
        T atual = valor;
        foreach (fn<T(T)> etapa in etapas) {
            atual = etapa(atual);
        }
        return atual;
    }
}

function int dobro(int x) {
    return x * 2;
}

function int somarUm(int x) {
    return x + 1;
}

function string formatarPar(string k, int v) {
    return f"${k}=${v}";
}

function fn<int()> criarContador(int inicio, int passo) {
    int atual = inicio;
    return () => {
        atual += passo;
        return atual;
    };
}

function fn<int(int)> somador(int base) {
    return (int x) => base + x;
}

function fn<int(int)> compor(fn<int(int)> f, fn<int(int)> g) {
    return (int x) => f(g(x));
}

function list<U> mapear<T, U>(list<T> xs, fn<U(T)> f) {
    list<U> out = [];
    foreach (T item in xs) {
        out.push(f(item));
    }
    return out;
}

function list<T> filtrar<T>(list<T> xs, fn<bool(T)> pred) {
    list<T> out = [];
    foreach (T item in xs) {
        if (pred(item)) {
            out.push(item);
        }
    }
    return out;
}

function Acc reduzir<T, Acc>(list<T> xs, Acc inicial, fn<Acc(Acc, T)> comb) {
    Acc acc = inicial;
    foreach (T item in xs) {
        acc = comb(acc, item);
    }
    return acc;
}

function main() {
    print("== lambdas basicas ==");
    var saudacao = () => "ola lambda";
    fn<int()> resposta = () => {
        int base = 40;
        return base + 2;
    };
    fn<int(int)> quadrado = (int x) => x * x;
    fn<string(string, int)> repetirTag = (string s, int n) => {
        string acc = "";
        for (int i = 0; i < n; i++) {
            if (i > 0) { acc = acc + "-"; }
            acc = acc + s;
        }
        return acc;
    };
    print(saudacao(), " | ", resposta(), " | ", quadrado(7), " | ", repetirTag("q", 3));

    print("== fn vazia, texto e conversoes ==");
    fn<int(int)> slot;
    print("slot vazio: ", !slot);
    try {
        print(slot(10));
    } catch (e) {
        print("capturado: ", e);
    }
    slot = (int x) => x + 5;
    print("estado: ", slot ? "ativo" : "vazio", " | ", slot, " | ", str(slot) + "/" + f"${slot}");
    fn<double(int)> alargada = (int x) => x + 1;
    print(f"alargada: ${alargada(4):.1f}");
    fn<void(string)> logar = (string msg) => {
        print(f"[void] ${msg}");
    };
    logar("executado");
    fn<void(int)> descartaRetorno = (int x) => {
        print(f"[descarta] ${x}");
        return x * 100;
    };
    descartaRetorno(3);

    print("== retorno explicito e result ==");
    var rampa = (int x) -> double => {
        if (x < 0) { return 0; }
        return x * 1.25;
    };
    print(f"rampa: ${rampa(-3):.2f} / ${rampa(2):.2f}");

    var divSegura = (double a, double b) -> result<double> => {
        if (b == 0) { return err("divisao por zero"); }
        return ok(a / b);
    };
    var somaInversos = (double x, double y) -> result<double> => {
        double ix = divSegura(12.0, x)?;
        double iy = divSegura(12.0, y)?;
        return ok(ix + iy);
    };
    print("inversos: ", somaInversos(3.0, 4.0), " | ", somaInversos(3.0, 0.0));

    print("== closures por valor ==");
    int externo = 10;
    var capValor = (int x) => x + externo;
    externo = 500;
    print("capValor: ", capValor(6));

    list<fn<int(int)>> somadoresFor = [];
    for (int i = 1; i <= 3; i++) {
        somadoresFor.push((int x) => x + i * 10);
    }
    print(f"for: ${somadoresFor[0](1)},${somadoresFor[1](1)},${somadoresFor[2](1)}");

    var multsComp = [(int x) => x * k for k in [2, 5, 9]];
    print(f"comp: ${multsComp[0](4)},${multsComp[1](4)},${multsComp[2](4)}");

    var c1 = criarContador(10, 5);
    int p1 = c1();
    int p2 = c1();
    var c2 = c1;
    int v1 = c1();
    int v2 = c2();
    int v3 = c1();
    print(f"contadores: p1=${p1}, p2=${p2}, c1=${v1}, c2=${v2}, c1=${v3}");

    var filaCont = [criarContador(0, 1), criarContador(100, 10)];
    int fc1 = filaCont[0]();
    int fc2 = filaCont[0]();
    int fc3 = filaCont[1]();
    print(f"filaCont: ${fc1},${fc2},${fc3}");

    print("== currying e funcoes nomeadas ==");
    fn<fn<string(string)>(string)> prefixar = (string p) => (string s) => f"[${p}] ${s}";
    var aviso = prefixar("AVISO");
    print(aviso("sistema pronto"), " | somador: ", somador(7)(8));

    var dobroMaisUm = compor(somarUm, dobro);
    fn<string(string, int)> fp = formatarPar;
    print("compor: ", dobroMaisUm(6), " | par: ", fp("qubits", 4));

    list<fn<int(int)>> ops = [dobro, somarUm, (int x) => x * x];
    list<int> resultadosOps = [];
    foreach (fn<int(int)> op in ops) {
        resultadosOps.push(op(5));
    }
    print("ops: ", resultadosOps);

    print("== alta ordem generica ==");
    list<int> nums = [1, 2, 3, 4, 5];
    print("mapear: ", mapear(nums, dobro), " | ", mapear(nums, (int x) => f"#${x}"));
    print("filtrar: ", filtrar(nums, (int x) => x % 2 == 1));
    print("reduzir: ", reduzir(nums, 0, (int acc, int x) => acc + x * x), " | ", reduzir(["a", "b", "c"], "ini", (string acc, string s) => f"${acc}-${s}"));

    Pipeline<int> pipe = Pipeline<int>();
    pipe.adicionar(dobro);
    pipe.adicionar((int x) => x + 3);
    pipe.adicionar((int x) => x * 10);
    print("pipeline: ", pipe.executar(4));

    print("== iife, ternario e mapa ==");
    print("iife expr: ", ((int a, int b) => a * b + 1)(6, 7));
    ((string s) => { print(f"iife bloco: ${s}"); })("rodou");

    bool crescer = false;
    int rTern = (crescer ? (int x) => x + 100 : (int x) => x - 100)(25);
    var modulo = (int x) => x >= 0 ? x : -x;
    print(f"ternario: ${rTern} / ${modulo(-42)}");

    map<string, fn<int(int, int)>> calc = {
        "add": (int a, int b) => a + b,
        "sub": (int a, int b) => a - b,
        "mul": (int a, int b) => a * b
    };
    print(f`calc: ${calc["add"](8, 3)},${calc["sub"](8, 3)},${calc["mul"](8, 3)}`);

    print("== classes e interfaces ==");
    Botao btn = Botao("Enviar", (int n) => f"ok(${n})");
    print(btn.clicar(2), " | ", btn.aoClicar(9));
    int gAntes = Botao.globalOp(7);
    Botao.globalOp = (int x) => x + 3;
    int gDepois = Botao.globalOp(7);
    print(f"static fn: ${gAntes} -> ${gDepois}");

    Escalador esc = Escalador(3, 10);
    var acum = esc.criarAcumulador();
    esc.fator = 999;
    int ac1 = acum(4);
    int ac2 = acum(4);
    print(f"this valor: ${ac1},${ac2},esc.desloc=${esc.deslocamento}");

    fn<Descritivel(string)> fabricaDesc = (string t) -> Descritivel => Etiqueta(t);
    Descritivel d = fabricaDesc("quantico");
    print("iface: ", d.descrever());
}
