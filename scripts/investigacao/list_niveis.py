import json

with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    d = json.load(f)

for r in d['recursos']:
    if r['nivel'] == 3:
        print(f"{r['id']} [{r['categoria']}]: {r['nome']}")
