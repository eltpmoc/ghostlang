import subprocess

code = """function void minha_h(qreg q) {
    q.h(0);
}
qreg busca(1);
minha_h(busca);
print("P: ", busca.prob(0));
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
