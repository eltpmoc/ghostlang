import json
from datetime import datetime

with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    d = json.load(f)

promocoes = {
    "qml-inspirado": {
        "nivel": 3,
        "evidencia": "Otimização quantum-inspired multiverso implementada no REPL com hypothesis e collapse_max/collapse_min. Validada por teste automatizado repl_hypothesis no CTest."
    },
    "net-api": {
        "nivel": 3,
        "evidencia": "Módulos de inferência ghost_api (API HTTP/CGI JSON) e ghost_ask (CLI preditivo de borda) compilados como executáveis autônomos e validados por teste automatizado ghost_api_ask no CTest."
    },
    "plat-portable": {
        "nivel": 3,
        "evidencia": "Distribuição independente GhostLang_Portable atualizada e sincronizada com binários (ghost, ghost_api, ghost_ask, ghost_node, ghost_swarm), headers e toolchain gcc/mingw."
    }
}

for r in d['recursos']:
    if r['id'] in promocoes:
        p = promocoes[r['id']]
        r['nivel'] = p['nivel']
        r['evidencia'] = p['evidencia']
        r['proximos_passos'] = []

total_pts = sum(r['nivel'] for r in d['recursos'])
max_pts = 488
progresso = round((total_pts / max_pts) * 100, 1)

d['progressoGeral'] = progresso
d['ultimaAtualizacao'] = datetime.utcnow().isoformat() + "Z"

if 'snapshots' in d:
    d['snapshots'].append({
        'data': '2026-11-09',
        'progresso': progresso,
        'recursos': len(d['recursos']),
        'nota': f'Promoção de qml-inspirado, net-api e plat-portable ao Nível 3 (Funcional) com suíte de 139 testes CTest 100% OK.'
    })

with open('dashboard/ghostlang-status.json', 'w', encoding='utf-8') as f:
    json.dump(d, f, indent=2, ensure_ascii=False)

print(f"Sucesso! Novo progresso geral: {progresso}% ({total_pts}/{max_pts} pontos).")
