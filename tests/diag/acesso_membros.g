// Controle de acesso (16/11/2026 em diante): private so na propria classe, protected na
// classe e nas derivadas, public em todo lugar. Antes, o compilador deixava ler c.saldo de fora.
class Conta {
    private double saldo;
    protected string dono;
    public int numero;

    function Conta(string d) {
        this.dono = d;
        this.saldo = 0;
        this.numero = 1;
    }
    private function void auditar() { print("ok"); }
    function double ver() {
        auditar();
        return saldo;
    }
}

class Poupanca extends Conta {
    function Poupanca(string d) { super(d); }
    function string quem() { return dono; }
    function double espiar() { return saldo; }
}

function main() {
    Conta c = new Conta("ana");
    print(c.numero, c.ver());
    print(c.saldo);
    c.dono = "bia";
    c.auditar();
}
