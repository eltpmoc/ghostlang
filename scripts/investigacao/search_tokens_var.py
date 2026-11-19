
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    if 'coll_memory[n] =' in l:
        print(f'{i+1}: {l.strip()}')
