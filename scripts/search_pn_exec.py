
with open('src/Interpreter.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(355, 375):
    print(f'{i+1}: {lines[i].strip()}')
