#!/usr/bin/env python3
from __future__ import annotations
import argparse
from pathlib import Path

BEGIN = "# BEGIN DCR_CPP38E_HOT_BLOCK_RECOVERY"
END = "# END DCR_CPP38E_HOT_BLOCK_RECOVERY"
DEFS = """# BEGIN DCR_CPP38E_HOT_BLOCK_RECOVERY
# cpp38e: ChuChu validated hot-path specialization. These switches remove
# diagnostic writes from the generated SH-4 fast path, constant-fold the
# proven 256-cycle tick quantum and inline the common MMU-off main-RAM word
# path. No guest clocks/timing rules are changed.
target_compile_definitions(dcr_game_module PRIVATE
    DCR_ASSUME_MMU_OFF=1
    DCR_FIXED_SH4_TICK_BATCH=256
    DCR_DISABLE_HOT_TICK_CALL_COUNTER=1
    DCR_LIGHTWEIGHT_HOT_METRICS=1
    DCR_LIGHTWEIGHT_CURRENT_PC=1
)
# END DCR_CPP38E_HOT_BLOCK_RECOVERY
"""

def read(p: Path) -> str:
    return p.read_text(encoding="utf-8-sig", errors="strict")

def write(p: Path, s: str) -> None:
    p.write_text(s.replace("\r\n", "\n"), encoding="utf-8")

def patch_cmake(p: Path) -> bool:
    s=read(p)
    if BEGIN in s:
        return False
    anchors=[
        'target_compile_definitions(dcr_game_module PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)\n',
        'target_compile_definitions(dcr_game_module PRIVATE DCR_GAME_MODULE_BUILD=1 DCR_GUEST_ABI_VERSION=1)\n',
    ]
    for a in anchors:
        if a in s:
            s=s.replace(a,a+DEFS,1)
            write(p,s)
            return True
    raise SystemExit('[cpp38e] No pude localizar target_compile_definitions(dcr_game_module ...) en CMakeLists.txt')

def patch_sources(cpp: Path) -> tuple[int,int]:
    files=0; replacements=0
    for p in sorted(cpp.glob('generated_program*.cpp')):
        s=read(p)
        old=s
        n=s.count('++runtime.dynamic_direct_hint_hits;')
        if n:
            s=s.replace('++runtime.dynamic_direct_hint_hits;', 'DCR_HOT_METRIC_INC(runtime.dynamic_direct_hint_hits);')
            write(p,s)
            files += 1; replacements += n
    return files,replacements

def main() -> None:
    ap=argparse.ArgumentParser()
    ap.add_argument('cpp_dir', type=Path)
    ns=ap.parse_args()
    cpp=ns.cpp_dir.resolve()
    cmake=cpp/'CMakeLists.txt'
    if not cmake.exists():
        raise SystemExit(f'[cpp38e] Falta {cmake}')
    cmake_changed=patch_cmake(cmake)
    files,repls=patch_sources(cpp)
    # Human-readable marker in generated source tree; does not affect ABI.
    (cpp/'.dcr_cpp38e_hot_block_recovery').write_text(
        'DCR_CPP38E_HOT_BLOCK_RECOVERY=1\n'
        'HOT_PC_1=0x8C0FE5A0\nHOT_PC_2=0x8C1088DA\n'
        'MMU_ASSUMPTION=OFF_VALIDATED\nTICK_BATCH=256\n', encoding='utf-8')
    print(f'[cpp38e] CMake specialization: {"applied" if cmake_changed else "already present"}')
    print(f'[cpp38e] dynamic-hint metric patch: files={files} replacements={repls}')
    print('[cpp38e] hot targets: 0x8C0FE5A0, 0x8C1088DA')

if __name__=='__main__':
    main()
