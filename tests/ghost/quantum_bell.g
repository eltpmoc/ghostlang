// quantum_bell.g - Emaranhamento com o registrador quantico nativo (F3)
import "libs/quantum.g";

function main() {
    qreg q(2);
    q.seed(42);
    bell(q, 0, 1);

    print("=== Par de Bell ===");
    print(f"Estado: ${q}");
    print(q.draw());
    print(f"<ZZ> = ${q.expect("ZZ")}  <XX> = ${q.expect("XX")}  <Z0> = ${q.expect_z(0)}");
    print(q.histogram(1000));

    int a = q.measure(0);
    int b = q.measure(1);
    print(f"Medido: q0=${a} q1=${b} (sempre iguais)");
    print(f"Depois da medicao: ${q}");

    print("=== GHZ com 4 qubits ===");
    qreg g(4);
    g.seed(7);
    ghz(g);
    show(g);
    print(g.histogram(1000));
}
