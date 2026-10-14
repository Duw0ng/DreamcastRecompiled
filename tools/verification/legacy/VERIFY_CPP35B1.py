from pathlib import Path
import hashlib, re, subprocess, sys
root=Path(__file__).resolve().parents[3]
run=(root/'run_game.bat').read_text(encoding='utf-8')
mod=(root/'tools/modular/modularize_generated.py').read_text(encoding='utf-8')
frozen=root/'runtime/guest_abi/v1/dc_runtime.hpp'
mutable=root/'runtime/include/dc_runtime.hpp'

def fp(path):
    out=subprocess.check_output([sys.executable, str(root/'tools/modular/guest_abi_fingerprint.py'), str(path)], text=True)
    return out.strip().splitlines()[-1]
checks={
 'version cpp35b1':'cpp35b1 CACHE_ABI_FREEZE' in run,
 'no whole-header fc':'fc /b "!CPP!\\dc_runtime.hpp"' not in run,
 'struct fingerprint validation':'guest_abi_fingerprint.py' in run,
 'valid cache skips modularizer':'goto :cached_guest_ready' in run,
 'cached guest immutable marker':'guest source/build metadata is read-only' in run,
 'runtime has separate build tree':'runtime_build_cpp35b1' in run,
 'runtime-only targets omit guest':'set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple"' in run,
 'rebuild runtime no dreamcast_program':'REBUILD_RUNTIME%"=="1" set "BUILD_TARGET=DreamcastRuntime dcr_network dcr_maple dreamcast_program' not in run,
 'frozen guest ABI exists':frozen.exists(),
 'new guest uses frozen ABI':'runtime / "guest_abi" / "v1"' in mod,
 'modularizer not mutable runtime header':'src = runtime / "include" / name' not in mod,
 'current layout matches frozen':fp(mutable)==fp(frozen),
}
for n,ok in checks.items(): print(('PASS' if ok else 'FAIL'),n)
if not all(checks.values()): raise SystemExit(1)
print('cpp35b1 cache/ABI freeze invariants: PASS')
