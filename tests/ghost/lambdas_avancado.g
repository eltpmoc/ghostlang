// Lambdas avancadas: inferencia de tipos nos parametros, capturas explicitas (incluindo [&]),
// e uso de nome direto de funcao generica como valor.

function T id<T>(T x) {
    return x;
}

function U aplicar<T, U>(T val, fn<U(T)> f) {
    return f(val);
}

function list<U> mapearLista<T, U>(list<T> xs, fn<U(T)> f) {
    list<U> res = [];
    foreach (x in xs) {
        res.push(f(x));
    }
    return res;
}

class Contador {
    int valor;

    function Contador(int v) {
        valor = v;
    }

    function fn<int()> getIncrementadorPorValor() {
        // [this] copia o objeto
        return [this]() => {
            valor += 10;
            return valor;
        };
    }

    function fn<int()> getIncrementadorPorRef() {
        // [&this] captura por referencia
        return [&this]() => {
            valor += 1;
            return valor;
        };
    }
}

function main() {
    print("== inferencia de tipos em parametros ==");
    // Lambda com parametro inferido
    var inc = (x) => x + 1;
    print("inc(10) = ", inc(10));
    print("inc(2.5) = ", inc(2.5));

    // Lambda inferida passada para funcao generica
    int r1 = aplicar(5, (n) => n * 3);
    string r2 = aplicar("Ghost", (s) => s + "Lang");
    print("aplicar: ", r1, " | ", r2);

    // Mapeamento com lambda de parametro inferido
    list<int> nums = [1, 2, 3, 4];
    list<int> dobros = mapearLista(nums, (x) => x * 2);
    print("dobros: ", dobros);

    print("== capturas explicitas ==");
    int a = 10;
    int b = 20;

    // [a] captura a por valor, nao permite b
    var fVal = [a](x) => x + a;
    print("fVal(5) = ", fVal(5));

    // [&a] captura por referencia e altera
    var fRef = [&a](x) => {
        a += x;
        return a;
    };
    print("fRef(5) = ", fRef(5), " a apos = ", a);

    // [=, &b] captura b por referencia e outros por valor
    var fMisto = [=, &b](x) => {
        b += a + x;
        return b;
    };
    print("fMisto(1) = ", fMisto(1), " b apos = ", b);

    // [&, a] captura a por valor e outros por referencia
    int c = 100;
    var fMisto2 = [&, a](x) => {
        c += a + x;
        return c;
    };
    print("fMisto2(2) = ", fMisto2(2), " c apos = ", c);

    // [] sem captura
    var fPura = [](int x, int y) => x + y;
    print("fPura(3, 4) = ", fPura(3, 4));

    print("== capturas de this ==");
    Contador cont = Contador(50);
    var fnVal = cont.getIncrementadorPorValor();
    print("fnVal 1: ", fnVal());
    print("fnVal 2: ", fnVal());
    print("cont.valor apos fnVal = ", cont.valor);

    var fnRef = cont.getIncrementadorPorRef();
    print("fnRef 1: ", fnRef());
    print("fnRef 2: ", fnRef());
    print("cont.valor apos fnRef = ", cont.valor);

    print("== nome direto de funcao generica ==");
    // Uso direto como valor com deducao ou especificacao de tipo
    fn<int(int)> fIdInt = id;
    print("fIdInt(42) = ", fIdInt(42));

    fn<string(string)> fIdStr = id;
    print("fIdStr(oi) = ", fIdStr("oi"));

    // Com tipo explicito
    var fIdDouble = id<double>;
    print("fIdDouble(3.14) = ", fIdDouble(3.14));

    // Passado diretamente como argumento para funcao de alta ordem
    list<string> palavras = ["ola", "mundo"];
    list<string> eco = mapearLista(palavras, id);
    print("eco: ", eco);
    print("fim lambdas avancadas");
}
