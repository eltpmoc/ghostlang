// Verificador de tipos (c41): vários erros de tipo e de nome, todos mostrados de uma vez
class Ponto {
    int x;
    function Ponto(int a) { x = a; }
    function int dobro() { return x * 2; }
}

function int soma(int a, int b) { return a + b; }
function avisar() { return 1; }
function int metade(int v) { return; }

function main() {
    int n = "dez";
    string s = 5;
    print(soma(1));
    print(soma("a", 2));
    Ponto p = Ponto(1, 2);
    print(p.y);
    p.voar();
    total = 3;
    print(naoExiste(2));
    if (s == 1) { print("igual"); }
    int n = 2;
    list<int> xs = ["a", "b"];
}
