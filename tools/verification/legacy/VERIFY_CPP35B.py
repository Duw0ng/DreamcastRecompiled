from pathlib import Path
import hashlib, re
root=Path(__file__).resolve().parents[3]
maple=(root/'runtime/plugins/dcr_maple.cpp').read_text(encoding='utf-8')
host=(root/'runtime/dcr_plugin_host.cpp').read_text(encoding='utf-8')
v2=(root/'runtime/include/dcr_plugin_api_v2.hpp').read_text(encoding='utf-8')
cmake=(root/'runtime/CMakeLists.txt').read_text(encoding='utf-8')
checks={
 'Maple private build id':'cpp35b-maple-private-state-v2' in maple,
 'no compat bridge':'compat_device_' not in maple,
 'Maple control ABI':'DCRMapleControlV2' in v2 and 'DCR_MAPLE_CONTROL_IFACE_V2' in v2,
 'host bound control':'dc_plugin_maple_get_controller_state' in host,
 'Maple target':'add_library(dcr_maple SHARED' in cmake,
 'Windows Maple input libs':'target_link_libraries(dcr_maple PRIVATE user32 winmm)' in cmake,
}
for n,ok in checks.items(): print(('PASS' if ok else 'FAIL'),n)
if not all(checks.values()): raise SystemExit(1)
print('cpp35b source invariants: PASS')
