// libs/quantum.g - Biblioteca quantica da GhostLang (F3)
// Uso: import "libs/quantum.g";
//
// Portas nativas do qreg (metodos): h x y z s sdg t tdg sx rx ry rz p u
//   cx cy cz ch cp crx cry crz swap ccx cswap h_all x_all
//   qft iqft qft_range iqft_range phase_flip diffuser
// Medicao: measure(q) measure_all() histogram(shots) prob(q) prob_state(i) expect("ZZ")
// Visualizacao: print(q) show(q) q.draw() q.probabilities()
// Convencao: qubit 0 e o bit menos significativo; |q(n-1)...q0>.

// Par de Bell (|00> + |11>)/sqrt(2) nos qubits a e b
function void bell(qreg q, int a, int b) {
    q.h(a);
    q.cx(a, b);
}

// Estado GHZ (|0...0> + |1...1>)/sqrt(2) em todo o registrador
function void ghz(qreg q) {
    q.h(0);
    for (var k = 0; k < q.size() - 1; k = k + 1) {
        q.cx(k, k + 1);
    }
}

// Numero otimo de iteracoes de Grover para 1 item marcado entre 2^n
function int grover_iterations(int n) {
    return (int)floor(PI / 4.0 * sqrt(pow(2.0, n)));
}

// Busca de Grover: amplifica o estado |marcado>
function void grover(qreg q, int marcado) {
    q.h_all();
    int voltas = grover_iterations(q.size());
    for (var k = 0; k < voltas; k = k + 1) {
        q.phase_flip(marcado);
        q.diffuser();
    }
}

// Deutsch-Jozsa com n qubits de entrada e 1 ancilla (o qubit n).
// tipo: 0 = constante 0, 1 = constante 1, 2 = balanceada (paridade).
// Retorna 0 se a funcao for constante e 1 se for balanceada, com UMA consulta ao oraculo.
function int deutsch_jozsa(int n, int tipo) {
    qreg q(n + 1);
    q.x(n);
    q.h_all();
    if (tipo == 1) {
        q.x(n);
    }
    if (tipo == 2) {
        for (var k = 0; k < n; k = k + 1) {
            q.cx(k, n);
        }
    }
    for (var k = 0; k < n; k = k + 1) {
        q.h(k);
    }
    int resultado = 0;
    for (var k = 0; k < n; k = k + 1) {
        if (q.measure(k) == 1) {
            resultado = 1;
        }
    }
    return resultado;
}

// Prepara |valor> e aplica a QFT: as fases codificam valor/2^n
function void qft_de(qreg q, int valor) {
    q.set_basis(valor);
    q.qft();
}

// ---------------------------------------------------------------------------
// QML (F4) - qcircuit: circuitos parametrizados treinaveis
// Construcao: h x y z s t cx cz swap rx ry rz (fixas)
//             rx_train ry_train rz_train (treinaveis) · ansatz(camadas)
//             encode_rx/ry/rz(qubit, feature, escala) · encode_all(escala)
// Uso: observable("ZZ", coef) · energy() · minimize(passos, lr)            (VQE)
//      add_sample(rotulo, x0, x1, ...) · load_csv(arq) · fit(epocas, lr)
//      predict(x0, ...) · score(x0, ...) · accuracy() · save/load        (classificador)
// ---------------------------------------------------------------------------

// Hamiltoniano da molecula H2 (base STO-3G, 0.735 A) em 2 qubits.
// Energia exata do estado fundamental: -1.857275 Ha (parte eletronica).
function void hamiltoniano_h2(qcircuit c) {
    c.observable("II", -1.052373245772859);
    c.observable("IZ", 0.39793742484318045);
    c.observable("ZI", -0.39793742484318045);
    c.observable("ZZ", -0.01128010425623538);
    c.observable("XX", 0.18093119978423156);
}
