
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(485, 525):
    print(f'{i+1}: {lines[i].strip()}')
