from pathlib import Path
import sys
r=Path(__file__).resolve().parents[3]
checks={
 "runtime/dc_runtime.cpp":["v1.2-cpp34f-alias-safe-ipbin-reentry","reentry suppressed","live-alias","ipbin_observe_game_execution","alias_suppressed"],
 "runtime/plugins/dcr_network.cpp":["cpp34f-network-abi1"],
 "run_game.bat":["cpp34f ALIAS_SAFE_IPBIN_REENTRY_ABI1"],
 "tools/modular/modularize_generated.py":["cpp34d IPBIN_REENTRY_SHADOW_GUARD_ABI1","cpp34e BOOTSAFE_IPBIN_REENTRY_ABI1","cpp34f ALIAS_SAFE_IPBIN_REENTRY_ABI1"],
 "VERSION":["v1.2-cpp34f-alias-safe-ipbin-reentry-abi1"],
}
bad=False
for rel,needles in checks.items():
 t=(r/rel).read_text(encoding="utf-8",errors="replace")
 for n in needles:
  ok=n in t; print(("[OK] " if ok else "[FAIL] ")+rel+" :: "+n); bad|=not ok
sys.exit(1 if bad else 0)
