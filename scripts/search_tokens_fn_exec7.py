
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(2400, 2420):
    print(f'{i+1}: {lines[i].strip()}')
