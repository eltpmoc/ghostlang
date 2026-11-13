// Metodos genericos (function T m<T>(...) dentro de classes, static e em classes genericas).
// Mesma saida no programa compilado (CTest golden_metodos_genericos) e no REPL (tests/repl/metodos_genericos.ghost).

class Registro {
    list<string> linhas = [];
    int total = 0;

    // Anota um valor de qualquer tipo T, convertendo para string 
    function void anotar<T>(string rotulo, T valor) {
        linhas.push(rotulo + "=" + valor);
        total++;
    }
    // Retorna o ultimo valor anotado, ou um padrao se nao houver nenhum
    function T ultimoOu<T>(T padrao) {
        if (total == 0) {
            return padrao;
        }
        return padrao;
    }
    // Retorna uma lista com n elementos iguais a valor, de qualquer tipo T
    function list<T> repetir<T>(T valor, int n) {
        list<T> r = [];
        for (int k = 0; k < n; k++) {
            r.push(valor);
        }
        anotar("repetir", n);
        return r;
    }
    // Retorna uma string com o resumo das anotações
    function string resumo() {
        anotar<int>("resumo", total);
        return linhas.join("; ");
    }
    // Metodos genericos estaticos (static function T m<T>(...))
    static function T maior<T>(T a, T b) {
        if (a > b) {
            return a;
        }
        return b;
    }
    // Retorna uma lista com os dois elementos, em ordem decrescente
    static function list<T> par<T>(T a, T b) {
        return [maior(a, b), maior<T>(b, a)];
    }
}

// Funcoes genericas fora de classes (function T m<T>(...))
class Caixa<T> {
    T valor;
    // Construtor generico
    function Caixa(T v) {
        valor = v;
    }
    // Converte o valor para outro tipo U, se possivel
    function U converter<U>() {
        U r = valor;
        return r;
    }
    // Retorna uma nova Caixa<U> com o valor convertido para U
    function Caixa<U> com<U>(U novo) {
        return Caixa<U>(novo);
    }
    // Retorna uma lista com o valor e os outros elementos, de qualquer tipo T
    function list<T> junto<U>(list<U> outros, T extra) {
        list<T> r = [valor];
        foreach (o in outros) {
            r.push(valor);
        }
        r.push(extra);
        return r;
    }
}

// Classe generica com heranca
class RegistroFilho extends Registro {
    // Anota todos os elementos de uma lista de qualquer tipo T
    function void anotarTudo(list<int> xs) {
        foreach (x in xs) {
            anotar<int>("x", x);
        }
        anotar("fim", true);
    }
}
// Funcoes genericas fora de classes (function T m<T>(...))
function int contar<T>(T obj) {
    return obj.total;
}
// Retorna uma lista de rotulos de um objeto generico T, e anota um valor n no objeto
function list<string> rotulos<T>(T reg, int n) {
    reg.anotar<int>("de fora", n);
    return reg.linhas;
}

function main() {
    print("== metodos genericos ==");
    Registro r;
    r.anotar("idade", 30);
    r.anotar("nome", "ana");
    r.anotar<double>("pi", 3);
    r.anotar("ok", true);
    print(r.linhas, " total ", r.total);
    print(r.ultimoOu(7), " ", r.ultimoOu("x"), " ", r.ultimoOu<double>(2), " ", r.repetir("ab", 2), " ", r.repetir<double>(1, 3));
    print(r.resumo());

    print("== static generico ==");
    print(Registro.maior(3, 9), " ", Registro.maior("b", "a"), " ", Registro.maior<double>(2, 2.5), " ", Registro.par(4, 1));

    print("== em classe generica ==");
    Caixa<int> c = Caixa<int>(7);
    print(c.converter<double>(), " ", c.converter<bool>(), " ", c.com("texto").valor, " ", c.com<double>(2).valor, " ", c.junto(["a", "b"], 9));

    print("== heranca e funcoes genericas ==");
    RegistroFilho f;
    f.anotarTudo([1, 2]);
    print(f.linhas, " ", contar(f), " ", contar(r), " ", rotulos(r, 5).length());
    Registro copia = r;
    copia.anotar("so na copia", 1);
    print("copia ", copia.total, " original ", r.total);
}
