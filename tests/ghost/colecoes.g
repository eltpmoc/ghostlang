// Coleções e texto: set<T>, compreensões de lista e formatação ${x:.2f} nas f-strings.
// Mesma saída no programa compilado (CTest golden_colecoes) e no REPL (tests/repl/colecoes.ghost).

class Turma {
    // alunos é um conjunto de strings, sem repetições
    set<string> alunos;
    function bool matricular(string nome) {
        return alunos.add(nome);
    }
}

// Retorna os divisores de n como um set<int>, e a soma dos elementos de um set<int>
function set<int> divisores(int n) {
    set<int> r;
    for (int d = 1; d <= n; d++) {
        if (n % d == 0) {
            r.add(d);
        }
    }
    return r;
}

// Soma os elementos de um set<int> usando foreach
function int somaDe(set<int> s) {
    int t = 0;
    foreach (x in s) {
        t += x;
    }
    return t;
}

// Ponto é uma classe simples com x e y, usada em compreensões de lista
class Ponto {
    double x;
    double y;
    function Ponto(double x = 0, double y = 0) {
        this.x = x;
        this.y = y;
    }
}

// Retorna os quadrados dos números pares de 0 a n-1 como uma lista de inteiros
function list<int> quadradosPares(int n) {
    return [k * k for k in range(n) if k % 2 == 0];
}

// Retorna os quadrados dos números pares de 0 a n-1 como uma lista de inteiros
function formatos() {
    double pi = 3.14159265;
    int n = 1234567;
    string nome = "Ana";
    bool ok = true;
    list<int> xs = [1, 2, 3];
    print(f"[${pi:.2f}] [${pi:10.3f}] [${pi:<10.1f}] [${pi:^10.2f}] [${pi:*>9.1f}]");
    print(f"[${n:,}] [${n:,d}] [${n:12,}] [${-n:+,}] [${n:+d}] [${42:05d}] [${-42:06d}]");
    print(f"[${pi:e}] [${pi:.3e}] [${pi * 1000000:.2E}] [${pi:g}] [${pi:.3}] [${0.000012345:.3g}]");
    print(f"[${255:x}] [${255:X}] [${8:o}] [${5:b}] [${-10:x}] [${0.256:.1%}] [${1:%}]");
    print(f"[${nome:>6}] [${nome:<6}] [${nome:^7}] [${nome:.2}] [${nome:-^9}] [${"ação":>6}] [${"coração":.3}]");
    print(f"[${ok}] [${ok:>6}] [${ok:d}] [${xs}] [${xs:>12}] [${n % 2 == 0 ? "par" : "impar"}] [${n > 0 ? pi : 0.0:.1f}]");
    print(f"[${pi * 2:.4f}] [${n / 1000:d}] [${7 / 2.0:.0f}] [${2.5:.0f}] [${-0.001:.2f}] [${1e20:,.0f}]");
    print(f"preco: R$ ${19.9:.2f} | ${12:3d}% | ${3.0} | ${3:.1f}");
}

// Retorna os divisores de n como um set<int>, e a soma dos elementos de um set<int>
function conjuntos() {
    set<int> a = [5, 1, 3, 3, 1];
    set<int> b = [3, 4, 5, 6];
    set<string> cores = ["verde", "azul", "verde", "amarelo"];
    set<double> d = [2.5, 1, 2.5];
    set<int> vazio;
    print(a, " ", b, " ", cores, " ", d, " ", vazio, " ", a.length(), " ", len(cores));
    print(a.union(b), " ", a.intersection(b), " ", a.difference(b), " ", b.difference(a));
    print(a.contains(3), " ", a.contains(4), " ", cores.has("azul"), " ", vazio.empty());
    print(a.add(7), " ", a.add(7), " ", a.remove(1), " ", a.remove(100), " ", a);
    set<int> pequeno = [3, 5];
    print(pequeno.is_subset(a), " ", a.is_superset(pequeno), " ", b.is_subset(a), " ", a.min(), " ", a.max());
    list<int> lista = a.to_list();
    lista.push(0);
    print(lista, " ", divisores(12), " ", somaDe(divisores(12)), " ", divisores(13));
    set<int> semRepetir = set([4, 4, 2, 9, 2]);
    print(semRepetir, " ", "texto " + cores, " ", str(b), " ", f"${a}");
    Turma t;
    print(t.matricular("Bia"), " ", t.matricular("Ana"), " ", t.matricular("Bia"), " ", t.alunos);
    set<int> copia = a;
    copia.add(99);
    print(a.contains(99), " ", copia.contains(99), " ", a == copia, " ", a == a);
    vazio.add(2);
    vazio.clear();
    print(vazio, " ", vazio.length());
}

// Mapa de string para int com contagem de itens
function compreensoes() {
    list<int> xs = [1, 2, 3, 4, 5, 6];
    print([x * 2 for x in xs]);
    print([x for x in xs if x % 2 == 1], " ", quadradosPares(7));
    list<double> metades = [x / 2.0 for x in xs];
    print(metades, " ", [x > 3 for x in xs]);
    list<string> nomes = ["ana", "bruno", "carla"];
    print([n + "!" for n in nomes], " ", [len(n) for n in nomes if n != "bruno"]);
    print(["${n}!" for n in nomes], " ", [f"${n}=${len(n)}" for n in nomes]);
    set<int> restos = [x % 3 for x in xs];
    print(restos, " ", [r * 10 for r in restos]);
    map<string, int> idades = {"ana": 30, "bia": 25, "caio": 41};
    print([k for k in idades if idades[k] > 28], " ", [idades[k] for k in idades]);
    print([c for c in "ghost" if c != "h"]);
    list<list<int>> tabela = [[i * j for j in range(1, 4)] for i in range(1, 4)];
    print(tabela, " ", [linha.sum() for linha in tabela]);
    list<Ponto> pts = [Ponto(i, i * i) for i in range(4)];
    print([p.y for p in pts if p.x > 0]);
    int limite = 4;
    print([x for x in xs if x < limite && x != 2], " ", [x + limite for x in [10, 20]]);
    print(len([x for x in xs if x > 100]), " ", [x for x in xs].length());
    int x = 99;
    list<int> ys = [x + 1 for x in xs];
    print(ys, " x continua ", x);
    double total = 0;
    foreach (v in [x * 1.5 for x in xs if x > 4]) {
        total += v;
    }
    list<int> qs = [x * x for x in range(3)];
    print("total ", total, " ", f"${qs}");
}

// Retorna os quadrados dos números pares de 0 a n-1 como uma lista de inteiros
function list<int> quadradosPares2(int n) {
    return [x * x for x in range(n) if x % 2 == 0];
}

// Função principal que chama as outras funções
function main() {
    print("== formatacao em f-strings ==");
    formatos();
    print("== set<T> ==");
    conjuntos();
    print("== compreensoes de lista ==");
    compreensoes();
    print("== compreensao de lista com classe ==");
    // um Ponto(x, x²) para cada quadrado par; objetos não têm print no compilado, então mostramos os campos
    list<int> quadrados = quadradosPares2(5);
    list<Ponto> pontos = [Ponto(q, q * q) for q in quadrados];
    print(quadrados, " ", pontos.length(), " ", [p.x for p in pontos], " ", [p.y for p in pontos]);

}
