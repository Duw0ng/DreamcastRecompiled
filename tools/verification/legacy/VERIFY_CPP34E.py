from pathlib import Path
import sys
r=Path(__file__).resolve().parents[3]
checks={
"runtime/dc_runtime.cpp":["v1.2-cpp34e-bootsafe-ipbin-reentry","bool armed{}","runtime.pvr_page_flips > 0u","reentry guard armed","[DCR SH4 GUARD]"],
"runtime/plugins/dcr_network.cpp":["cpp34e-network-abi1"],
"run_game.bat":["cpp34e BOOTSAFE_IPBIN_REENTRY_ABI1","DreamcastRuntime dcr_network dreamcast_program"],
"VERSION":["v1.2-cpp34e-bootsafe-ipbin-reentry-abi1"]}
bad=False
for rel,ns in checks.items():
 t=(r/rel).read_text(encoding="utf-8",errors="replace")
 for n in ns:
  ok=n in t; print(("[OK] " if ok else "[FAIL] ")+rel+": "+n); bad|=not ok
sys.exit(1 if bad else 0)
