#!/usr/bin/env python3
from pathlib import Path
import hashlib, re, sys
root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, ok):
    checks.append((name,bool(ok))); print(('PASS' if ok else 'FAIL'), name)
run=(root/'run_game.bat').read_text(encoding='utf-8-sig',errors='replace')
api=(root/'runtime/include/dcr_plugin_api_v2.hpp').read_text(errors='replace')
host=(root/'runtime/dcr_plugin_host.cpp').read_text(errors='replace')
rt=(root/'runtime/dc_runtime.cpp').read_text(errors='replace')
cm=(root/'runtime/CMakeLists.txt').read_text(errors='replace')
sync=(root/'tools/portable/sync_portable.py').read_text(errors='replace')
launch=(root/'portable/launcher/main_windows.go').read_text(errors='replace')
check('version cpp37a', 'cpp37a GDROM_NATIVE_PLUGIN_PHASE1' in run)
check('GDROM ABI2 interface', 'DCR_GDROM_CONTROL_IFACE_V1' in api and 'DCRGDROMControlV1' in api)
check('dcr_gdrom CMake target', 'add_library(dcr_gdrom SHARED' in cm)
check('runtime plugin map route', 'dc_plugin_gdrom_load_disc_map(runtime, path)' in rt)
check('runtime bulk read route', 'dc_plugin_gdrom_read_bytes' in rt)
check('runtime TOC route', 'dc_plugin_gdrom_build_toc' in rt)
check('plugin host snapshot bridge', 'cpp37a media snapshot active' in host)
check('run_game builds gdrom', 'DreamcastRuntime dcr_network dcr_maple dcr_gdrom' in run)
check('run_game gdrom trace', '--gdrom-trace' in run and 'DCR_GDROM_TRACE' in run)
check('portable copies gdrom', '"dcr_gdrom.dll"' in sync)
check('launcher gdrom trace', 'GD-ROM Trace (verbose)' in launch and 'DCR_GDROM_TRACE=1' in launch)
# DCRuntime body fingerprint must match frozen ABI1 helper result.
def body(text):
    m=re.search(r'struct\s+DCRuntime\s*\{',text); assert m
    i=m.end(); depth=1; j=i
    while j<len(text) and depth:
        if text[j]=='{': depth+=1
        elif text[j]=='}': depth-=1
        j+=1
    x=text[m.start():j]
    x=re.sub(r'//.*?$|/\*.*?\*/','',x,flags=re.M|re.S)
    x=re.sub(r'\s+','',x)
    return hashlib.sha256(x.encode()).hexdigest()
h1=body((root/'runtime/include/dc_runtime.hpp').read_text(errors='replace'))
h2=body((root/'runtime/guest_abi/v1/dc_runtime.hpp').read_text(errors='replace'))
check('Guest ABI1 fingerprint unchanged', h1==h2=='00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f')
safe=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
check('SAFE guest unchanged', safe.is_file() and hashlib.sha256(safe.read_bytes()).hexdigest()=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4')
sys.exit(0 if all(v for _,v in checks) else 1)
