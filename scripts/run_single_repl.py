import subprocess

code = """qreg g(16);
print(f"qreg(16) em ${g.device_name()}");
exit
"""
p = subprocess.Popen(['./build/ghost.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(code)
print('OUT:\n', out)
print('ERR:\n', err)
