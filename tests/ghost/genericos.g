// Classes genericas (class Pilha<T>, class Par<A, B>): mesma saida no programa compilado (CTest golden_genericos)
// e no REPL (tests/repl/genericos.ghost).

interface Descritivel {
    function string descrever();
}

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

    function T desempilhar() {
        if (itens.length() == 0) {
            throw "pilha vazia";
        }
        return itens.pop();
    }

    function T topo() {
        return itens.last();
    }

    function int tamanho() {
        return itens.length();
    }

    function bool vazia() {
        return itens.length() == 0;
    }

    function Pilha<T> copiaInvertida() {
        Pilha<T> r;
        for (int k = itens.length() - 1; k >= 0; k--) {
            T v = itens[k];
            r.empilhar(v);
        }
        return r;
    }
}

class Par<A, B> implements Descritivel {
    A primeiro;
    B segundo;

    function Par(A a, B b) {
        primeiro = a;
        segundo = b;
    }

    function Par<B, A> trocado() {
        return Par<B, A>(segundo, primeiro);
    }

    function string descrever() {
        return "(" + primeiro + ", " + segundo + ")";
    }
}

class Caixa<T> {
    T valor;
    int trocas = 0;

    function Caixa(T v) {
        valor = v;
    }

    function T pegar() {
        return valor;
    }

    function void trocar(T novo) {
        valor = novo;
        trocas++;
    }
}

class PilhaDeInteiros extends Pilha<int> {
    function PilhaDeInteiros() {
        super();
    }

    function int soma() {
        int s = 0;
        foreach (v in itens) {
            s += v;
        }
        return s;
    }
}

function int somaPilha(Pilha<int> p) {
    int s = 0;
    while (!p.vazia()) {
        s += p.desempilhar();
    }
    return s;
}

function Pilha<string> palavras(string a, string b, string c) {
    Pilha<string> p;
    p.empilhar(a);
    p.empilhar(b);
    p.empilhar(c);
    return p;
}

function main() {
    print("== Pilha<T> ==");
    Pilha<int> numeros;
    for (int i = 1; i <= 4; i++) {
        numeros.empilhar(i * 10);
    }
    print("topo ", numeros.topo(), " tamanho ", numeros.tamanho(), " soma ", somaPilha(numeros), " (a original continua com ", numeros.tamanho(), ")");
    Pilha<string> ps = palavras("a", "b", "c");
    print("desempilha ", ps.desempilhar(), " ", ps.desempilhar(), " restam ", ps.tamanho());
    Pilha<string> inv = palavras("x", "y", "z").copiaInvertida();
    print("invertida: topo ", inv.topo(), " ", inv.itens);
    try {
        Pilha<double> vazia = new Pilha<double>();
        vazia.desempilhar();
    } catch (e) {
        print("erro: ", e);
    }

    print("== Par<A, B> ==");
    Par<string, int> idade = Par<string, int>("ana", 30);
    Par<int, string> invertido = idade.trocado();
    print(idade.descrever(), " ", invertido.descrever(), " ", idade.primeiro, " ", invertido.primeiro + 1);
    Descritivel d = Par<double, bool>(2.5, true);
    print("como interface: ", d.descrever());
    list<Par<string, double>> notas = [Par<string, double>("bia", 9.5), Par<string, double>("caio", 7)];
    print([n.primeiro + "=" + n.segundo for n in notas]);

    print("== Caixa<T> com varios tipos ==");
    Caixa<int> ci = Caixa<int>(7);
    Caixa<Ponto> cp = Caixa<Ponto>(Ponto(1, 2));
    Caixa<list<int>> cl = Caixa<list<int>>([1, 2, 3]);
    Caixa<Pilha<int>> cc = Caixa<Pilha<int>>(numeros);
    ci.trocar(8);
    ci.trocar(9);
    cp.valor.x = 5;
    cl.valor.push(4);
    print(ci.pegar(), " trocas ", ci.trocas, " | ponto ", cp.pegar().x, ",", cp.pegar().y, " | lista ", cl.pegar(), " | dentro ", cc.pegar().topo(), " de ", cc.pegar().tamanho());
    Caixa<int> copia = ci;
    copia.trocar(100);
    print("copia por valor: ", ci.pegar(), " ", copia.pegar());

    print("== heranca de Pilha<int> ==");
    PilhaDeInteiros pi = PilhaDeInteiros();
    pi.empilhar(5);
    pi.empilhar(6);
    print("soma ", pi.soma(), " topo ", pi.topo(), " como Pilha<int>: ", somaPilha(pi));
}
