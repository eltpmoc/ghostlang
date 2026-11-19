import subprocess

code = """gate m1(2, 2);
gate m2(2, 2);
m2 = m1.transpose();
show(m2);
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
