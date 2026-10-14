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
check('version cpp37c', 'cpp37c GDROM_CDDA_NATIVE' in run)
check('runtime build id cpp37c', 'cpp37c-gdrom-cdda-native' in run and 'v1.2-cpp37c-gdrom-cdda-native' in rt)
check('GDROM ABI2 V3 interface', 'DCR_GDROM_CONTROL_IFACE_V3' in api and 'DCRGDROMControlV3' in api)
check('phase3 bridge', 'dc_plugin_gdrom_phase3_available' in bridge and 'gdrom_control_v3_for' in host)
check('CDDA plugin state', 'CDDAState cdda' in plug and 'play_sectors_impl' in plug and 'play_tracks_impl' in plug)
check('CDDA raw 2352 decoder', 'cdda_audio_cache' in plug and '2352' in plug and 'render_cdda_frames_impl' in plug)
check('CDDA YellowBook compatibility', 'gd_cdda_yellowbook_descramble' in plug and 'gd_cdda_repair_sync_prefix' in plug)
check('CDDA transport controls', 'pause_cdda_impl' in plug and 'release_cdda_impl' in plug and 'stop_cdda_impl' in plug)
check('CDDA DLL batching', 'std::array<std::int16_t, 64u * 2u>' in host and 'render_frames' in host)
check('runtime phase3 command route', 'dc_plugin_gdrom_cdda_play_sectors' in rt and 'plugin_native_command' in rt)
check('runtime phase3 mixer route', 'dc_plugin_gdrom_cdda_mix_frame' in rt)
check('runtime phase3 scheduler route', 'dc_plugin_gdrom_cdda_advance' in rt)
check('runtime raw image stream closed in phase3', '!gdrom_control_v3_for(p)' in host)
check('heartbeat phase3 counters', '"phase3"' in rt and 'gd-cdda=' in rt)
check('dcr_gdrom CMake target', 'add_library(dcr_gdrom SHARED' in cm)
check('run_game builds gdrom', 'DreamcastRuntime dcr_network dcr_maple dcr_gdrom' in run)
check('portable copies gdrom', '"dcr_gdrom.dll"' in sync)
check('short normal title cpp37c', 'DCR_WINDOW_TITLE=DreamcastRecompiled - !DCR_GAME_TITLE! [cpp37c]' in run)
check('launcher v1.0', 'const launcherVersion = "1.0"' in launch)
check('launcher title cpp37c', 'DreamcastRecompiled Portable [cpp37c]' in launch and 'DCR_WINDOW_TITLE=DreamcastRecompiled - "+gameTitle+" [cpp37c]' in launch)
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
