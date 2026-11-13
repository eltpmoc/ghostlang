import json

with open('dashboard/ghostlang-status.json', 'r', encoding='utf-8') as f:
    d = json.load(f)

targets = ['q-state-gate', 'q-apply', 'q-algoritmos', 'alg-matriz', 'net-http-server']
for r in d['recursos']:
    if r['id'] in targets:
        print(f"=== {r['id']} ===")
        print(json.dumps(r, indent=2, ensure_ascii=False))
