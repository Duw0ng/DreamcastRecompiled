#!/usr/bin/env python3
from pathlib import Path
import hashlib,sys
r=Path(__file__).resolve().parents[3]
h=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
checks=[]
checks.append((h(r/'runtime/include/dc_runtime.hpp')=='ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739','DCRuntime ABI header identical to cpp34g'))
checks.append((h(r/'src/codegen/cpp_emitter.cpp')=='e56050f0864d1c9272334d6e8a06214011383cbc641345f743286b7889444bf4','SH-4 emitter identical to cpp34g'))
p=(r/'runtime/plugins/dcr_network.cpp').read_text(errors='replace')
checks.append(('dormant until modem MMIO' in p and 'activate_modem' in p,'network plugin is lazy until modem MMIO'))
b=(r/'run_game.bat').read_text(errors='replace')
checks.append(('CACHE RECOVERY' in b and 'cpp34h BOOTROM_RO_FLYCAST_MODEM_TRACE_ABI1' in b,'cpp34h cache contamination recovery enabled'))
d=(r/'runtime/dc_runtime.cpp').read_text(errors='replace')
checks.append(('boot_rom_readonly_range' in d and 'v1.2-cpp34j-stable-boot-lazy-fullstack' in d,'ABI-safe Boot ROM write-ignore present'))
for ok,msg in checks: print(('[PASS] ' if ok else '[FAIL] ')+msg)
sys.exit(0 if all(x for x,_ in checks) else 1)
