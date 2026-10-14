"""Check that the Windows runtime DLLs correspond to the extracted source tree.

A source revision string alone does not establish that CMake/MSBuild output was
rebuilt. This manifest ties the SHA256 of runtime sources to the SHA256 of DLLs
and only grants cache reuse when both still match. No SH-4 guest files touched.
"""
from __future__ import annotations
import hashlib
import json
import sys
from pathlib import Path

EXTENSIONS = {'.cpp', '.c', '.hpp', '.h', '.hlsl', '.cmake'}
REQUIRED_DLLS = ('Release/DreamcastRuntime.dll',
                 'plugins/Release/dcr_network.dll',
                 'plugins/Release/dcr_maple.dll',
                 'plugins/Release/dcr_gdrom.dll')


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def source_digest(runtime: Path) -> str:
    digest = hashlib.sha256()
    files = sorted(path for path in runtime.rglob('*')
                   if path.is_file() and (path.suffix.lower() in EXTENSIONS or
                                           path.name in {'CMakeLists.txt'}))
    if not files or not (runtime / 'dc_runtime.cpp').is_file():
        raise FileNotFoundError('Runtime source incomplete')
    for path in files:
        relative = path.relative_to(runtime).as_posix()
        digest.update(relative.encode('utf-8'))
        digest.update(b'\0')
        digest.update(file_sha256(path).encode('ascii'))
        digest.update(b'\n')
    return digest.hexdigest()


def inventory(runtime: Path, build: Path) -> dict:
    missing = [name for name in REQUIRED_DLLS if not (build / name).is_file()]
    if missing:
        raise FileNotFoundError('Missing runtime binaries: ' + ', '.join(missing))
    return {'format': 1, 'source_sha256': source_digest(runtime),
            'binary_sha256': {name: file_sha256(build / name) for name in REQUIRED_DLLS}}


def run(mode: str, runtime: Path, build: Path, manifest: Path) -> int:
    if mode not in {'check', 'write'}:
        raise ValueError('Mode must be check or write')
    try:
        actual = inventory(runtime, build)
    except (FileNotFoundError, OSError) as exc:
        print('[RUNTIME VERIFY] ' + str(exc))
        return 1
    if mode == 'write':
        manifest.parent.mkdir(parents=True, exist_ok=True)
        temporary = manifest.with_name(manifest.name + '.tmp')
        temporary.write_text(json.dumps(actual, sort_keys=True, indent=2) + '\n', encoding='utf-8')
        temporary.replace(manifest)
        print('[RUNTIME VERIFY] Manifest written, source=' + actual['source_sha256'][:16])
        return 0
    try:
        prior = json.loads(manifest.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError):
        print('[RUNTIME VERIFY] No valid source/binary manifest; rebuilding runtime only.')
        return 1
    if prior != actual:
        print('[RUNTIME VERIFY] Runtime source or DLL checksum mismatch; rebuilding runtime only.')
        return 1
    print('[RUNTIME VERIFY] Runtime source and all four DLLs match manifest.')
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 5:
        print('usage: verify_runtime_artifacts.py check|write SOURCE_DIR BUILD_DIR MANIFEST')
        sys.exit(2)
    sys.exit(run(sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])))
