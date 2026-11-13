// nucleo.g - teste do núcleo da linguagem no compilador (saída conferida por tests/golden/nucleo.expected)
// Tipos, operadores, controle de fluxo, funções, coleções, OOP, erros, result e hypothesis.

// ---------- funções: tipos, valor padrão, chamada antes da declaração ----------
function int dobro(int x) {
    return x * 2;
}

function double media(list<double> xs) {
    if (xs.length() == 0) {
        return 0.0;
    }
    return xs.sum() / xs.length();
}

function string saudacao(string nome, string prefixo = "Ola") {
    return prefixo + ", " + nome + "!";
}

function result<double> dividir(double a, double b) {
    if (b == 0) {
        return err("divisao por zero");
    }
    return ok(a / b);
}

function int fatorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * fatorial(n - 1);
}

// ---------- OOP ----------
class Animal {
    string nome;
    int patas = 4;

    function Animal(string n) {
        this.nome = n;
    }

    function string som() {
        return "...";
    }

    function string descrever() {
        return nome + " faz " + som() + " e tem " + patas + " patas";
    }
}

class Passaro extends Animal {
    bool voa = true;

    function Passaro(string n) {
        super(n);
        this.patas = 2;
    }

    function string som() {
        return "piu";
    }

    function string descrever() {
        return super.descrever() + " (voa: " + voa + ")";
    }
}

class Contador {
private:
    int valor;
public:
    function void incrementar() {
        valor++;
    }
    function int atual() {
        return valor;
    }
}

function main() {
    print("== tipos e operadores ==");
    int a = 17;
    int b = 5;
    bool ok_flag = a > b && !(b == 0);
    print(a % b, " ", a & b, " ", a | b, " ", a ^ b, " ", ~a, " ", a << 2, " ", a >> 1);
    print("bool: ", ok_flag, " ", true || false, " ", !ok_flag);
    print("precedencia: ", 2 + 3 * 4, " ", (2 + 3) * 4, " ", 10 - 4 - 3, " ", pow(2, 10));
    float f = 1.5;
    double d = 2.25;
    complex z = 3.0 - 2.0i;
    print("float ", f, " double ", d, " complex ", z);
    int c = 10;
    c += 5;
    c -= 3;
    c *= 2;
    c /= 4;
    c %= 4;
    c++;
    c--;
    ++c;
    print("compostos: ", c);
    string s = "Ghost";
    s += "Lang";
    print(s, " tem ", len(s), " letras; ternario: ", a > 10 ? "grande" : "pequeno");

    print("== controle de fluxo ==");
    int soma = 0;
    for (int i = 0; i < 10; i++) {
        if (i == 2) {
            continue;
        }
        if (i == 7) {
            break;
        }
        soma += i;
    }
    print("soma for com break/continue: ", soma);
    int w = 0;
    while (true) {
        w++;
        if (w >= 3) {
            break;
        }
    }
    int dw = 0;
    do {
        dw += 2;
    } while (dw < 7);
    print("while ", w, " do-while ", dw);
    int nota = 7;
    if (nota >= 9) {
        print("A");
    } else if (nota >= 7) {
        print("B");
    } else {
        print("C");
    }

    print("== switch ==");
    for (int k = 1; k <= 4; k++) {
        switch (k) {
            case 1:
                print("um");
                break;
            case 2, 3:
                print("dois ou tres");
                break;
            default:
                print("outro");
        }
    }
    string cor = "verde";
    switch (cor) {
        case "azul":
            print("frio");
            break;
        case "verde":
            print("natureza");
        case "amarelo":
            print("(caiu no proximo case)");
            break;
    }

    print("== funcoes ==");
    print(dobro(21), " ", fatorial(5), " ", saudacao("Ana"), " ", saudacao("Bia", "Oi"));

    print("== colecoes ==");
    list<int> xs = [5, 3, 8, 1];
    xs.push(4);
    print(xs, " tamanho ", xs.length(), " primeiro ", xs[0], " ultimo ", xs[-1]);
    xs.sort();
    print("ordenada ", xs, " soma ", xs.sum(), " max ", xs.max(), " contem 8? ", xs.contains(8));
    list<string> nomes = [];
    nomes.push("ana");
    nomes.push("bia");
    print(nomes.join(", "), " | fatia ", xs.slice(1, 3));
    list<double> notas = [7, 8.5, 9];
    print("media ", media(notas));
    map<string, int> idades = {"ana": 30, "bia": 25};
    idades["caio"] = 41;
    idades["ana"] += 1;
    print(idades, " tem bia? ", idades.has("bia"), " get ", idades.get("zeca", -1));
    int total = 0;
    foreach (x in xs) {
        total += x;
    }
    print("foreach total ", total);
    foreach (nome, idade in idades) {
        print(nome, " -> ", idade);
    }
    for (i in range(3)) {
        print("range ", i);
    }
    list<list<int>> grade = [[1, 2], [3, 4]];
    print("grade ", grade, " elemento ", grade[1][0]);

    print("== OOP ==");
    var gato = new Animal("Gato");
    Passaro p = new Passaro("Sabia");
    print(gato.descrever());
    print(p.descrever());
    Contador cont;
    cont.incrementar();
    cont.incrementar();
    print("contador ", cont.atual());

    print("== erros ==");
    try {
        print("antes");
        throw "falhou de proposito";
    } catch (e) {
        print("capturado: ", e);
    } finally {
        print("finally roda sempre");
    }
    try {
        print(xs[99]);
    } catch (e) {
        print("indice: ", e);
    }
    try {
        assert(1 + 1 == 3, "matematica quebrou");
    } catch (e) {
        print("assert: ", e);
    }
    result<double> r1 = dividir(10, 4);
    result<double> r2 = dividir(1, 0);
    print(r1, " ", r2, " ", r1.value(), " ", r2.value_or(-1), " erro: ", r2.error());
    if (r2.is_err()) {
        print("r2 falhou como esperado");
    }

    print("== hypothesis ==");
    double melhor = 0;
    hypothesis (var t in [1, 2, 3, 4]) {
        melhor = t * 10 - (t - 3) * (t - 3) * 5;
        collapse_max(melhor);
    }
    print("melhor linha do tempo: ", melhor);
    double menor = 100;
    hypothesis (var t in range(1, 6)) {
        menor = (t - 4) * (t - 4);
        collapse_min(menor);
    }
    print("menor: ", menor);
}
