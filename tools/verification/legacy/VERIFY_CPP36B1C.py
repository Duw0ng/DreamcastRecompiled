from pathlib import Path
import hashlib, re, sys
root=Path(__file__).resolve().parents[3]
maple=(root/'runtime/plugins/dcr_maple.cpp').read_text(encoding='utf-8')
network=(root/'runtime/plugins/dcr_network.cpp').read_text(encoding='utf-8')
runner=(root/'run_game.bat').read_text(encoding='utf-8-sig')
checks={
    'MSVC LONG/int clamp removed': '(pt.x-ps.mouse_x)*5' not in maple and '(pt.y-ps.mouse_y)*5' not in maple,
    'mouse coordinates normalized to int': 'const int mouse_x = static_cast<int>(pt.x);' in maple and 'const int mouse_y = static_cast<int>(pt.y);' in maple,
    'mouse deltas clamp as int': 'std::clamp(dx, -127, 127)' in maple and 'std::clamp(dy, -127, 127)' in maple,
    'NOMINMAX guarded': '#ifndef NOMINMAX\n#define NOMINMAX\n#endif\n#include <windows.h>' in maple,
    'WIN32_LEAN_AND_MEAN guarded': '#ifndef WIN32_LEAN_AND_MEAN\n#define WIN32_LEAN_AND_MEAN\n#endif\n#include <winsock2.h>' in network,
    'runtime build id bumped': 'cpp36b1c-msvc-maple-input-fix' in runner,
    'safe guest preserved': (root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll').exists(),
}
failed=[]
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL'), name)
    if not ok: failed.append(name)
if failed: sys.exit(1)
p=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
h=hashlib.sha256(p.read_bytes()).hexdigest()
print('SAFE guest SHA256:', h)
if h != 'ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4':
    print('FAIL safe guest hash changed')
    sys.exit(1)
print('cpp36b1c MSVC Maple input fix: PASS')
