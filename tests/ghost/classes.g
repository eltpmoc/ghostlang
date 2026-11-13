// Classes: mesmo resultado no programa compilado (CTest golden_classes) e no REPL (tests/repl/classes.ghost)

class Ponto {
    double x;
    double y;

    function Ponto(double x = 0, double y = 0) {
        this.x = x;
        this.y = y;
    }

    function void mover(double dx, double dy) {
        x += dx;
        y += dy;
    }

    function Ponto somado(Ponto o) {
        return new Ponto(x + o.x, y + o.y);
    }

    function string texto() {
        return "(" + x + ", " + y + ")";
    }
}

// Retangulo tem um Ponto como campo, e largura e altura
class Retangulo {
    Ponto canto;
    double largura = 1;
    double altura = 1;

    function double area() {
        return largura * altura;
    }

    function void escalar(double k) {
        largura *= k;
        altura *= k;
    }

    function void dobrar() {
        escalar(2);
    }
}

// Conta com saldo privado e historico de operacoes
class Conta {
private:
    double saldo = 0;
    list<string> historico;

    function void registrar(string op) {
        historico.push(op);
    }
public:
    string titular;

    function Conta(string t, double inicial = 0) {
        titular = t;
        saldo = inicial;
        registrar("abertura " + inicial);
    }

    function void depositar(double v) {
        if (v <= 0) {
            throw "deposito precisa ser positivo";
        }
        saldo += v;
        registrar("deposito " + v);
    }

    function void sacar(double v) {
        if (v > saldo) {
            throw "saldo insuficiente para sacar " + v;
        }
        saldo -= v;
        registrar("saque " + v);
    }

    function double consultar() {
        return saldo;
    }

    function string extrato() {
        return historico.join("; ");
    }
}

// Forma é a classe base, Quadrado herda de Forma, Cubo herda de Quadrado
class Forma {
    string nome = "forma";

    function double area() {
        return 0;
    }

    function string resumo() {
        return nome + " com area " + area();
    }
}

// Quadrado herda de Forma
class Quadrado extends Forma {
    double lado;

    function Quadrado(double l) {
        nome = "quadrado";
        lado = l;
    }

    function double area() {
        return lado * lado;
    }
}

// Cubo herda de Quadrado, que herda de Forma
class Cubo extends Quadrado {
    function Cubo(double l) {
        super(l);
        nome = "cubo";
    }

    function double area() {
        return 6 * super.area();
    }

    function string resumo() {
        return super.resumo() + " (6 faces)";
    }
}

// mapa de string para int com contagem de itens
class Estoque {
    map<string, int> itens;
    int total = 0;

    function void adicionar(string nome, int qtd = 1) {
        if (itens.has(nome)) {
            itens[nome] += qtd;
        } else {
            itens[nome] = qtd;
        }
        total += qtd;
    }

    function int quantidade(string nome) {
        return itens.get(nome, 0);
    }
}

// pilha de inteiros com contagem de chamadas de funcoes
class Pilha {
    list<int> itens;

    function void empilhar(int v) {
        contar();
        itens.push(v);
    }

    function int desempilhar() {
        contar();
        return itens.pop();
    }

    function bool vazia() {
        return itens.length() == 0;
    }
}

// variavel global para contar chamadas de funcoes
int chamadas = 0;

// funcoes fora de classes podem acessar variaveis globais
function void contar() {
    chamadas++;
}

// funcoes fora de classes podem receber objetos como parametros e retornar objetos
function int esvaziar(Pilha p) {
    int s = 0;
    while (!p.vazia()) {
        s += p.desempilhar();
    }
    return s;
}

// funcoes fora de classes podem receber objetos como parametros e retornar objetos
function double distancia2(Ponto a, Ponto b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// funcoes fora de classes podem receber objetos como parametros e retornar objetos
function string apresentar(Forma f) {
    return f.resumo();
}

function main() {
    print("== objetos e metodos ==");
    Ponto a = new Ponto(1, 2); // construtor com new
    Ponto b = Ponto(4, 6); // construtor sem new
    Ponto origem; // construtor default
    a.mover(1, 1);
    print("a ", a.texto(), " b ", b.texto(), " origem ", origem.texto());
    print("soma ", a.somado(b).texto(), " distancia2 ", distancia2(a, b));

    print("== semantica de valor ==");
    Ponto c = a;
    c.x = 100;
    print("a.x ", a.x, " c.x ", c.x);

    print("== campo objeto ==");
    Retangulo r;
    r.canto.mover(2, 3);
    r.canto.x += 1;
    r.dobrar();
    r.largura += 0.5;
    print("canto ", r.canto.texto(), " area ", r.area());

    print("== private e excecoes ==");
    Conta conta = new Conta("Ana", 50);
    conta.depositar(25);
    try {
        conta.sacar(500);
    } catch (e) {
        print("erro: ", e);
    }
    try {
        conta.depositar(-1);
    } catch (e) {
        print("erro: ", e);
    }
    conta.sacar(30);
    print(conta.titular, " saldo ", conta.consultar());
    print("extrato: ", conta.extrato());

    print("== heranca, super e metodos virtuais ==");
    Forma f;
    Quadrado q = new Quadrado(3);
    Cubo cubo = new Cubo(2);
    print(f.resumo());
    print(q.resumo());
    print(cubo.resumo());
    print("cubo.lado ", cubo.lado, " cubo.nome ", cubo.nome);
    print("parametro do tipo base: ", apresentar(cubo));

    print("== listas de objetos ==");
    list<Ponto> pts = [new Ponto(1, 1), new Ponto(2, 2), new Ponto(3, 3)];
    pts[0].mover(10, 0);
    pts[1].y = -2;
    pts.push(new Ponto(9, 9));
    double somaX = 0;
    foreach (p in pts) {
        somaX += p.x;
    }
    print("pontos ", pts.length(), " primeiro ", pts[0].texto(), " segundo ", pts[1].texto(), " soma x ", somaX);

    print("== mapa em campo ==");
    Estoque est;
    est.adicionar("parafuso", 10);
    est.adicionar("porca");
    est.adicionar("parafuso", 5);
    print("parafusos ", est.quantidade("parafuso"), " porcas ", est.quantidade("porca"), " pregos ", est.quantidade("prego"), " total ", est.total);

    print("== metodos chamando funcoes ==");
    Pilha pilha;
    for (int i = 1; i <= 4; i++) {
        pilha.empilhar(i * 10);
    }
    print("soma ", esvaziar(pilha), " original ainda tem ", pilha.itens.length(), " chamadas ", chamadas);
}
