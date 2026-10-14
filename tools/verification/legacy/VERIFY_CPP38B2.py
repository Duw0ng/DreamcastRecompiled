#!/usr/bin/env python3
from __future__ import annotations
import hashlib, subprocess, sys
from pathlib import Path

root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('PASS' if cond else 'FAIL')+': '+name)

def text(p): return p.read_text(encoding='utf-8-sig', errors='replace')
rt=text(root/'runtime/dc_runtime.cpp')
h=text(root/'runtime/include/dc_runtime.hpp')
gh=text(root/'runtime/guest_abi/v1/dc_runtime.hpp')
em=text(root/'src/codegen/cpp_emitter.cpp')
mod=text(root/'tools/modular/modularize_generated.py')
run=text(root/'run_game.bat')
go=text(root/'portable/launcher/main_windows.go')
patch=text(root/'tools/perf/patch_metricfree_guest.py')

check('runtime build id', 'v1.2-cpp38b2-fpu-metricfree-recovery' in rt)
check('heartbeat uses real metric switch', 'fpu-metrics=" << (DCR_FPU_REGION_METRICS_ENABLED(runtime) ? "on" : "off")' in rt)
check('metric gate independent of FPU mode', '#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((runtime).fpu_region_metrics)' in h)
check('fixed metric-off macros compile to no-op', '#define DCR_FPU_METRIC_INC(value) ((void)0)' in h and '#define DCR_FPU_METRIC_ADD(value, amount) ((void)0)' in h)
check('frozen Guest ABI header has same metric macro behavior', 'cpp38b2: FPU telemetry' in gh and 'DCR_FPU_METRIC_INC' in gh)
check('scalar FPU counters gated', 'DCR_FPU_METRIC_INC(runtime.fpu_fmac_ops)' in em and 'DCR_FPU_METRIC_INC(runtime.fpu_ftrv_ops)' in em)
check('Superblock counters gated', 'DCR_FPU_METRIC_INC(sfc_stat_regions)' in em and 'DCR_FPU_METRIC_ADD(runtime.fpu_super_actual_writebacks' in em)
check('legacy forced-on condition removed', 'runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics' not in em and 'runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics' not in rt)
check('modular guest compiles metrics out', 'DCR_FIXED_FPU_REGION_METRICS=0' in mod)
check('cache migration helper present', 'cpp38b2-metricfree-v1' in patch and 'metricize_text' in patch)
check('run_game one-time guest implementation revision', 'DCR_GUEST_IMPL_REV=cpp38b2-metricfree-v1' in run and 'patch_metricfree_guest.py' in run)
check('historical SAFE seed auto-adoption disabled', 'set "SAFE_SEED_AUTO=0"' in run)
check('cpp38b2 defaults preserved', 'FPU_MODE=superblock' in run and 'HOST_SYNC_QUANTUM=262144' in run and 'FPU_METRICS=0' in run)
check('PVR indexscratch retained', 'pvr38=on/cpp38b2/indexscratch' in rt and 'mapindex' not in rt)
check('cpp38b2 test launcher exists', (root/'scripts/tests/TEST_CPP38B2_FAST.bat').is_file())
check('portable launcher cpp38b2 v1.4 source', 'const launcherVersion = "1.4"' in go and '[cpp38b2]' in go)
check('portable launcher PE exists', (root/'portable/DreamcastRecompiled.exe').is_file() and (root/'portable/DreamcastRecompiled.exe').read_bytes()[:2]==b'MZ')

proc=subprocess.run([sys.executable, str(root/'tools/modular/guest_abi_fingerprint.py'),
                     str(root/'runtime/include/dc_runtime.hpp'), str(root/'runtime/guest_abi/v1/dc_runtime.hpp')],
                    capture_output=True, text=True)
check('ABI1 structural fingerprint MATCH', proc.returncode==0 and 'MATCH' in proc.stdout and '00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f' in proc.stdout)
seed=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
if seed.is_file():
    digest=hashlib.sha256(seed.read_bytes()).hexdigest()
    check('historical SAFE seed retained unchanged', digest=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4')
else:
    check('historical SAFE seed retained unchanged', False)

bad=[n for n,ok in checks if not ok]
print(f'\n{len(checks)-len(bad)}/{len(checks)} checks passed')
if bad:
    print('FAILED: '+', '.join(bad))
    raise SystemExit(1)
