
with open('src/InterpreterQuantum.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'callquantum(' in l.lower():
        print(f'{i+1}: {l.strip()}')
