// Revisao de paridade (07/11): potencia ** (tipo inteiro e precedencia dos unarios), qualquer expressao
// dentro de ${...} no compilado, fatias com passo negativo, operacoes vetoriais com tipos mistos,
// sobrecarga, protected, static em classe generica e capturas de lambda.
// Mesma saida no programa compilado (CTest golden_revisao_paridade) e no REPL (tests/repl/revisao_paridade.ghost).

class Contador {
    static int total = 7;
}

function T maior<T>(T a, T b) {
    if (a > b) {
        return a;
    }
    return b;
}

class Base {
protected:
    int segredo = 7;
public:
    function int ver() {
        return segredo;
    }
}

class Filha extends Base {
    function int dobro() {
        return segredo * 2;
    }
}

class Caixa<T> {
    static int criadas = 0;
    T v;
    function Caixa(T x) {
        v = x;
        criadas++;
    }
}

function string tipo(int x) {
    return "int";
}

function string tipo(double x) {
    return "double";
}

function string tipo(string x) {
    return "string";
}

function string tipo(int a, int b) {
    return "dois ints";
}

function potencias() {
    int x = 3;
    double y = 2;
    print(-x ** 2, " ", 2 * -x ** 2, " ", -2 ** 2, " ", (-2) ** 2, " ", 5 -2 ** 2, " ", 2 ** -1, " ", 2.0 ** -2, " ", -2 ** -1);
    print(y ** 0.5, " ", x ** 2 % 4, " ", x ** y, " ", 2 ** 10, " ", 2 ** 3 ** 2, " ", -2 ** 3 ** 2, " ", ~1 ** 2, " ", !0 ** 2);
    print(1 ** -5, " ", (-1) ** -3, " ", 10 ** 0, " ", 0 ** 0, " ", 1.5 ** 2, " ", 3 * 2 ** 3 + 1, " ", -x, " ", - -x, " ", 4 - -x ** 2);
    int r = 3 ** 4;
    var v = 5 ** 2;
    print(r, " ", v, " ", v % 7, " ", f"${2 ** 8:,}", " ", [k ** 2 for k in range(4)]);
    try {
        print(0 ** -1);
    } catch (e) {
        print("erro: ", e);
    }
}

function fstrings() {
    list<int> xs = [1, 2, 3];
    string nome = "ana";
    var dobro = (int x) => x * 2;
    print(f"lista ${[1, 2, 3]} | comp ${[x * x for x in xs]} | pot ${2 ** 8:,} | gen ${maior<double>(2, 3.5)} | ${maior(4, 9)}");
    print(f"static ${Contador.total} | lambda ${dobro(21)} | aninhada ${f"[${nome:>5}]"} | set ${set([3, 1, 3])} | ${"chaves {ok}"}");
}

function diversos() {
    print(tipo(1), " ", tipo(2.5), " ", tipo("a"), " ", tipo(1, 2), " ", tipo(true ? 1 : 2));
    Filha f;
    print(f.ver(), " ", f.dobro());
    Caixa<int> a = Caixa<int>(1);
    Caixa<int> b = Caixa<int>(2);
    Caixa<string> c = Caixa<string>("x");
    print(Caixa<int>.criadas, " ", Caixa<string>.criadas);
    list<int> xs = [0, 1, 2, 3, 4, 5];
    print(xs[1:4], " ", xs[::-2], " ", xs[-2:], " ", xs[:2], " ", "abcdef"[1:5:2], " ", xs[10:20], " ", xs[4:1]);
    print(vec_add([1, 2], [3, 4]), " ", dot([1.5, 2], [2, 2]), " ", vec_add([1, 2], [0.5, 0.5]), " ", [3.0, 4.0].norm(), " ", mat_transpose([1, 2, 3, 4, 5, 6], 2, 3));
    var soma = (a, b) => a + b;
    print(soma(1, 2), " ", soma(1.5, 2), " ", soma("a", "b"));
    int n = 5;
    var addN = (int x) => x + n;
    n = 100;
    print(addN(1));
    var porRef = [&](int x) => x + n;
    n = 1000;
    print(porRef(1));
}

function main() {
    print("== potencia ==");
    potencias();
    print("== f-strings ==");
    fstrings();
    print("== diversos ==");
    diversos();
    list<int> xs = [0, 1, 2, 3, 4, 5];
    var k = -2;
    print(xs[::-1], " ", xs[::k], " ", "abcdef"[::-1], " ", "abcdef"[::-2], " ", xs[-1:-4:-1]);
}
