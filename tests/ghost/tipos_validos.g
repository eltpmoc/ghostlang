// Verificador de tipos (c41): código válido que NÃO pode ser acusado (herança, interfaces, genéricos,
// sobrecarga, padrão, static, lambdas, compreensões, escopos e conversões que o C++ aceita)
interface Nomeado {
    function string nome();
    function string saudacao() { return "ola, " + nome(); }
}

class Animal implements Nomeado {
    protected:
    string especie;
    public:
    static int total = 0;
    function Animal(string e) { especie = e; Animal.total = Animal.total + 1; }
    function string nome() { return especie; }
    function string som() { return "..."; }
}

class Cao extends Animal {
    int idade;
    function Cao(int i) { super(i > 3 ? "cao velho" : "cao"); idade = i; }
    function string som() { return "au"; }
    function string ficha() { return especie + " (" + idade + " anos) " + this.som() + " " + saudacao(); }
}

class Caixa<T> {
    T valor;
    function Caixa(T v) { valor = v; }
    function T pegar() { return valor; }
}

function int soma(int a, int b) { return a + b; }
function double soma(double a, double b, double c) { return a + b + c; }
function string rotulo(string s, int n = 1) { return s + "#" + n; }
function T primeiro<T>(list<T> xs) { return xs[0]; }

function int usaGlobal() { return contador * 2; }   // global declarada depois

int contador = 5;

function main() {
    Cao c = Cao(5);
    Animal a = c;                    // derivada -> base
    print(c.ficha(), " | ", a.som(), " | ", Animal.total);
    print(soma(1, 2), " ", soma(1.0, 2.0, 3.5), " ", rotulo("x"), " ", rotulo("y", 2));
    Caixa<int> k = Caixa<int>(7);
    print(k.pegar() + 1, " ", primeiro<int>([4, 5]), " ", primeiro([2.5]));
    double d = 3;                    // int -> double
    int n = 3.9;                     // double -> int (como no C++)
    bool b = 1;
    string s = "n=" + n + " d=" + d; // texto + número concatena
    print(s, " ", b, " ", usaGlobal());
    for (int i = 0; i < 2; i++) { int x = i * 10; print(x); }
    for (int i = 0; i < 1; i++) { print("de novo ", i); }    // outro laço, outro i
    int x = 1;
    if (x > 0) { int y = x + 1; string x2 = "dentro"; print(y, x2); }
    var dobro = (int v) => v * 2;
    fn<int(int)> mais = (int v) -> int => v + x;            // captura x
    list<int> qs = [q * q for q in range(5) if q % 2 == 0];
    map<string, int> m = {"a": 1, "b": 2};
    foreach (chave, valor in m) { print(chave, "=", valor); }
    print(dobro(4), " ", mais(1), " ", qs, " ", len(qs));
    try { throw "falha"; } catch (string err) { print("pegou ", err); }
    ghost G = 1.5 - 2.5 * i + 3.5 * theta;   // i e a unidade, mesmo depois de for (int i ...)
    print(G);
    complex z = 2.0;
    print(z);
}
