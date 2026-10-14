from pathlib import Path
import subprocess, sys
root=Path(__file__).resolve().parents[3]
api=(root/'runtime/include/dcr_plugin_api_v2.hpp').read_text(errors='ignore')
maple=(root/'runtime/plugins/dcr_maple.cpp').read_text(errors='ignore')
run=(root/'run_game.bat').read_text(errors='ignore')
checks=[
 ('DCR_MAPLE_CONTROL_IFACE_V3' in api,'Maple Control V3'),
 ('kMapleFuncPuruPuru' in maple and 'maple_process_purupuru' in maple,'PuruPuru device'),
 ('XInputSetState' in maple,'native XInput rumble'),
 ('DCR_MAPLE_PORT_' in maple and 'input_spec' in maple,'per-port host selection'),
 ('keyboard_mouse' in maple and 'GetCursorPos' in maple,'combined keyboard + mouse'),
 ('xinput:' in maple and 'winmm' in maple,'independent controller specs'),
 ('cpp36b-purupuru-modern-input-v3' in maple,'cpp36b plugin build id'),
 ('--maple-trace' in run,'Maple trace launcher switch'),
]
fp=root/'tools/modular/guest_abi_fingerprint.py'
a=subprocess.check_output([sys.executable,str(fp),str(root/'runtime/include/dc_runtime.hpp')],text=True).strip()
b=subprocess.check_output([sys.executable,str(fp),str(root/'runtime/guest_abi/v1/dc_runtime.hpp')],text=True).strip()
checks.append((a==b and a=='00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f','Guest ABI1 fingerprint frozen'))
for ok,label in checks: print(('PASS' if ok else 'FAIL'),label)
if not all(ok for ok,_ in checks): sys.exit(1)
