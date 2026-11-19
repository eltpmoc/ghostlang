
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'cast' in l.lower() or 'lparen' in l.lower() and 'int' in l.lower():
        print(f'{i+1}: {l.strip()}')
