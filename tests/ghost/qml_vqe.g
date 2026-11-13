// qml_vqe.g - VQE: energia do estado fundamental da molecula H2 (F4)
import "libs/quantum.g";

function main() {
    qcircuit h2(2);
    h2.seed(3);
    h2.ansatz(2);
    hamiltoniano_h2(h2);

    print(f"Hamiltoniano: ${h2.observable_string()}");
    print(h2.draw());
    print(f"Energia inicial: ${h2.energy()} Ha");

    h2.set_verbose(100);
    double e = h2.minimize(300, 0.05);
    print(f"Energia VQE:      ${e} Ha");
    print("Referencia exata: -1.857275 Ha");
    print(f"Estado otimizado: ${h2.state()}");
}
