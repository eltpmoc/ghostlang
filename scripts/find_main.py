
import os
for root, dirs, files in os.walk('src'):
    for f in files:
        if 'main' in f.lower():
            print(os.path.join(root, f))
