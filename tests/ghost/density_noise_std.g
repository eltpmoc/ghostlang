import "libs/quantum_density.g" as qd;

function main() {
    var rho = qd.qdensity_create(1);
    print(qd.qdensity_purity(rho));
    print(qd.qdensity_prob(rho, 0));
    qd.qdensity_h(rho, 0);
    print(qd.qdensity_prob(rho, 0));
    print(qd.qdensity_purity(rho));
    qd.qdensity_depolarize(rho, 0, 0.5);
    print(qd.qdensity_purity(rho) < 1.0);
    var rho_mixed = qd.qdensity_maximally_mixed(2);
    print(qd.qdensity_purity(rho_mixed));
    print(qd.qdensity_trace(rho_mixed));
}
