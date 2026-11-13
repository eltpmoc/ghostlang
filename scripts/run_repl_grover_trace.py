import subprocess

code = """import "libs/quantum.g";
print("=== Grover: busca de |1011> entre 16 estados ===");
qreg busca(4);
busca.seed(1);
grover(busca, 11);
print(f"Iteracoes: ${grover_iterations(4)}");
print(f"P(|1011>) = ${busca.prob_state(11)}");
print(busca.histogram(1000));

print("=== Deutsch-Jozsa (3 qubits de entrada) ===");
print(f"constante 0 -> ${deutsch_jozsa(3, 0)} (0 = constante)");
print(f"constante 1 -> ${deutsch_jozsa(3, 1)} (0 = constante)");
print(f"balanceada  -> ${deutsch_jozsa(3, 2)} (1 = balanceada)");

print("=== QFT de |5> com 3 qubits ===");
qreg f(3);
qft_de(f, 5);
print(f.probabilities());
f.iqft();
print(f"IQFT devolve: ${f}");

print("=== Rotacoes ===");
qreg r(1);
r.ry(0, PI / 3.0);
print(f"RY(pi/3)|0> = ${r}  P(1) = ${r.prob(0)}");
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
