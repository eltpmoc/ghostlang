
import os
for f in os.listdir('.'):
    if f.endswith('.cpp'):
        print(f)
