import json
with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    d = json.load(f)
niveis = {}
total_pts = 0
for r in d['recursos']:
    lvl = r['nivel']
    niveis[lvl] = niveis.get(lvl, 0) + 1
    total_pts += lvl
print('Recursos por nivel:', sorted(niveis.items()))
print(f'Total recursos: {len(d["recursos"])}')
max_pts = len(d['recursos']) * 5
print(f'Total pontos: {total_pts} / {max_pts} ({total_pts/max_pts*100:.2f}%)')
