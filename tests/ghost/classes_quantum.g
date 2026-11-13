// qreg e qcircuit como campos de classe: mesma saída no programa compilado (CTest golden_classes_quantum)
// e no REPL (tests/repl/classes_quantum.ghost).

circuit bell(a, b) {
    h(a);
    cx(a, b);
}

class Par {
    qreg q(2);
    int preparos = 0;

    function void preparar() {
        bell(q, 0, 1);
        preparos++;
    }

    function void inverter() {
        this.q.x(0);
    }

    function double p(int k) {
        return q.prob_state(k);
    }

    function string estado() {
        return f"${q} (preparos: ${preparos})";
    }
}

class Registrador {
    int n;
    qreg q(1);

    function Registrador(int n) {
        this.n = n;
        q = qreg(n);
    }

    function void superpor() {
        for (int k = 0; k < n; k++) {
            q.h(k);
        }
    }
}

class Ansatz {
    qcircuit c(2);

    function Ansatz() {
        c.ry_train(0);
        c.ry_train(1);
        c.cx(0, 1);
    }

    function int parametros() {
        return c.num_params();
    }
}

function double chanceDeIguais(Par par) {
    return par.p(0) + par.p(3);
}

function main() {
    Par a;
    print("novo: ", a.q, " ", a.estado());
    a.preparar();
    print("bell: ", a.estado(), " p00=", f"${a.p(0):.4f}", " p11=", f"${a.p(3):.4f}", " p01=", f"${a.p(1):.4f}");
    Par b = a;
    b.inverter();
    print("copia invertida: ", b.q, " | original: ", a.q);
    a.q.h(0);
    print("depois de a.q.h(0): ", f"${a.q.prob_state(0):.4f} ${a.q.prob_state(1):.4f} ${a.q.prob_state(2):.4f} ${a.q.prob_state(3):.4f} qubit1=${a.q.prob(1):.4f}");
    print("iguais em b: ", f"${chanceDeIguais(b):.4f}", " em a: ", f"${chanceDeIguais(a):.4f}");

    Registrador r = new Registrador(3);
    r.superpor();
    print("registrador de ", r.n, " qubits: p(000)=", f"${r.q.prob_state(0):.4f}", " p(111)=", f"${r.q.prob_state(7):.4f}");

    Ansatz z;
    print("ansatz: ", z.parametros(), " parametro(s)");

    list<Par> pares = [Par(), Par()];
    pares[1].preparar();
    print("lista: ", pares[0].q, " | ", pares[1].q, " | preparos ", pares[1].preparos);
}
