
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'evalquantumat' in l.lower() or 'callquantum' in l.lower():
        print(f'{i+1}: {l.strip()}')
