
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(1115, 1150):
    print(f'{i+1}: {lines[i].strip()}')
