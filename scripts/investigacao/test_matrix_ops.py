import subprocess

code = """gate a(2, 2);
gate b(2, 2);
gate c(2, 2);
c = a + b;
show(c);
c = a - b;
show(c);
c = a * b;
show(c);
c = a * 2.0;
show(c);
c = a / 2.0;
show(c);
gate t(2, 2);
t = transpose(a);
show(t);
gate err_dim(3, 3);
c = a + err_dim;
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
