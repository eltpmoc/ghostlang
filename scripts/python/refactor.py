import os
import re

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Replace GhostNumber(x, y) with GhostNumberG(x, y, 0.0)
    # Using regex: GhostNumber \s* ( \s* [^,]+ \s* , \s* [^)]+ \s* )
    def repl_2args(m):
        arg1 = m.group(1)
        arg2 = m.group(2)
        return f"GhostNumberG({arg1}, {arg2}, 0.0)"
    
    # We'll do a simple regex for the two-argument constructor.
    # It might have nested parentheses, so regex is tricky.
    # Actually, we can just replace the word GhostNumber with GhostNumberG
    # and manually fix the compile errors if any, but since we know most are simple:
    # like GhostNumber(val, 0)
    # GhostNumber(stod(val1), 0.0)
    
    # Let's just do GhostNumber -> GhostNumberG first.
    content = re.sub(r'\bGhostNumber\b', 'GhostNumberG', content)

    # Now fix GhostNumberG(a, b) -> GhostNumberG(a, b, 0.0)
    # We can use a heuristic: GhostNumberG(..., ...) where there is exactly one comma.
    # But C++ allows nested commas in function calls.
    # Let's just do `.real` -> `.x` and `.ghost` -> `.p` for GhostNumberG properties.
    # We know from grep that `.real` is ONLY used on GhostNumber variables in this codebase.
    content = content.replace('.real', '.x')
    content = content.replace('.ghost', '.p')
    
    # Also fix #include "GhostNumber.hpp" -> #include "GhostNumberG.hpp"
    content = content.replace('GhostNumber.hpp', 'GhostNumberG.hpp')

    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Updated {filepath}")

for root, dirs, files in os.walk('src'):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.hpp'):
            if f == 'GhostNumberG.cpp': continue
            process_file(os.path.join(root, f))
for root, dirs, files in os.walk('include'):
    for f in files:
        if f.endswith('.cpp') or f.endswith('.hpp'):
            if f == 'GhostNumberG.hpp': continue
            process_file(os.path.join(root, f))

