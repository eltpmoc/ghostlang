import subprocess

code = """gate q1(2, 2);
show(q1);
gate res(2, 2);
res = q1 + q1;
show(res);
res = q1 * q1;
show(res);
gate h_gate = hadamard;
show(h_gate);
gate py_gate = pauliy;
show(py_gate);
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
