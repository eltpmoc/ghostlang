import json

with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

for r in data['recursos']:
    if r['nivel'] == 3:
        print(f"{r['id']} [{r['categoria']}]: {r['nome']}")
