import subprocess
import time
import sys
import os

if len(sys.argv) < 4:
    print("Uso: python test_swarm.py <node_bin> <swarm_bin> <work_dir>")
    sys.exit(1)

node_bin = sys.argv[1]
swarm_bin = sys.argv[2]
work_dir = sys.argv[3]
port = "8099"

weights_file = os.path.join(work_dir, "cerebro.weights")
if os.path.exists(weights_file):
    try:
        os.remove(weights_file)
    except Exception:
        pass

# Inicia o no operario
proc = subprocess.Popen([node_bin, port, "1"], cwd=work_dir)
time.sleep(1)

# Executa o swarm federado
res = subprocess.run([swarm_bin, "--test", port, "127.0.0.1"], cwd=work_dir, capture_output=True, text=True)
try:
    proc.wait(timeout=5)
except Exception:
    proc.kill()

if res.returncode != 0:
    print("Falha no swarm:")
    print("STDOUT:", res.stdout)
    print("STDERR:", res.stderr)
    sys.exit(1)

if not os.path.exists(weights_file):
    print("cerebro.weights nao foi gerado!")
    sys.exit(1)

try:
    os.remove(weights_file)
except Exception:
    pass

print("Swarm federado validado com sucesso!")
sys.exit(0)
