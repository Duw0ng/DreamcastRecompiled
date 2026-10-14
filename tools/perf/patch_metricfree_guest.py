#!/usr/bin/env python3
from __future__ import annotations
import argparse, re, sys
from pathlib import Path

REV = "cpp38b2-metricfree-v1"

OLD_MACRO = '''#if defined(DCR_FIXED_FPU_MODE)\n#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) (DCR_FIXED_FPU_MODE == 2)\n#if defined(DCR_FIXED_FPU_REGION_METRICS)\n#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((DCR_FIXED_FPU_MODE != 3) || (DCR_FIXED_FPU_REGION_METRICS != 0))\n#else\n#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((DCR_FIXED_FPU_MODE != 3) || (runtime).fpu_region_metrics)\n#endif\n#else\n#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) ((runtime).fpu_aot_mode == 2u)\n#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((runtime).fpu_aot_mode != 3u || (runtime).fpu_region_metrics)\n#endif\n'''
NEW_MACRO = '''#if defined(DCR_FIXED_FPU_MODE)\n#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) (DCR_FIXED_FPU_MODE == 2)\n#else\n#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) ((runtime).fpu_aot_mode == 2u)\n#endif\n\n// cpp38b2: FPU telemetry is independent from the selected FPU execution mode.\n#if defined(DCR_FIXED_FPU_REGION_METRICS)\n#define DCR_FPU_REGION_METRICS_ENABLED(runtime) (DCR_FIXED_FPU_REGION_METRICS != 0)\n#else\n#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((runtime).fpu_region_metrics)\n#endif\n\n#if defined(DCR_FIXED_FPU_REGION_METRICS) && (DCR_FIXED_FPU_REGION_METRICS == 0)\n#define DCR_FPU_METRIC_INC(value) ((void)0)\n#define DCR_FPU_METRIC_ADD(value, amount) ((void)0)\n#else\n#define DCR_FPU_METRIC_INC(value) do { if (DCR_FPU_REGION_METRICS_ENABLED(runtime)) ++(value); } while (0)\n#define DCR_FPU_METRIC_ADD(value, amount) do { if (DCR_FPU_REGION_METRICS_ENABLED(runtime)) (value) += (amount); } while (0)\n#endif\n'''

FIELDS = [
    'fpu_ftrv_ops','fpu_ftrv_fr1_ops','fpu_fipr_ops','fpu_fmac_ops','fpu_fsca_ops','fpu_fsrra_ops','fpu_frchg_ops',
    'fpu_block_cache_hits','fpu_block_cache_fallbacks','fpu_block_cache_ops','fpu_block_cache_reuses','fpu_block_cache_writebacks',
    'fpu_trace_guard_entries','fpu_trace_guard_reuses','fpu_trace_lazy_loads','fpu_trace_actual_writebacks',
    'fpu_trace_tick_flushes','fpu_trace_barrier_flushes','fpu_trace_exit_flushes','fpu_trace_cross_block_keeps',
    'fpu_super_entries','fpu_super_reuses','fpu_super_static_loads','fpu_super_actual_writebacks',
    'fpu_super_tick_flushes','fpu_super_barrier_flushes','fpu_super_exit_flushes','fpu_super_cross_block_keeps',
    'fpu_ssa_regions','fpu_ssa_scalar_loads','fpu_ssa_scalar_writebacks','fpu_ssa_matrix_reads','fpu_ssa_fmac_ops',
    'fpu_fmov64_loads','fpu_fmov64_stores','fpu_fmov64_sq_stores','fpu_fmov64_reg_moves',
]
LOCAL_FIELDS = [
    'sfc_stat_regions','sfc_stat_ops','sfc_stat_reuses','sfc_stat_planned_wb','sfc_stat_ftrv','sfc_stat_fipr','sfc_stat_fmac',
    'sfc_stat_fsca','sfc_stat_fsrra','sfc_stat_cross','sfc_stat_super_reuses','sfc_stat_entries','sfc_stat_static_loads',
]

def patch_header(path: Path) -> bool:
    s = path.read_text(encoding='utf-8', errors='replace')
    before = s
    if OLD_MACRO in s:
        s = s.replace(OLD_MACRO, NEW_MACRO, 1)
    elif 'DCR_FPU_METRIC_INC' not in s:
        raise RuntimeError(f'header macro layout not recognized: {path}')
    if s != before:
        path.write_text(s, encoding='utf-8')
        return True
    return False

def metricize_text(s: str) -> tuple[str, int]:
    n = 0
    old = s
    s, c = re.subn(r'if \(runtime\.fpu_aot_mode != 3u \|\| runtime\.fpu_region_metrics\)',
                   'if (DCR_FPU_REGION_METRICS_ENABLED(runtime))', s); n += c
    for f in FIELDS:
        s, c = re.subn(r'\+\+runtime\.'+re.escape(f)+r';', f'DCR_FPU_METRIC_INC(runtime.{f});', s); n += c
        s, c = re.subn(r'runtime\.'+re.escape(f)+r'\s*\+=\s*([^;\n]+);',
                       lambda m, f=f: f'DCR_FPU_METRIC_ADD(runtime.{f}, {m.group(1).strip()});', s); n += c
    for f in LOCAL_FIELDS:
        s, c = re.subn(r'\+\+'+re.escape(f)+r';', f'DCR_FPU_METRIC_INC({f});', s); n += c
        s, c = re.subn(r'\b'+re.escape(f)+r'\s*\+=\s*([^;\n]+);',
                       lambda m, f=f: f'DCR_FPU_METRIC_ADD({f}, {m.group(1).strip()});', s); n += c
    return s, n

def patch_cmake(path: Path) -> bool:
    s = path.read_text(encoding='utf-8', errors='replace')
    if 'DCR_FIXED_FPU_REGION_METRICS=0' in s:
        return False
    needle = 'target_compile_definitions(dcr_game_module PRIVATE DCR_GAME_MODULE_BUILD=1 DCR_GUEST_ABI_VERSION=1)'
    if needle not in s:
        raise RuntimeError('dcr_game_module target not found in generated CMakeLists.txt')
    s = s.replace(needle, needle+'\ntarget_compile_definitions(dcr_game_module PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)', 1)
    path.write_text(s, encoding='utf-8')
    return True

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('cpp_dir', type=Path)
    ns=ap.parse_args()
    cpp=ns.cpp_dir.resolve()
    if not cpp.is_dir():
        print(f'[cpp38b2] ERROR generated cpp dir missing: {cpp}', file=sys.stderr); return 2
    header=cpp/'dc_runtime.hpp'; cmake=cpp/'CMakeLists.txt'
    if not header.is_file() or not cmake.is_file():
        print('[cpp38b2] ERROR cache lacks generated sources/CMake. Use --recompile-game once.', file=sys.stderr); return 3
    changed=0
    try:
        changed += int(patch_header(header))
        changed += int(patch_cmake(cmake))
        source_files=[p for p in cpp.rglob('*.cpp') if p.is_file()]
        transformed=0
        for p in source_files:
            s=p.read_text(encoding='utf-8', errors='replace')
            ns_, count=metricize_text(s)
            if ns_ != s:
                p.write_text(ns_, encoding='utf-8')
                changed += 1
                transformed += count
        (cpp/'.dcr_guest_impl_source').write_text(f'DCR_GUEST_IMPL={REV}\n', encoding='ascii')
    except Exception as e:
        print(f'[cpp38b2] ERROR: {e}', file=sys.stderr); return 4
    print(f'[cpp38b2] metric-free source migration ready | files={changed} rewrites={transformed} rev={REV}')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
