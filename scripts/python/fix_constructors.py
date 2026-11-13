import os
import re

def process_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Regex to find GhostNumberG(a, b) where a and b are expressions that might contain parentheses
    # Since regex can't easily parse nested parentheses, we can manually look at the lines from grep.
    
    # We'll just do a simpler replacement for the exact patterns we saw:
    lines = content.split('\n')
    for i, line in enumerate(lines):
        if 'GhostNumberG' not in line: continue
        # Find all occurrences of GhostNumberG(...) in this line
        # It's easiest to just use a regex for simple arguments:
        # GhostNumberG( [^,]+ , [^,)]+ )
        # This matches GhostNumberG(val, 0)
        # We need to replace it with GhostNumberG(val, 0.0, 0)
        
        # A more robust regex for exactly 2 arguments (no commas inside arguments):
        new_line = re.sub(r'GhostNumberG\s*\(\s*([^,]+?)\s*,\s*([^,)]+?)\s*\)', r'GhostNumberG(\1, 0.0, \2)', line)
        
        lines[i] = new_line

    new_content = '\n'.join(lines)
    
    # Let's fix the generatedCode one that has GhostNumberG(" + values[i] + ", 0.0) -> GhostNumberG(" + values[i] + ", 0.0, 0.0)
    # The regex handled it correctly!

    if new_content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Fixed {filepath}")

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

