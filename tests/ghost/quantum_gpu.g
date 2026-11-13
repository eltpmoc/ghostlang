// quantum_gpu.g - Simulador quantico na GPU (CUDA) com fallback para CPU
// "auto" (padrao): registradores com 14+ qubits vao para a GPU se houver ghost_quantum_cuda.
// Use quantum_device("cuda") para exigir GPU, ou quantum_device("cpu") para forcar a CPU.
import "libs/quantum.g";

function main() {
    print(f"Backend: ${quantum_backend()}");

    qreg pequeno(4);
    qreg grande(20);
    print(f"qreg(4)  -> ${pequeno.device_name()}");
    print(f"qreg(20) -> ${grande.device_name()}");

    // Grover com 20 qubits: 804 iteracoes de oraculo + difusao sobre 1.048.576 amplitudes
    grande.seed(3);
    grover(grande, 777777);
    print(f"Grover(20): iteracoes ${grover_iterations(20)}, P(alvo) = ${grande.prob_state(777777)}");
    int achado = grande.measure_all();
    print(f"Medido: ${achado} (esperado 777777)");

    // GHZ com 22 qubits
    qreg g(22);
    ghz(g);
    print(f"GHZ(22) em ${g.device_name()}: P(0...0) = ${g.prob_state(0)}  <X...X> = ${g.expect("XXXXXXXXXXXXXXXXXXXXXX")}");
}
