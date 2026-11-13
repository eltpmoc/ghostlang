
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'qreg_memory' in l or 'qcircuit_memory' in l:
        print(f'{i+1}: {l.strip()}')
