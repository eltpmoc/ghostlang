// Interfaces genericas: interface Colecao<T>, metodos padrao, restricoes em interfaces,
// checagem de tipos em list<Interface> e heranca de interfaces.

interface Colecao<T> {
    function int tamanho();
    function void adicionar(T item);
    function T primeiro();

    // Metodo padrao com implementacao default na interface
    function bool vazia() {
        return tamanho() == 0;
    }
}

interface Somavel<T extends number> extends Colecao<T> {
    function T somar();
}

class ListaNumerica<T extends number> implements Somavel<T> {
    list<T> dados = [];

    function ListaNumerica() {}

    function int tamanho() {
        return dados.length();
    }

    function void adicionar(T item) {
        dados.push(item);
    }

    function T primeiro() {
        return dados[0];
    }

    function T somar() {
        T total = 0;
        foreach (x in dados) {
            total += x;
        }
        return total;
    }
}

class PilhaGenerica<T> implements Colecao<T> {
    list<T> elementos = [];

    function PilhaGenerica() {}

    function int tamanho() {
        return elementos.length();
    }

    function void adicionar(T item) {
        elementos.push(item);
    }

    function T primeiro() {
        return elementos[0];
    }
}

class TextoColecao implements Colecao<string> {
    list<string> itens = [];

    function TextoColecao() {}

    function int tamanho() {
        return itens.length();
    }

    function void adicionar(string item) {
        itens.push(item);
    }

    function string primeiro() {
        return itens[0];
    }
}

function void inspecionarColecao(Colecao<string> c, string rotulo) {
    print(rotulo, ": tamanho=", c.tamanho(), " vazia=", c.vazia(), " primeiro=", c.primeiro());
}

function main() {
    print("== metodos padrao em interfaces ==");
    PilhaGenerica<string> pStr = PilhaGenerica<string>();
    print("pStr inicialmente vazia? ", pStr.vazia());
    pStr.adicionar("primeiro");
    pStr.adicionar("segundo");
    print("pStr apos adicionar vazia? ", pStr.vazia());
    inspecionarColecao(pStr, "pStr");

    print("== interfaces genericas e polimorfismo ==");
    Colecao<string> c1 = pStr;
    print("via interface: ", c1.primeiro(), " tamanho=", c1.tamanho(), " vazia=", c1.vazia());

    TextoColecao tc = TextoColecao();
    tc.adicionar("alfa");
    Colecao<string> c2 = tc;
    print("via interface texto: ", c2.primeiro(), " tamanho=", c2.tamanho());

    print("== list<Interface> ==");
    // Lista de interface homogenea
    list<Colecao<string>> colecoes = [c1, c2];
    print("colecoes na lista: ", colecoes.length());
    foreach (c in colecoes) {
        print("item da lista: ", c.primeiro());
    }

    print("== restricoes em interfaces genericas ==");
    ListaNumerica<int> numList = ListaNumerica<int>();
    numList.adicionar(10);
    numList.adicionar(20);
    numList.adicionar(30);
    print("soma numList = ", numList.somar(), " vazia? ", numList.vazia());

    Somavel<int> somador = numList;
    print("via Somavel<int>: soma=", somador.somar(), " primeiro=", somador.primeiro());
    print("fim interfaces genericas");
}
