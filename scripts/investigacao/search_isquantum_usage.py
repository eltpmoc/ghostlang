
with open('src/InterpreterQuantum.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(350, 375):
    print(f'{i+1}: {lines[i].strip()}')
