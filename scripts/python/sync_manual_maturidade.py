"""Sincroniza as tabelas de maturidade do docs/MANUAL.md com o dashboard.

Cada linha `| **`id`** | **N** | ... | descrição |` recebe o nível atual do recurso
(dashboard/ghostlang-status.json). Para os recursos atualizados a partir de --desde
(data AAAA-MM-DD), a última coluna passa a ser a evidência do dashboard, que é o
texto conferido contra o código e os testes.

Uso: python scripts/python/sync_manual_maturidade.py [--desde 2026-11-13] [--conferir]
  --conferir: só lista o que mudaria e sai com 1 se houver diferença de nível.
"""
import argparse
import json
import pathlib
import re
import sys

RAIZ = pathlib.Path(__file__).resolve().parents[2]
LINHA = re.compile(r'^\| \*\*`(?P<id>[a-z0-9-]+)`\*\* \| \*\*(?P<nivel>\d)\*\* \|(?P<resto>.*)\|\s*$')


def atualizar_roadmap(dados: dict, so_conferir: bool) -> bool:
    """Reescreve o cabeçalho (bloco de citações '>') do ROADMAP a partir do dashboard."""
    import collections
    rec = dados['recursos']
    tot = sum(r['alvo'] for r in rec)
    got = sum(min(r['nivel'], r['alvo']) for r in rec)
    niveis = collections.Counter(r['nivel'] for r in rec)
    data = dados['projeto'].get('atualizado_em', '')
    data_br = '/'.join(reversed(data.split('-'))) if data else '?'
    chk = dados['checklist']
    feitos = sum(1 for x in chk if x.get('feito'))
    faltam = [x['texto'].split(' (')[0] for x in chk if not x.get('feito')]
    abaixo = [r['nome'] for r in rec if r['nivel'] <= 2]
    pct = f"{got / tot * 100:.1f}".replace('.', ',')
    linhas = [
        f"> **Situação em {data_br}:** versão {dados['projeto'].get('versao_atual', '?')}, em desenvolvimento. Progresso no dashboard: {pct}% "
        f"({niveis.get(4, 0)} recursos no nível 4, {niveis.get(3, 0)} no 3, {niveis.get(2, 0)} no 2 e {niveis.get(1, 0)} no 1; "
        f"nenhum no nível 5, que exige CI rodando e Linux/macOS comprovados).",
        f"> **Checklist oficial:** {feitos} de {len(chk)} itens. Faltam: {'; '.join(faltam) if faltam else 'nada'}.",
        f"> **Testes:** {dados['projeto'].get('testes_ctest', '?')} testes do CTest passando no Windows x64.",
        "> **Plataformas:** só Windows x64 testado. Linux (Fedora 43) e macOS 26 são as próximas etapas; o CI "
        "(`.github/workflows/ci.yml`, repositório https://github.com/eltpmoc/ghostlang) roda o Windows como alvo verificado e Linux/macOS como experimentais.",
        f"> **Ainda incompletos (nível 1 ou 2):** {', '.join(abaixo) if abaixo else 'nenhum'}.",
        "> *Cabeçalho gerado por `scripts/python/sync_manual_maturidade.py` a partir de `dashboard/ghostlang-status.json`.*",
    ]
    road = RAIZ / 'docs' / 'ROADMAP_GHOSTLANG_COMPLETA.md'
    texto = road.read_text(encoding='utf-8').split('\n')
    ini = next(i for i, l in enumerate(texto) if l.startswith('>'))
    fim = ini
    while fim < len(texto) and texto[fim].startswith('>'):
        fim += 1
    novo = texto[:ini] + linhas + texto[fim:]
    if novo != texto:
        print('roadmap  cabecalho atualizado')
        if not so_conferir:
            road.write_text('\n'.join(novo), encoding='utf-8')


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--desde', default=None)
    ap.add_argument('--conferir', action='store_true')
    args = ap.parse_args()

    dados = json.loads((RAIZ / 'dashboard' / 'ghostlang-status.json').read_text(encoding='utf-8'))
    recursos = {r['id']: r for r in dados['recursos']}
    manual = RAIZ / 'docs' / 'MANUAL.md'
    linhas = manual.read_text(encoding='utf-8').split('\n')

    mudou_nivel = []
    mudou_texto = []
    for i, linha in enumerate(linhas):
        m = LINHA.match(linha)
        if not m or m['id'] not in recursos:
            continue
        r = recursos[m['id']]
        colunas = m['resto'].split(' | ')
        nivel = str(r['nivel'])
        if nivel != m['nivel']:
            mudou_nivel.append(f"{m['id']}: {m['nivel']} -> {nivel}")
        if args.desde and r.get('atualizado_em', '') >= args.desde:
            ev = r['evidencia'].replace('|', '/').replace('\n', ' ').strip()
            if colunas[-1].strip() != ev:
                colunas[-1] = ' ' + ev + ' '
                mudou_texto.append(m['id'])
        linhas[i] = f"| **`{m['id']}`** | **{nivel}** |" + ' | '.join(colunas) + '|'

    roadmap_mudou = atualizar_roadmap(dados, args.conferir)
    for x in mudou_nivel:
        print('nivel  ', x)
    for x in mudou_texto:
        print('texto  ', x)
    if args.conferir:
        return 1 if (mudou_nivel or roadmap_mudou) else 0
    manual.write_text('\n'.join(linhas), encoding='utf-8')
    print(f'{len(mudou_nivel)} nivel(is) e {len(mudou_texto)} texto(s) atualizados em docs/MANUAL.md')
    return 0


if __name__ == '__main__':
    sys.exit(main())
