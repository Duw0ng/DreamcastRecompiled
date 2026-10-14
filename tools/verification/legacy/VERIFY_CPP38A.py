from pathlib import Path
import hashlib, subprocess, sys
root=Path(__file__).resolve().parents[3]
ok=True
def check(name, cond):
    global ok
    print(('PASS' if cond else 'FAIL'), name)
    ok &= bool(cond)

def fp(path):
    return subprocess.check_output([sys.executable, str(root/'tools/modular/guest_abi_fingerprint.py'), str(path)], text=True).strip().splitlines()[-1].split('=')[-1].strip() if False else None

# Compare through the official two-file checker and inspect its exit code.
r=subprocess.run([sys.executable, str(root/'tools/modular/guest_abi_fingerprint.py'),
                  str(root/'runtime/include/dc_runtime.hpp'),
                  str(root/'runtime/guest_abi/v1/dc_runtime.hpp')], capture_output=True, text=True)
check('Guest ABI1 fingerprint frozen', r.returncode==0 and '00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f' in r.stdout)

safe=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
h=hashlib.sha256(safe.read_bytes()).hexdigest() if safe.exists() else ''
check('SAFE ChuChu guest unchanged', h=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4')

src=(root/'runtime/dc_runtime.cpp').read_text(errors='ignore')
check('one-copy TA capture present', 'exactly one 32-byte copy' in src)
check('CH2 TA bulk main-RAM path present', 'pvr_submit_packet_block_from_main_ram' in src)
check('CH2 texture memcpy path present', 'CH2->VRAM bulk texture fast-path active' in src)
check('SIMD tag scanner present', 'pvr_find_complete_vertex32_strip' in src and '_mm_movemask_epi8' in src)
check('cpp38a build id present', 'v1.2-cpp38a-pvr-ta-native-fastpath' in src)
run=(root/'run_game.bat').read_text(errors='ignore')
check('short cpp38a game title present', 'DreamcastRecompiled - !DCR_GAME_TITLE! [cpp38a]' in run)
launcher=(root/'portable/launcher/main_windows.go').read_text(errors='ignore')
check('portable launcher cpp38a', '[cpp38a]' in launcher and 'Launcher v1.1' in launcher)
print('RESULT', 'PASS' if ok else 'FAIL')
sys.exit(0 if ok else 1)
