"""Confere os exemplos da documentação: cada bloco ```ghost de docs/MANUAL.md e docs/SPEC.md
passa no `ghost check` (CTest docs_exemplos).

Blocos sem `function main` são trechos: entram dentro de um main para a checagem. Um bloco
que não é programa (molde de sintaxe) deve usar ```text, não ```ghost.

Uso: python scripts/python/checar_docs.py <ghost.exe> [arquivo.md ...]
"""
import pathlib
import re
import subprocess
import sys
import tempfile

RAIZ = pathlib.Path(__file__).resolve().parents[2]
BLOCO = re.compile(r'^```ghost[ \t]*\n(.*?)^```', re.M | re.S)


def programa(codigo: str) -> str:
    if re.search(r'^\s*function\s+(\w+\s+)?main\s*\(', codigo, re.M):
        return codigo
    topo, corpo = [], []
    for linha in codigo.split('\n'):
        (topo if re.match(r'^\s*import\s', linha) else corpo).append(linha)
    tem_decl = re.search(r'^\s*(function|class|interface|circuit)\b', codigo, re.M)
    if tem_decl:
        return codigo + '\nfunction main() {\n}\n'
    return '\n'.join(topo) + '\nfunction main() {\n' + '\n'.join('    ' + l for l in corpo) + '\n}\n'


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    ghost = sys.argv[1]
    docs = [pathlib.Path(a) for a in sys.argv[2:]] or [RAIZ / 'docs' / 'MANUAL.md', RAIZ / 'docs' / 'SPEC.md', RAIZ / 'README.md']
    falhas = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        for doc in docs:
            texto = doc.read_text(encoding='utf-8')
            for m in BLOCO.finditer(texto):
                total += 1
                linha = texto[:m.start()].count('\n') + 1
                arq = pathlib.Path(tmp) / f'bloco_{total}.g'
                arq.write_text(programa(m.group(1)), encoding='utf-8')
                r = subprocess.run([ghost, 'check', str(arq)], cwd=RAIZ, capture_output=True, text=True,
                                   encoding='utf-8', errors='replace')
                if r.returncode != 0:
                    falhas += 1
                    print(f'[FALHA] {doc.name}:{linha}')
                    print((r.stdout + r.stderr).strip()[:800])
    print(f'{total - falhas} de {total} blocos ghost passam no ghost check')
    return 1 if falhas else 0


if __name__ == '__main__':
    sys.exit(main())
