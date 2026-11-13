import os
import re

files_to_check = []
for root, dirs, files in os.walk('src'):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.hpp'):
            files_to_check.append(os.path.join(root, f))
for root, dirs, files in os.walk('include'):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.hpp'):
            files_to_check.append(os.path.join(root, f))

for filepath in files_to_check:
    if "GhostNumberG" in filepath:
        continue # Skip the GhostNumberG files themselves to avoid destroying them!
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 1. Replace GhostNumber( with GhostNumberG(
    # But wait, GhostNumber(x, y) needs to be GhostNumberG(x, y, 0.0, 0.0) maybe?
    # Actually, GhostNumberG has constructors:
    # GhostNumberG(double x, double y, double p, double theta2)
    # GhostNumberG(double x, double y, double p)
    # GhostNumberG(double x, bool is_scalar)
    # GhostNumberG(double value) => wait, if we call GhostNumberG(0.0, 0.0), which one does it hit?
    pass
