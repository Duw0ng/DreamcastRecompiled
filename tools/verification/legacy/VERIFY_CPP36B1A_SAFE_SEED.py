from pathlib import Path
import hashlib, struct, sys
root=Path(__file__).resolve().parents[3]
mod=root/'safe_guests'/'CHUCHU_ROCKET'/'dcr_game_module.dll'
man=root/'safe_guests'/'CHUCHU_ROCKET'/'manifest.ini'
expected='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4'
assert mod.exists() and man.exists()
b=mod.read_bytes()
assert hashlib.sha256(b).hexdigest()==expected
assert b[:2]==b'MZ'
pe=struct.unpack_from('<I',b,0x3c)[0]
assert b[pe:pe+4]==b'PE\0\0'
assert b.find(b'dcr_game_abi_version_v1')>=0
assert b.find(b'dcr_game_attach_v1')>=0
# ABI function is the first exported text symbol in this known module: mov eax,0x10000; ret
assert b.find(bytes.fromhex('b800000100c3'))>=0
for a in [0x8C036102,0x8C036118,0x8C036160,0x8C036236,0x8C036526,0x8C036280]:
    assert b.find(a.to_bytes(4,'little'))>=0, hex(a)
txt=man.read_text(encoding='utf-8')
for line in ['DCR_GUEST_ABI=1','DCR_PORTABLE_SAFE=1','GAME_SLUG=CHUCHU_ROCKET',f'MODULE_SHA256={expected}']:
    assert line in txt, line
print('cpp36b1a SAFE seed: PASS')
print('SHA256:', expected)
print('ABI: Guest ABI1 / export 0x00010000')
print('Portable-safe provenance: previous online portable builder (--no-embed-raw-image)')
