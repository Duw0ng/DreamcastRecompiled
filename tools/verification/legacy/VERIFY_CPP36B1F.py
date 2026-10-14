#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys
r=Path(__file__).resolve().parents[3]
src=(r/'portable/launcher/main_windows.go').read_text(encoding='utf-8')
bat=(r/'run_game.bat').read_text(encoding='utf-8-sig')
checks={
 'version':'cpp36b1f DREAMCAST_CONTROLLER_VISUAL' in bat,
 'launcher_v06':'const launcherVersion = "0.6"' in src,
 'visual_class':'DCRDreamcastPadVisualV06' in src,
 'paint_handler':'controllerVisualWndProc' in src and 'WM_PAINT' in src,
 'mapped_state':'dreamcastVisualState' in src,
 'dpad':'out.Buttons |= 1 << 4' in src and 'out.Buttons |= 1 << 7' in src,
 'face_buttons':'face("A"' in src and 'face("B"' in src and 'face("X"' in src and 'face("Y"' in src,
 'start':'"START"' in src,
 'analog':'Stick %d,%d' in src,
 'triggers':'bar(110, st.LTrig, "L")' in src and 'bar(w-260, st.RTrig, "R")' in src,
 'per_port':'DREAMCAST PORT %s' in src,
 'runtime_id_preserved':'DCR_RUNTIME_BUILD_ID=cpp36b1e-runtime-external-raw-env' in bat,
 'auto_portable':'AUTO_PORTABLE_SYNC=1' in bat,
}
safe=r/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
expected='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4'
checks['safe_hash']=safe.is_file() and hashlib.sha256(safe.read_bytes()).hexdigest()==expected
exe=r/'portable/DreamcastRecompiled.exe'
checks['launcher_built']=exe.is_file() and exe.stat().st_size>100000
ok=True
for k,v in checks.items():
 print(('PASS' if v else 'FAIL'), k)
 ok &= v
if exe.is_file(): print('launcher_sha256', hashlib.sha256(exe.read_bytes()).hexdigest())
sys.exit(0 if ok else 1)
