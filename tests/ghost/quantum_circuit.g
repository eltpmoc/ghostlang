// quantum_circuit.g - blocos circuit { } reutilizaveis (F3)
//
//   circuit nome(params) { portas }   declara um sub-circuito
//   q.nome(args);  ou  nome(q, args); aplica em um qreg ou qcircuit
import "libs/quantum.g";

// Funcao declarada ANTES do circuit que ela usa (declaracao antecipada)
function void preparar(qreg q) {
    q.emaranhar(0, 1);
}

// Par de Bell entre dois qubits quaisquer
circuit emaranhar(a, b) {
    h(a);
    cx(a, b);
}

// GHZ de n qubits: laco dentro do circuit
circuit cadeia_ghz(int n) {
    h(0);
    for (int i = 0; i < n - 1; i = i + 1) {
        cx(i, i + 1);
    }
}

// Rotacao com angulo como parametro
circuit girar(int k, double theta) {
    ry(k, theta);
    rz(k, theta / 2.0);
}

// Grover: um circuit chamando outro
circuit oraculo(int alvo) {
    phase_flip(alvo);
}
circuit passo_grover(int alvo) {
    oraculo(alvo);
    diffuser();
}

// Camada variacional para qcircuit (portas treinaveis)
circuit camada(int n) {
    for (int i = 0; i < n; i = i + 1) {
        ry_train(i);
    }
    for (int i = 0; i < n - 1; i = i + 1) {
        cx(i, i + 1);
    }
}

// Somador de Draper: soma k ao registrador (mod 2^n) usando a QFT.
// O inverso automatico somar_dg subtrai k.
circuit somar(int n, int k) {
    qft();
    double ang = PI * k;
    for (int j = n - 1; j >= 0; j = j - 1) {
        p(j, ang);
        ang = ang / 2.0;
    }
    iqft();
}

// Usa o inverso de outro circuit: U . T . U+ (conjugacao)
circuit fase_conjugada(a, b) {
    emaranhar(a, b);
    t(b);
    emaranhar_dg(a, b);
}

// Fase controlavel: usado na estimativa de fase (QPE) com fase_ctrl
circuit fase(int alvo, double th) {
    p(alvo, th);
}

function main() {
    print("=== Bell com circuit ===");
    qreg b(2);
    b.emaranhar(0, 1);
    print(f"estado: ${b}");
    print(f"<ZZ> = ${b.expect("ZZ")}");

    qreg b2(3);
    preparar(b2);
    emaranhar(b2, 1, 2);
    print(f"emaranhar(b2, 1, 2) depois de preparar: ${b2}");

    print("=== GHZ(5) com laco ===");
    qreg g(5);
    g.cadeia_ghz(5);
    print(f"estado: ${g}");
    print(g.draw());

    print("=== Rotacao parametrizada ===");
    qreg r(1);
    r.girar(0, PI / 3.0);
    print(f"P(1) = ${r.prob(0)}");

    print("=== Grover com circuit (alvo |1011>) ===");
    qreg busca(4);
    busca.h_all();
    for (int k = 0; k < grover_iterations(4); k = k + 1) {
        busca.passo_grover(11);
    }
    print(f"P(|1011>) = ${busca.prob_state(11)}");

    print("=== VQE do H2 com ansatz feito de circuits ===");
    qcircuit h2(2);
    h2.seed(3);
    h2.camada(2);
    h2.camada(2);
    hamiltoniano_h2(h2);
    print(f"parametros treinaveis: ${h2.num_params()}");
    double e = h2.minimize(300, 0.05);
    print(f"Energia VQE: ${e} Ha (exata: -1.857275)");

    print("=== Inverso automatico: nome_dg ===");
    qreg u(2);
    u.emaranhar(0, 1);
    u.emaranhar_dg(0, 1);
    print(f"emaranhar + emaranhar_dg: ${u}");

    qreg soma(4);
    soma.set_basis(5);
    soma.somar(4, 3);
    print(f"5 + 3 = ${soma}");
    somar_dg(soma, 4, 6);
    print(f"8 - 6 = ${soma}");

    qreg rot(1);
    rot.girar(0, 1.234);
    rot.girar_dg(0, 1.234);
    print(f"girar + girar_dg: P(1) = ${rot.prob(0)}");

    qreg cj(2);
    cj.h(0);
    cj.fase_conjugada(0, 1);
    print(f"fase_conjugada (inverso dentro de circuit): ${cj}");

    qcircuit vqc(2);
    vqc.seed(1);
    vqc.camada(2);
    vqc.camada_dg(2);
    print(f"qcircuit camada + camada_dg: ${vqc.num_params()} parametros, ${vqc.num_gates()} portas");

    print("=== Circuit controlado: nome_ctrl(controle, ...) ===");
    qreg cc(3);
    cc.h(2);
    cc.emaranhar_ctrl(2, 0, 1);
    print(f"emaranhar_ctrl com controle em superposicao: ${cc}");
    cc.emaranhar_dg_ctrl(2, 0, 1);
    cc.h(2);
    print(f"desfeito com emaranhar_dg_ctrl: ${cc}");

    // Estimativa de fase: U = P(2*pi*0.375) no autoestado |1> (q3); 3 qubits de contagem
    qreg qpe(4);
    qpe.x(3);
    for (int j = 0; j < 3; j = j + 1) {
        qpe.h(j);
    }
    int reps = 1;
    for (int j = 0; j < 3; j = j + 1) {
        for (int r = 0; r < reps; r = r + 1) {
            qpe.fase_ctrl(j, 3, 2.0 * PI * 0.375);
        }
        reps = reps * 2;
    }
    qpe.iqft_range(0, 3);
    print(f"QPE: ${qpe}  (contagem 011 = 3 -> fase 3/8 = 0.375)");
}
