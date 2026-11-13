
with open('src/InterpreterCore.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i in range(2130, 2150):
    print(f'{i+1}: {lines[i].strip()}')
