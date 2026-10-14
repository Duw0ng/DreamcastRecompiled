from pathlib import Path
import hashlib, sys
r=Path(__file__).resolve().parents[3]
checks={
 'patcher': (r/'tools/perf/patch_cpp38e_hot_block_recovery.py').exists(),
 'apply bat': (r/'scripts/stable/APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat').exists(),
 'fast bat': (r/'scripts/stable/TEST_CPP38E_FAST.bat').exists(),
 'diag bat': (r/'scripts/stable/TEST_CPP38E_HOTPC_DIAG.bat').exists(),
 'restore bat': (r/'scripts/maintenance/RESTORE_CPP38D_GUEST_BASELINE.bat').exists(),
 'runtime build id': 'v1.2-cpp38e-hot-block-recovery' in (r/'runtime/dc_runtime.cpp').read_text(encoding='utf-8-sig'),
 'runner build id': 'cpp38e-hot-block-recovery' in (r/'run_game.bat').read_text(encoding='utf-8-sig'),
 'hotrec default': 'set "HOTREC=1"' in (r/'run_game.bat').read_text(encoding='utf-8-sig'),
 'hint metric macro': 'DCR_HOT_METRIC_INC(runtime.dynamic_direct_hint_hits)' in (r/'src/codegen/cpp_emitter.cpp').read_text(encoding='utf-8-sig'),
 'safe guest present': (r/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll').exists(),
}
for k,v in checks.items(): print(('PASS' if v else 'FAIL'), k)
if not all(checks.values()): sys.exit(1)
p=r/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
print('SAFE_GUEST_SHA256', hashlib.sha256(p.read_bytes()).hexdigest())
