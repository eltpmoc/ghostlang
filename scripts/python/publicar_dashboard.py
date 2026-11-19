"""Copia o dashboard (dashboard/index.html e dashboard/ghostlang-status.json) para docs/dashboard/,
que o GitHub Pages publica em https://eltpmoc.github.io/ghostlang/dashboard/.

A fonte continua sendo dashboard/: os scripts e a documentação gravam lá. Rode este script depois
de atualizar o dashboard. O CTest docs_dashboard_publicado (--conferir) falha se a cópia ficou velha.

Uso: python scripts/python/publicar_dashboard.py [--conferir]
"""
import pathlib
import sys

RAIZ = pathlib.Path(__file__).resolve().parents[2]
ARQUIVOS = ['index.html', 'ghostlang-status.json']


def ler(p: pathlib.Path) -> bytes:
    # compara sem diferenca de fim de linha (o git pode trocar LF por CRLF no Windows)
    return p.read_bytes().replace(b'\r\n', b'\n') if p.exists() else b''


def main() -> int:
    conferir = '--conferir' in sys.argv[1:]
    destino = RAIZ / 'docs' / 'dashboard'
    velhos = [a for a in ARQUIVOS if ler(RAIZ / 'dashboard' / a) != ler(destino / a)]
    if conferir:
        for a in velhos:
            print(f'docs/dashboard/{a} difere de dashboard/{a}: rode python scripts/python/publicar_dashboard.py')
        if not velhos:
            print('docs/dashboard/ igual a dashboard/')
        return 1 if velhos else 0
    destino.mkdir(parents=True, exist_ok=True)
    for a in ARQUIVOS:
        (destino / a).write_bytes((RAIZ / 'dashboard' / a).read_bytes())
    print(f'{len(velhos)} arquivo(s) atualizado(s) em docs/dashboard/')
    return 0


if __name__ == '__main__':
    sys.exit(main())
