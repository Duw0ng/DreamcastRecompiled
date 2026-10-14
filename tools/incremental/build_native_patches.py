#!/usr/bin/env python3
"""Build only newly learned SH-4 seeds as ABI1 DLLs. Never touches the base guest.

The interpreter remains the fallback for seeds rejected by analysis/code generation.
Patches are keyed by the exact staged boot image hash and an emitter revision.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

SEED_RE = re.compile(r'^\s*(0x[0-9a-fA-F]{8})(?:\s|$)')
PATCH_SCHEMA = 'native-seed-patch-v1'


def hash_file(path: Path) -> str:
    digest=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024 * 1024),b''):
            digest.update(chunk)
    return digest.hexdigest()


def known_base(map_path: Path) -> set[int]:
    if not map_path.is_file(): return set()
    with map_path.open(encoding='utf-8-sig',newline='') as f:
        return {int(row['address'],16) for row in csv.DictReader(f) if row.get('address','').startswith('0x')}


def read_seeds(path: Path, base: int, image_size: int) -> list[int]:
    if not path.exists(): return []
    results=set()
    for line in path.read_text(encoding='utf-8-sig',errors='replace').splitlines():
        # runtime-code entries are comments and MUST NOT be compiled from disk.
        m=SEED_RE.match(line)
        if m:
            address=int(m.group(1),16)
            if not address & 1 and base <= address and address-base+32 <= image_size:
                results.add(address)
    return sorted(results)


def execute(command: list[str], cwd: Path) -> None:
    print('[NATIVE SEEDS]',subprocess.list2cmdline(command),flush=True)
    subprocess.run(command,cwd=cwd,check=True)


def cmake_source(patch_id: str, runtime_lib: Path, outdir: Path) -> str:
    return f'''cmake_minimum_required(VERSION 3.20)
project(dcr_sh4_patch LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(dcr_sh4_patch_{patch_id} SHARED
    generated_program.cpp
    incremental_exports.cpp
)
target_include_directories(dcr_sh4_patch_{patch_id} PRIVATE "${{CMAKE_CURRENT_SOURCE_DIR}}")
target_link_libraries(dcr_sh4_patch_{patch_id} PRIVATE "{runtime_lib.as_posix()}")
target_compile_definitions(dcr_sh4_patch_{patch_id} PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)
set_target_properties(dcr_sh4_patch_{patch_id} PROPERTIES
    PREFIX ""
    INTERPROCEDURAL_OPTIMIZATION FALSE
    RUNTIME_OUTPUT_DIRECTORY "{outdir.as_posix()}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "{outdir.as_posix()}"
    LIBRARY_OUTPUT_DIRECTORY "{outdir.as_posix()}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "{outdir.as_posix()}"
)
if(MSVC)
    target_compile_options(dcr_sh4_patch_{patch_id} PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)
else()
    target_compile_options(dcr_sh4_patch_{patch_id} PRIVATE $<$<CONFIG:Release>:-O2> -fPIC)
endif()
'''


def exports_source(hash_value: str) -> str:
    return f'''#include "generated_program.hpp"
#include <cstdint>
#ifdef _WIN32
#define DCR_PATCH_EXPORT extern "C" __declspec(dllexport)
#else
#define DCR_PATCH_EXPORT extern "C" __attribute__((visibility("default")))
#endif
DCR_PATCH_EXPORT std::uint32_t dcr_sh4_patch_abi_v1() {{ return 0x00010000u; }}
DCR_PATCH_EXPORT const char* dcr_sh4_patch_boot_sha256_v1() {{ return "{hash_value}"; }}
DCR_PATCH_EXPORT bool dcr_sh4_patch_register_v1(dcrecomp_generated::DCRuntime* runtime) {{
    if (!runtime) return false;
    dcrecomp_generated::register_recompiled_program(*runtime);
    return true;
}}
'''


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--root',required=True,type=Path)
    ap.add_argument('--game-dir',required=True,type=Path)
    ap.add_argument('--image',required=True,type=Path)
    ap.add_argument('--base',required=True,type=lambda s:int(s,0))
    ap.add_argument('--max-per-batch',type=int,default=8)
    ap.add_argument('--dry-run',action='store_true')
    ns=ap.parse_args()
    root=ns.root.resolve()
    game=ns.game_dir.resolve()
    image=ns.image.resolve()
    if not image.is_file(): raise RuntimeError(f'boot image missing: {image}')
    guest_build=game/'build'
    image_hash=hash_file(image)
    (game/'incremental').mkdir(parents=True,exist_ok=True)
    # Runtime validates this before loading any patch. A patch never survives a
    # changed boot image, including a different image with the same game slug.
    (game/'incremental'/'current_boot_sha256.txt').write_text(image_hash+'\n',encoding='ascii')
    all_seeds=read_seeds(game/'interp_seeds.txt',ns.base,image.stat().st_size)
    base_seeds=known_base(game/'generated'/'function_map.csv')
    wanted=[s for s in all_seeds if s not in base_seeds]
    patch_root=game/'incremental'
    native=patch_root/'native'
    native.mkdir(exist_ok=True)
    existing=set()
    for manifest in native.glob('dcr_sh4_patch_*.json'):
        try:
            entry=json.loads(manifest.read_text(encoding='utf-8'))
            if entry['schema']==PATCH_SCHEMA and entry['boot_sha256']==image_hash and \
                (native / (manifest.stem + ('.dll' if os.name=='nt' else '.so'))).exists():
                existing.update(int(s,16) for s in entry['seeds'])
        except (OSError,KeyError,ValueError,TypeError): pass
    new=[s for s in wanted if s not in existing]
    print(f'[NATIVE SEEDS] recorded={len(all_seeds)} base={len(base_seeds)} native-cache={len(existing)} pending={len(new)}')
    if not new:return 0
    if ns.dry_run:
        print('[NATIVE SEEDS] would compile',*[f'0x{s:08X}' for s in new]);return 0
    compiler=root/'build'/'Release'/'dc_incremental_recomp.exe'
    if not compiler.exists():
        execute(['cmake','-S',str(root),'-B',str(root/'build'),'-A','x64'],root)
    execute(['cmake','--build',str(root/'build'),'--config','Release','--target','dc_incremental_recomp','--parallel','1'],root)
    if not compiler.is_file():raise RuntimeError('missing dc_incremental_recomp.exe')
    importlib=guest_build/'Release'/'DreamcastRuntime.lib'
    if not importlib.exists():raise RuntimeError('runtime import lib absent; base cache must be built first')
    max_batch=max(1,min(ns.max_per_batch,16))
    pending=[new[i:i+max_batch] for i in range(0,len(new),max_batch)]
    while pending:
        batch=pending.pop(0)
        fingerprint=hashlib.sha256((PATCH_SCHEMA+'|'+image_hash+'|'+','.join(map(str,batch))+'|'+hash_file(root/'src'/'codegen'/'cpp_emitter.cpp')+'|'+hash_file(root/'src'/'recompiler'/'incremental.cpp')+'|'+hash_file(root/'runtime'/'guest_abi'/'v1'/'dc_runtime.hpp')).encode()).hexdigest()[:16]
        failed=patch_root/'failed'/f'{fingerprint}.json'
        if len(batch)==1 and failed.exists():
            print(f'[NATIVE SEEDS] quarantined 0x{batch[0]:08X}, interpreter fallback retained')
            continue
        basename=f'dcr_sh4_patch_{fingerprint}'
        src=patch_root/'staging'/fingerprint/'src'
        build=patch_root/'staging'/fingerprint/'build'
        out=patch_root/'staging'/fingerprint/'bin'
        src.mkdir(parents=True,exist_ok=True)
        out.mkdir(parents=True,exist_ok=True)
        arguments=[str(compiler),f'--image={image}',f'--base=0x{ns.base:08X}',f'--output={src}']+[f'--seed=0x{s:08X}' for s in batch]
        try:
            execute(arguments,root)
            for name in ('dc_runtime.hpp','dc_arm7.hpp','dc_native_overrides.hpp'):
                shutil.copy2(root/'runtime'/'guest_abi'/'v1'/name,src/name)
            (src/'incremental_exports.cpp').write_text(exports_source(image_hash),encoding='utf-8')
            (src/'CMakeLists.txt').write_text(cmake_source(fingerprint,importlib,out),encoding='utf-8')
            execute(['cmake','-S',str(src),'-B',str(build),'-A','x64'],root)
            execute(['cmake','--build',str(build),'--config','Release','--parallel','1'],root)
            built=out/(basename+'.dll')
            if not built.exists():raise RuntimeError('compiled patch DLL missing')
            published=native/built.name
            temporary=native/(built.name+'.new')
            shutil.copy2(built,temporary)
            os.replace(temporary,published)
            manifest={'schema':PATCH_SCHEMA,'boot_sha256':image_hash,'seeds':[f'0x{s:08X}' for s in batch],
                      'module_sha256':hash_file(published),'tool':'dc_incremental_recomp'}
            (native/(basename+'.json')).write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
            print(f'[NATIVE SEEDS] published {published.name}: {len(batch)} seeds')
        except (subprocess.CalledProcessError,RuntimeError) as e:
            # A bad seed must not poison the whole batch or force perpetual
            # recompilation on every subsequent launch.
            print(f'[NATIVE SEEDS] WARN patch build failed ({e}); keeping interpreter fallback',file=sys.stderr)
            if len(batch)>1:
                mid=len(batch)//2
                pending[:0]=[batch[:mid],batch[mid:]]
                print(f'[NATIVE SEEDS] retrying smaller batches ({mid}+{len(batch)-mid})')
            else:
                failed.parent.mkdir(parents=True,exist_ok=True)
                failed.write_text(json.dumps({'schema':PATCH_SCHEMA,'seed':f'0x{batch[0]:08X}',
                    'boot_sha256':image_hash,'reason':str(e),'retry':'after emitter/ABI change'},indent=2)+'\n')
                print(f'[NATIVE SEEDS] quarantined 0x{batch[0]:08X}; will stay interpreted')
    return 0

if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as e:
        print(f'[NATIVE SEEDS] WARN: {e}; base guest unchanged',file=sys.stderr)
        raise SystemExit(1)
