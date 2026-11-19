
import os
for root, dirs, files in os.walk('tests'):
    for f in files:
        if 'algoritmo' in f.lower() or 'grover' in f.lower() or 'deutsch' in f.lower():
            print(os.path.join(root, f))
