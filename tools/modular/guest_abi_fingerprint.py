#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, re, sys
from pathlib import Path

def extract_struct(text: str, name: str = 'DCRuntime') -> str:
    needle = f'struct {name} {{'
    start = text.find(needle)
    if start < 0:
        raise ValueError(f'no se encontro {needle!r}')
    depth = 0
    seen = False
    for i in range(start, len(text)):
        c = text[i]
        if c == '{':
            depth += 1; seen = True
        elif c == '}':
            depth -= 1
            if seen and depth == 0:
                return text[start:i+1]
    raise ValueError(f'struct {name} incompleto')

def canonicalize(s: str) -> str:
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    return ''.join(s.split())

def fingerprint(path: Path) -> str:
    text = path.read_text(encoding='utf-8', errors='replace')
    body = canonicalize(extract_struct(text))
    return hashlib.sha256(body.encode('utf-8')).hexdigest()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('cached', type=Path)
    ap.add_argument('reference', type=Path, nargs='?')
    ns = ap.parse_args()
    try:
        a = fingerprint(ns.cached)
        if ns.reference is None:
            print(a); return 0
        b = fingerprint(ns.reference)
    except Exception as e:
        print(f'[ABI FINGERPRINT] ERROR: {e}', file=sys.stderr); return 3
    print(f'[ABI FINGERPRINT] cached={a}')
    print(f'[ABI FINGERPRINT] frozen={b}')
    if a != b:
        print('[ABI FINGERPRINT] MISMATCH')
        return 2
    print('[ABI FINGERPRINT] MATCH')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
