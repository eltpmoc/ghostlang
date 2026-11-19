// Simulador quantico grande (GPU com CUDA, CPU sem): os resultados nao dependem do dispositivo.
// O CTest quantum_gpu_dispositivo exige que qreg(20) rode em "cuda" quando ha GPU e backend.
import "../../libs/quantum.g"

function main() {
    qreg grande(20);
    grande.seed(3);
    grover(grande, 777777);
    print(f"Grover(20): iteracoes ${grover_iterations(20)}, P(alvo) = ${grande.prob_state(777777)}");
    print(f"Medido: ${grande.measure_all()}");
    qreg g(22);
    ghz(g);
    print(f"GHZ(22): P(0...0) = ${g.prob_state(0)}  <X...X> = ${g.expect("XXXXXXXXXXXXXXXXXXXXXX")}");
}
