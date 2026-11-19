import subprocess

code = """import "libs/quantum.g";
qreg busca(4);
busca.seed(1);
busca.h_all();
busca.phase_flip(11);
busca.diffuser();
print("P: ", busca.prob_state(11));
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
