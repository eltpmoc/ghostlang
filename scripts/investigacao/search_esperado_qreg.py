
import os
for root, dirs, files in os.walk('src'):
    for f in files:
        p = os.path.join(root, f)
        with open(p, 'r', encoding='utf-8', errors='ignore') as fp:
            c = fp.read()
            if 'esperado qreg' in c:
                print(p)
