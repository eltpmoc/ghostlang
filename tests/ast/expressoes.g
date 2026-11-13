// Forma da AST (precedência da docs/NUCLEO.md, genéricos, lambdas, compreensões e comandos)
class Caixa<T> {
    T valor;
    function Caixa(T v) { valor = v; }
    function T pegar() { return valor; }
}

function U trocar<U>(U a) { return a; }

function main() {
    int a = -2 ** 2 + 3 * 4 % 5;
    int b = a - 1;
    bool c = a > 1 && !(b < 2) || false;
    var d = c ? 2 ** 3 ** 2 : 0;
    Caixa<int> k = Caixa<int>(7);
    double e = trocar<double>(2.5);
    fn<int(int)> f = (int x) -> int => x * 2;
    list<int> xs = [x * x for x in range(5) if x % 2 == 0];
    map<string, int> m = {"um": 1, "dois": 2};
    print(f"total ${xs.size():03d} e ${m["um"]}", xs[1:3], k.pegar());
    for (int i = 0; i < 3; i++) { a += i; }
    foreach (chave, valor in m) { print(chave, valor); }
    switch (a) { case 1, 2: print("pouco"); break; default: print("muito"); }
    try { throw "falha"; } catch (string err) { print(err); } finally { print("fim"); }
}
