// OOP: herança, super, sobrescrita e modificadores private/protected/public junto ao membro
// (16/11/2026 em diante). Mesma saída no compilador e no REPL (tests/repl/oop_acesso.ghost).
// Os acessos proibidos estão em tests/diag/acesso_membros.g e tests/repl/acesso_membros.ghost.
class Conta {
    private double saldo;
    protected string dono;
    public int numero;

    function Conta(string d, int n) {
        this.dono = d;
        this.numero = n;
        this.saldo = 0;
    }
    private function bool valido(double v) { return v > 0; }
    function void depositar(double v) {
        if (valido(v)) { saldo += v; }
    }
    function double ver() { return saldo; }
    function string descricao() { return "Conta " + numero + " de " + dono; }
}

class Poupanca extends Conta {
    protected double taxa;
    function Poupanca(string d, int n, double t) {
        super(d, n);
        this.taxa = t;
    }
    function void render() { depositar(ver() * taxa); }
    function string descricao() { return "Poupanca " + numero + " de " + dono + " (taxa " + taxa + ")"; }
}

function main() {
    Conta c = new Conta("ana", 1);
    c.depositar(100);
    c.depositar(-5);
    print(c.descricao(), ": ", c.ver());

    Poupanca p = new Poupanca("bia", 2, 0.5);
    p.depositar(40);
    p.render();
    print(p.descricao(), ": ", p.ver());
    p.numero = 20;
    print("numero publico alterado: ", p.numero);
}
