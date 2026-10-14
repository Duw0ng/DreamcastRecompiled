#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys
r=Path(__file__).resolve().parents[3]
src=(r/'portable/launcher/main_windows.go').read_text(encoding='utf-8')
maple=(r/'runtime/plugins/dcr_maple.cpp').read_text(encoding='utf-8')
cmake=(r/'runtime/CMakeLists.txt').read_text(encoding='utf-8')
bat=(r/'run_game.bat').read_text(encoding='utf-8-sig')
checks={
 'version':'cpp36b1g SONY_HAPTICS_VISUAL_V2' in bat,
 'runtime_stamp':'DCR_RUNTIME_BUILD_ID=cpp36b1g-sony-haptics-visual-v2' in bat,
 'launcher_v07':'const launcherVersion = "0.7"' in src,
 'double_buffer':'CreateCompatibleDC' in src and 'BitBlt' in src and 'SRCCOPY' in src,
 'state_change_repaint':'st != ctlVisualState' in src,
 'dreamcast_shape':'drawDreamcastBody' in src and 'PolyBezierTo' in src,
 'authentic_colors':'face("A"' in src and 'rgb(218, 72, 70)' in src and 'rgb(55, 133, 214)' in src and 'rgb(241, 194, 37)' in src and 'rgb(91, 191, 139)' in src,
 'launcher_plugin_rumble':'dcr_maple_test_host_rumble_v1' in src,
 'launcher_haptics_probe':'dcr_maple_probe_host_haptics_v1' in src,
 'dualsense_pid':'0x0CE6u' in maple and '0x0DF2u' in maple,
 'ds4_pid':'0x05C4u' in maple and '0x09CCu' in maple,
 'dualsense_usb':'report[0] = 0x02u' in maple,
 'dualsense_bt':'report[0] = 0x31u' in maple and 'report[2] = 0x10u' in maple,
 'ds4_usb':'report[0] = 0x05u' in maple,
 'ds4_bt':'report[0] = 0x11u' in maple,
 'sony_crc':'0xA2u' in maple and 'crc32_ieee' in maple,
 'windows_hid_links':'setupapi hid' in cmake,
 'purupuru_uses_sony':'sony_hid_rumble' in maple and 'maple_host_rumble' in maple,
}
safe=r/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
checks['safe_hash']=safe.is_file() and hashlib.sha256(safe.read_bytes()).hexdigest()=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4'
exe=r/'portable/DreamcastRecompiled.exe'
checks['launcher_built']=exe.is_file() and exe.stat().st_size>100000
ok=True
for k,v in checks.items():
 print(('PASS' if v else 'FAIL'),k); ok &= v
sys.exit(0 if ok else 1)
