#!/usr/bin/env python3
from pathlib import Path
import hashlib, re, sys
root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, ok):
    checks.append((name,bool(ok))); print(('PASS' if ok else 'FAIL'), name)
run=(root/'run_game.bat').read_text(encoding='utf-8-sig',errors='replace')
api=(root/'runtime/include/dcr_plugin_api_v2.hpp').read_text(errors='replace')
bridge=(root/'runtime/include/dcr_gdrom_bridge.hpp').read_text(errors='replace')
host=(root/'runtime/dcr_plugin_host.cpp').read_text(errors='replace')
rt=(root/'runtime/dc_runtime.cpp').read_text(errors='replace')
plug=(root/'runtime/plugins/dcr_gdrom.cpp').read_text(errors='replace')
cm=(root/'runtime/CMakeLists.txt').read_text(errors='replace')
sync=(root/'tools/portable/sync_portable.py').read_text(errors='replace')
launch=(root/'portable/launcher/main_windows.go').read_text(errors='replace')
check('version cpp37b', 'cpp37b GDROM_COMMAND_DMA_NATIVE' in run)
check('runtime build id cpp37b', 'cpp37b-gdrom-command-dma-native' in run and 'v1.2-cpp37b-gdrom-command-dma-native' in rt)
check('GDROM ABI2 V2 interface', 'DCR_GDROM_CONTROL_IFACE_V2' in api and 'DCRGDROMControlV2' in api)
check('runtime-only bridge', 'dcr_gdrom_bridge.hpp' in rt and 'dc_plugin_gdrom_phase2_available' in bridge)
check('plugin command state', 'request_command_impl' in plug and 'DCRGDROMCommandStateV2 command_state' in plug)
check('plugin multi transfer', 'multi_transfer_impl' in plug and 'write_guest_bulk' in plug)
check('plugin DMA IRQ', 'kAsicEventGdDmaBit' in plug and 'raise_irq' in plug and 'dma_end_ack_impl' in plug)
check('runtime phase2 request route', 'dc_plugin_gdrom_request_command' in rt and '[plugin-state]' in rt)
check('runtime phase2 multi route', 'dc_plugin_gdrom_begin_multi' in rt and 'dc_plugin_gdrom_multi_transfer' in rt)
check('heartbeat phase2 counters', 'gd-native=' in rt and 'gd-plugin=' in rt and 'gd-xfer=' in rt)
check('dcr_gdrom CMake target', 'add_library(dcr_gdrom SHARED' in cm)
check('run_game builds gdrom', 'DreamcastRuntime dcr_network dcr_maple dcr_gdrom' in run)
check('portable copies gdrom', '"dcr_gdrom.dll"' in sync)
check('short normal title env', 'DCR_WINDOW_TITLE=DreamcastRecompiled - !DCR_GAME_TITLE! [cpp37b]' in run)
check('runtime title override', 'dcr_clean_window_title' in rt and 'DCR_BUILD_SHORT' in rt)
check('launcher v0.9', 'const launcherVersion = "0.9"' in launch)
check('launcher generic short title', 'DreamcastRecompiled Portable [cpp37b]' in launch)
check('launcher dynamic disc title', 'discMapTitle' in launch and 'DCR_WINDOW_TITLE=DreamcastRecompiled - "+gameTitle+" [cpp37b]' in launch)
# DCRuntime body fingerprint must remain exactly frozen.
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
launcher=root/'portable/DreamcastRecompiled.exe'
check('Windows GUI launcher built', launcher.is_file() and launcher.read_bytes()[:2]==b'MZ')
sys.exit(0 if all(v for _,v in checks) else 1)
