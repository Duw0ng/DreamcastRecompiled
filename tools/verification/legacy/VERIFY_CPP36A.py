from pathlib import Path
import subprocess, sys
root=Path(__file__).resolve().parents[3]
checks=[]
api=(root/'runtime/include/dcr_plugin_api_v2.hpp').read_text(errors='ignore')
maple=(root/'runtime/plugins/dcr_maple.cpp').read_text(errors='ignore')
run=(root/'run_game.bat').read_text(errors='ignore')
checks += [
 ('DCR_MAPLE_CONTROL_IFACE_V3' in api, 'Maple Control V3 interface'),
 ('std::array<MaplePortState, 4>' in maple, 'four-port device tree'),
 ('std::array<MapleDeviceKind, 2>' in maple, 'two expansion slots per port'),
 ('DCR_MAPLE_TRACE' in maple and '--maple-trace' in run, 'Maple trace runtime + launcher BAT'),
 ('cpp36a-maple-device-tree-v3' in maple, 'cpp36a plugin build id'),
]
fp=root/'tools/modular/guest_abi_fingerprint.py'
a=subprocess.check_output([sys.executable,str(fp),str(root/'runtime/include/dc_runtime.hpp')],text=True).strip()
b=subprocess.check_output([sys.executable,str(fp),str(root/'runtime/guest_abi/v1/dc_runtime.hpp')],text=True).strip()
checks.append((a==b and a=='00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f','Guest ABI1 fingerprint frozen'))
for ok,label in checks: print(('PASS' if ok else 'FAIL'),label)
if not all(x[0] for x in checks): sys.exit(1)
