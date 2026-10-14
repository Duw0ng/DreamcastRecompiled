#!/usr/bin/env python3
import argparse, hashlib, shutil, sys
from pathlib import Path

def norm(s: str) -> str:
    return ' '.join((s or '').strip().upper().split())

def read_manifest(path: Path):
    out={}
    for raw in path.read_text(encoding='utf-8-sig').splitlines():
        line=raw.strip()
        if not line or line.startswith(('#',';')) or '=' not in line: continue
        k,v=line.split('=',1); out[k.strip()]=v.strip()
    return out

def sha256(path: Path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def write_marker(path: Path, abi: str, digest: str, source: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f'DCR_PORTABLE_GUEST=1\nDCR_GUEST_ABI={abi}\nDCR_SAFE_SEED_SHA256={digest}\nDCR_SAFE_SEED_SOURCE={source}\n', encoding='ascii')

def main():
    ap=argparse.ArgumentParser()
    for n in ('manifest','seed','module','marker','legacy-marker','slug','title','volume','abi'):
        ap.add_argument('--'+n, required=True)
    a=ap.parse_args()
    manifest=Path(a.manifest); seed=Path(a.seed); module=Path(a.module)
    m=read_manifest(manifest)
    expected=m.get('MODULE_SHA256','').lower()
    checks=[
        (m.get('DCR_PORTABLE_SAFE')=='1','portable-safe flag'),
        (m.get('DCR_GUEST_ABI')==a.abi,'guest ABI'),
        (norm(m.get('GAME_SLUG'))==norm(a.slug),'game slug'),
        (not m.get('TITLE') or norm(m.get('TITLE'))==norm(a.title),'title'),
        (not m.get('VOLUME') or norm(m.get('VOLUME'))==norm(a.volume),'volume'),
        (len(expected)==64,'manifest SHA256'),
        (seed.is_file(),'seed file'),
    ]
    bad=[name for ok,name in checks if not ok]
    if bad:
        print('[SAFE GUEST] WARN: seed no aplicable: '+', '.join(bad))
        return 2
    seed_hash=sha256(seed)
    if seed_hash.lower()!=expected:
        print(f'[SAFE GUEST] WARN: SHA256 seed invalido: {seed_hash}')
        return 3
    current_hash=sha256(module) if module.is_file() else ''
    if current_hash.lower()==expected:
        print('[SAFE GUEST] El dcr_game_module.dll actual YA ES el portable-safe validado.')
    else:
        module.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(seed,module)
        if sha256(module).lower()!=expected:
            print('[SAFE GUEST] ERROR: copia SAFE no verifico SHA256.')
            return 4
        print('[SAFE GUEST] Modulo portable anterior adoptado SIN recompilar SH-4.')
    source=m.get('SOURCE','SAFE_SEED')
    write_marker(Path(a.marker),a.abi,expected,source)
    write_marker(Path(a.legacy_marker),a.abi,expected,source)
    print(f'[SAFE GUEST] PORTABLE SAFE = YES | SHA256={expected}')
    return 0
if __name__=='__main__':
    raise SystemExit(main())
