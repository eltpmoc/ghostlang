// Gerador pela AST (c40, etapa 2): o núcleo coberto, com os detalhes que mudam o C++ gerado
int total = 0;                       // global (declaração + atribuição no main)
string rotulo = "inicio";            // global com valor
var passo = 2;                       // var no nível principal: local do main() do C++ (só o código solto usa)

function int dobro(int x) { return x * 2; }
function int aplicar(fn<int(int)> f, int v) { return f(v); }
function double media(list<int> xs, double peso = 1.0) {
    int soma = 0;
    foreach (x in xs) { soma += x; }
    return soma * peso / len(xs);
}
function string classificar(int n) {
    if (n < 0) {
        return "negativo";
    } else if (n == 0) {
        return "zero";
    } else if (n < 10) {
        return "pequeno";
    } else {
        return "grande";
    }
}

for (int i = 0; i < 3; i++) { total += i * passo; }
if (total > 2) { print("total > 2: ", total); } else { print("total pequeno"); }

function main() {
    print(rotulo, " ", total, " ", dobro(21), " ", aplicar(dobro, 5));
    list<int> xs = [4, 8, 15, 16, 23, 42];
    print(media(xs), " ", media([1, 2], 2.0), " ", xs[1:3], " ", xs[::-2], " ", len(xs));
    print(classificar(-3), " ", classificar(0), " ", classificar(7), " ", classificar(99));
    int a = -2 ** 2;
    double b = 2.0 ** -1;
    int c = (3 + 4) * 2 % 5;
    print(a, " ", b, " ", c, " ", 7 / 2, " ", (double)7 / 2, " ", a > 0 ? "pos" : "neg");
    print(f"lista ${xs} | soma ${dobro(3) + 1} | media ${media(xs):.2f} | ${classificar(5)}");
    var unicos = set([3, 1, 3, 2]);
    print(unicos, " ", bool(0), " ", bool(5), " ", ghost_to_set([1, 1]));
    int k = 0;
    do { k++; } while (k < 4);
    while (true) {
        k--;
        if (k == 2) { continue; }
        if (k < 1) { break; }
    }
    map<string, int> m = map<string, int>();
    m["um"] = 1;
    m["dois"] = 2;
    foreach (chave, valor in m) { print(chave, "=", valor); }
    ghost G = 1.5 - 2.5 * i + 3.5 * theta;
    complex z = 2.0 + 3.0i;
    print(G, " ", z, " ", k);
}
