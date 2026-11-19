"""Extrai os exemplos 'GhostLang Source' do Guia do Programador (HTML) e roda ghost check em cada um
(CTest guia_exemplos). Uso: python scripts/python/checar_guia.py <ghost.exe>"""
import html
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GHOST = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "ghost.exe")
GUIA = os.path.join(ROOT, "docs/GhostLang_Guia_Oficial_do_Programador.html")
OUT = os.path.join(os.environ["TEMP"], "guia_exemplos")
os.makedirs(OUT, exist_ok=True)
s = open(GUIA, encoding="utf-8").read()
blocos = re.findall(r'<span class="code-filename">([^<]*)</span><span>([^<]*)</span></div>\s*<pre>(.*?)</pre>', s, re.S)
ok = falha = 0
for i, (nome, tipo, corpo) in enumerate(blocos):
    if "GhostLang" not in tipo:
        continue
    codigo = html.unescape(re.sub(r"<[^>]+>", "", corpo))
    arq = os.path.join(OUT, f"ex{i:02d}.g")
    open(arq, "w", encoding="utf-8").write(codigo)
    r = subprocess.run([GHOST, "check", arq], cwd=ROOT, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.returncode == 0:
        ok += 1
        print(f"ok     ex{i:02d} {nome}")
    else:
        falha += 1
        primeiro = next((l for l in (r.stderr + r.stdout).splitlines() if "erro" in l), (r.stderr + r.stdout).strip()[:160])
        print(f"FALHA  ex{i:02d} {nome}: {primeiro[:220]}")
print(f"\n{ok} ok, {falha} com erro, de {ok + falha} exemplos")
sys.exit(1 if falha else 0)
