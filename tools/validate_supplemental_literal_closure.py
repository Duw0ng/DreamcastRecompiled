#!/usr/bin/env python3
"""Validate that literal-loaded SH-4 callback targets used by generated supplemental code are registered.

This intentionally distinguishes callable literals from ordinary pointer/data literals: a
`LOAD_LITERAL32 -> sub_XXXXXXXX` only counts when the same local generated sequence saves
that register as a dynamic absolute target and performs a dynamic call/branch.
"""
from pathlib import Path
import re, sys
root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
registered=set(); refs=[]
for path in sorted(root.glob('generated*.cpp')):
    lines=path.read_text(errors='ignore').splitlines()
    for line in lines:
        m=re.search(r'register_target\(0x([0-9A-Fa-f]{8})u', line)
        if m: registered.add(int(m.group(1),16))
    for i,line in enumerate(lines):
        m=re.search(r'LOAD_LITERAL32 -> sub_([0-9A-Fa-f]{8})', line)
        if not m: continue
        window='\n'.join(lines[i:i+14])
        if 'SAVE_DYNAMIC_ABS_TARGET' not in window:
            continue
        if 'DYNAMIC_CALL' not in window and 'DYNAMIC_BRANCH' not in window:
            continue
        target=int(m.group(1),16)
        refs.append((target,path.name,i+1))
missing=[]
for target,path,line in refs:
    canonical=(target & 0x1fffffff) | 0x8c000000 if (target & 0x1fffffff) >= 0x0c000000 else target
    # Generated runtime accepts P1/P2 aliases, but registrations are canonical P1 in this tree.
    candidates={target, canonical}
    if not (candidates & registered):
        missing.append((target,path,line))
if missing:
    print(f'[FAIL] {len(missing)} callable literal reference(s) are not registered:')
    for target,path,line in missing:
        print(f'  0x{target:08X} at {path}:{line}')
    sys.exit(1)
print(f'[OK] {len(refs)} callable literal references covered by {len(registered)} registered AOT targets.')
