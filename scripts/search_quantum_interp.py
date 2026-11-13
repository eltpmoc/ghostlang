
with open('src/InterpreterQuantum.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'qreg_memory' in l and ('insert' in l or 'emplace' in l or 'at(' in l or 'find(' in l):
        print(f'{i+1}: {l.strip()}')
