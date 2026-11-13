// result<T>, ok, err e o operador ?: mesmo resultado no programa compilado (CTest golden_result) e no REPL (tests/repl/result.ghost)

class Ponto {
    double x;
    double y;

    function Ponto(double x = 0, double y = 0) {
        this.x = x;
        this.y = y;
    }
}

class Tarefa {
    string nome;
    result<int> codigo;

    function Tarefa(string n) {
        nome = n;
    }

    function void concluir(int c) {
        codigo = ok(c);
    }

    function void falhar(string motivo) {
        codigo = err(nome + ": " + motivo);
    }

    function result<int> dobroDoCodigo() {
        return ok(codigo? * 2);
    }

    function string estado() {
        if (codigo.is_ok()) {
            return nome + " ok " + codigo.value();
        }
        return nome + " falhou (" + codigo.error() + ")";
    }
}

function result<double> dividir(double a, double b) {
    if (b == 0) {
        return err("divisao por zero");
    }
    return ok(a / b);
}

function result<double> raiz(double x) {
    if (x < 0) {
        return err("raiz de negativo: " + x);
    }
    return ok(sqrt(x));
}

// Propaga o erro com contexto, como um "?" manual
function result<double> raizDaDivisao(double a, double b) {
    result<double> d = dividir(a, b);
    if (d.is_err()) {
        return err("raizDaDivisao: " + d.error());
    }
    return raiz(d.value());
}

function result<int> metade(int n) {
    if (n % 2 != 0) {
        return err(n);
    }
    return ok(n / 2);
}

function result<list<int>> pares(int n) {
    if (n < 0) {
        return err("n negativo");
    }
    list<int> xs = [];
    for (int i = 0; i < n; i++) {
        xs.push(i * 2);
    }
    return ok(xs);
}

function result<Ponto> criarPonto(double x, double y) {
    if (x < 0 || y < 0) {
        return err("coordenada negativa");
    }
    return ok(new Ponto(x, y));
}

function string descrever(result<double> r) {
    if (r.is_ok()) {
        return "valor " + r.value();
    }
    return "falha: " + r.error();
}

// ---- operador ?: err sai da função com o mesmo erro; ok vira o valor ----
function result<double> mediaDasDivisoes(double a, double b, double c) {
    double x = dividir(a, b)?;
    double y = dividir(a, c)?;
    return ok((x + y) / 2);
}

function result<double> raizDaDivisao2(double a, double b) {
    return raiz(dividir(a, b)?);
}

function result<int> somaMetades(list<int> xs) {
    int s = 0;
    foreach (x in xs) {
        s += metade(x)?;
    }
    return ok(s);
}

function result<string> descreverPar(int n) {
    int m = metade(n)? * 10;
    string s = "metade*10=" + m;
    return ok(s + " / " + (metade(m)? + 1));
}

function result<int> comprimento(int n) {
    return ok(pares(n)?.length());
}

function result<double> escolher(bool usar, double a, double b) {
    double w = usar ? dividir(a, b)? : -1;
    return ok(w);
}

function result<int> comFinally(int n) {
    try {
        int m = metade(n)?;
        return ok(m);
    } finally {
        print("finally de comFinally(", n, ")");
    }
}

function main() {
    print("== ok e err ==");
    result<double> r1 = dividir(10, 4);
    result<double> r2 = dividir(1, 0);
    print(r1, " ", r2);
    print(r1.is_ok(), " ", r1.is_err(), " ", r2.is_ok(), " ", r2.is_err());
    print(r1.value(), " ", r2.value_or(-1), " erro: ", r2.error(), " erro de ok: [", r1.error(), "]");
    print(descrever(r1), " | ", descrever(r2));

    print("== value() de um erro lanca ==");
    try {
        double v = r2.value();
        print("nao deveria chegar aqui ", v);
    } catch (e) {
        print("capturado: ", e);
    }

    print("== conversao para o tipo declarado ==");
    result<int> t = ok(7 / 2.0);
    result<double> d = ok(5);
    result<string> s = ok("pronto");
    result<bool> b = ok();
    result<int> vazio;
    print(t, " ", d, " ", s, " ", b, " ", vazio);
    result<int> falha = err("sem numero");
    print("value_or trunca: ", falha.value_or(2.9));
    print("err de numero: ", metade(7), " ", metade(8));

    print("== reatribuicao e condicoes ==");
    r2 = dividir(9, 3);
    print("r2 agora ", r2);
    if (r2) {
        print("if (r2): ok");
    }
    if (!vazio) {
        print("if (!vazio): vazio e erro");
    }
    string sinal = r1.is_ok() ? "sim" : "nao";
    print("ternario: ", sinal);

    print("== encadeamento ==");
    print(raizDaDivisao(32, 2), " ", raizDaDivisao(1, 0), " ", raizDaDivisao(-8, 2));

    print("== texto ==");
    string texto = "r1 = " + r1;
    print(texto, " | ", str(r2), " | ", f"r1 vale ${r1}");

    print("== result de lista e de objeto ==");
    result<list<int>> ps = pares(4);
    print(ps, " tamanho ", ps.value().length(), " ", pares(-1));
    result<Ponto> p = criarPonto(3, 4);
    print("ponto ", p.value().x, ", ", p.value().y, " ", criarPonto(-1, 0).error());

    print("== lista de results ==");
    list<result<int>> rs = [metade(10), metade(3), metade(4)];
    int soma = 0;
    int falhas = 0;
    foreach (r in rs) {
        if (r.is_ok()) {
            soma += r.value();
        } else {
            falhas++;
        }
    }
    print(rs, " soma ", soma, " falhas ", falhas);

    print("== campo result em classe ==");
    Tarefa a = new Tarefa("backup");
    Tarefa c = new Tarefa("deploy");
    print(a.estado(), " | ", a.codigo);
    a.concluir(200);
    c.falhar("sem rede");
    print(a.estado(), " | ", c.estado(), " | ", c.codigo.is_err());

    print("== operador ? ==");
    print(mediaDasDivisoes(8, 2, 4), " ", mediaDasDivisoes(8, 0, 4), " ", mediaDasDivisoes(8, 2, 0));
    print(raizDaDivisao2(18, 2), " ", raizDaDivisao2(-18, 2), " ", raizDaDivisao2(1, 0));
    print(somaMetades([2, 4, 6]), " ", somaMetades([2, 5, 7]));
    print(descreverPar(8), " ", descreverPar(7), " ", descreverPar(6));
    print(comprimento(3), " ", comprimento(-1));
    print(escolher(true, 9, 3), " ", escolher(false, 9, 0), " ", escolher(true, 9, 0));
    print(comFinally(6));
    print(comFinally(3));
    print(a.dobroDoCodigo(), " ", c.dobroDoCodigo());
    int lado = r1.is_ok() ? 1 : -1;
    print("ternario continua valendo: ", lado, " ", r1.is_err() ? -2 : 2);
}
