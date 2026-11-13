
import os
for root, dirs, files in os.walk('src'):
    for f in files:
        if f.endswith('.cpp'):
            path = os.path.join(root, f)
            with open(path, 'r', encoding='utf-8', errors='ignore') as fp:
                c = fp.read()
                if 'spawn' in c or 'mutex' in c or 'sleep' in c:
                    print(path)
