// Restricoes de tipo em genericos (<T extends Animal>, <T implements A & B>, <T extends number>).
// Mesma saida no programa compilado (CTest golden_restricoes) e no REPL (tests/repl/restricoes.ghost).

interface Medivel {
    function double medida();
}

interface Nomeado {
    function string nome();
}

class Animal {
    string especie;

    function Animal(string e = "?") {
        especie = e;
    }

    function string som() {
        return "...";
    }
}

class Gato extends Animal {
    function Gato() {
        super("gato");
    }

    function string som() {
        return "miau";
    }
}

class Caixa implements Medivel, Nomeado {
    double v;

    function Caixa(double v = 0) {
        this.v = v;
    }

    function double medida() {
        return v;
    }

    function string nome() {
        return "caixa";
    }
}

class Regua implements Medivel {
    double cm;

    function Regua(double c = 0) {
        cm = c;
    }

    function double medida() {
        return cm / 100;
    }
}

function T maiorMedida<T implements Medivel>(list<T> xs) {
    T m = xs[0];
    foreach (x in xs) {
        if (x.medida() > m.medida()) {
            m = x;
        }
    }
    return m;
}

function string descrever<T implements Medivel & Nomeado>(T x) {
    return x.nome() + ":" + x.medida();
}

function string falar<T extends Animal>(T a) {
    return a.especie + " faz " + a.som();
}

function T soma<T extends number>(list<T> xs) {
    T s = 0;
    foreach (x in xs) {
        s += x;
    }
    return s;
}

function string par<K extends number, V>(K chave, V valor) {
    return chave + "=" + valor;
}

class Ranking<T implements Medivel> {
    list<T> itens = [];

    function void adicionar(T x) {
        itens.push(x);
    }

    function double total() {
        double t = 0;
        foreach (x in itens) {
            t += x.medida();
        }
        return t;
    }

    function T melhor() {
        return maiorMedida(itens);
    }
}

class Abrigo<T extends Animal> {
    list<T> bichos = [];

    function void acolher(T a) {
        bichos.push(a);
    }

    function string coro() {
        return [b.som() for b in bichos].join(" ");
    }
}

class Util {
    static function T dobro<T extends number>(T x) {
        return x * 2;
    }

    function string ouvir<A extends Animal>(A a) {
        return a.som();
    }
}

function main() {
    print("== funcoes com restricao ==");
    print(maiorMedida([Caixa(1), Caixa(3), Caixa(2)]).medida(), " ", maiorMedida([Regua(50), Regua(120)]).medida());
    print(descrever(Caixa(4)), " ", falar(Gato()), " ", falar(Animal("peixe")));
    print(soma([1, 2, 3]), " ", soma([1.5, 2]), " ", par(1, "um"), " ", par<double, bool>(2, true));

    print("== classes com restricao ==");
    Ranking<Caixa> rk;
    rk.adicionar(Caixa(2));
    rk.adicionar(Caixa(5));
    print("total ", rk.total(), " melhor ", rk.melhor().medida());
    Abrigo<Gato> abrigo;
    abrigo.acolher(Gato());
    abrigo.acolher(Gato());
    Abrigo<Animal> misto;
    misto.acolher(Animal("pato"));
    misto.acolher(Gato());
    print(abrigo.coro(), " | ", misto.coro());

    print("== metodos com restricao ==");
    Util u;
    print(Util.dobro(21), " ", Util.dobro(1.25), " ", u.ouvir(Gato()), " ", u.ouvir<Animal>(Gato()));
}
