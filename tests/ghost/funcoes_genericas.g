// Funcoes genericas (function T maior<T>(T a, T b)): tipo deduzido pelos argumentos ou explicito (maior<double>(...)).
// Mesma saida no programa compilado (CTest golden_funcoes_genericas) e no REPL (tests/repl/funcoes_genericas.ghost).

class Ponto {
    double x;
    double y;

    function Ponto(double x = 0, double y = 0) {
        this.x = x;
        this.y = y;
    }
}

class Pilha<T> {
    list<T> itens = [];

    function void empilhar(T valor) {
        itens.push(valor);
    }

    function int tamanho() {
        return itens.length();
    }
}

function T maior<T>(T a, T b) {
    if (a > b) {
        return a;
    }
    return b;
}

function T primeiro<T>(list<T> xs) {
    return xs[0];
}

function int contar<T>(T[] xs, T alvo) {
    int n = 0;
    foreach (x in xs) {
        if (x == alvo) {
            n++;
        }
    }
    return n;
}

function list<T> repetir<T>(T valor, int vezes) {
    list<T> r = [];
    for (int k = 0; k < vezes; k++) {
        r.push(valor);
    }
    return r;
}

function Pilha<T> pilhaDe<T>(list<T> xs) {
    Pilha<T> p;
    foreach (x in xs) {
        p.empilhar(x);
    }
    return p;
}

function int tamanhoDa<T>(Pilha<T> p) {
    return p.tamanho();
}

function string parTexto<K, V>(K chave, V valor) {
    return chave + " -> " + valor;
}

function T converter<T>(double v) {
    T r = v;
    return r;
}

function T somaTudo<T>(list<T> xs, T inicio) {
    T total = inicio;
    foreach (x in xs) {
        total += x;
    }
    return total;
}

function int profundidade<T>(T valor, int n) {
    if (n == 0) {
        return 0;
    }
    return 1 + profundidade(valor, n - 1);
}

function bool contem<T>(set<T> s, T x) {
    return s.contains(x);
}

function result<T> buscar<T>(list<T> xs, int i) {
    if (i < 0 || i >= xs.length()) {
        return err("indice " + i + " fora");
    }
    return ok(xs[i]);
}

function void anunciar<T>(T valor) {
    print("anuncio: ", valor);
}

function main() {
    print("== deducao pelo argumento ==");
    print(maior(3, 7), " ", maior(2.5, 1.5), " ", maior("ana", "bia"), " ", maior(true, false));
    list<string> nomes = ["caio", "ana", "bia"];
    print(primeiro(nomes), " ", primeiro([4.5, 2.0]), " ", contar([1, 2, 1, 3, 1], 1), " ", contar(nomes, "bia"));
    print(repetir("ok", 3), " ", repetir(7, 2), " ", parTexto("idade", 30), " ", parTexto(2, true));
    Pilha<string> p = pilhaDe(nomes);
    print("pilha de ", p.tamanho(), " ", tamanhoDa(p), " ", tamanhoDa(pilhaDe([1, 2, 3, 4])));
    set<int> s = [1, 5, 9];
    print(contem(s, 5), " ", contem(s, 4), " ", somaTudo([1, 2, 3], 10), " ", somaTudo(nomes, ">"));
    print(profundidade("x", 5), " ", buscar(nomes, 1), " ", buscar([1, 2], 5));
    Ponto pt = primeiro([Ponto(3, 4), Ponto(0, 0)]);
    print("ponto ", pt.x, " ", [maior(k, 2) for k in range(4)]);
    anunciar("generico como comando");
    anunciar<double>(3);

    print("== tipo explicito ==");
    print(maior<double>(3, 5.5), " ", maior<int>(9, 4), " ", converter<int>(7.9), " ", converter<double>(7.9));
    print(repetir<double>(1, 2), " ", parTexto<string, double>("pi", 3), " ", primeiro<int>([8, 9]));
}
