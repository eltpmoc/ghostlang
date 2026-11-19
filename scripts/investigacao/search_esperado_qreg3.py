
with open('src/InterpreterClass.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'qreg' in l:
        print(f'{i+1}: {l.strip()}')
