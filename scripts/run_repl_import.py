import subprocess

code = """import "libs/quantum.g";
qreg busca(4);
busca.seed(1);
grover(busca, 11);
print(f"P(|1011>) = ${busca.prob_state(11)}");
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
