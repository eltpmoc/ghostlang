import json

with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    d = json.load(f)

l = [r for r in d['recursos'] if r['nivel'] < 4]
print(f"Total de recursos abaixo de 4: {len(l)}")
for r in sorted(l, key=lambda x: (x['nivel'], x['categoria'], x['id'])):
    print(f"Nivel {r['nivel']} | {r['categoria']:<12} | {r['id']:<22} | {r['nome']}")
