from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[3]
bat=(root/'run_game.bat').read_text(encoding='utf-8-sig')
helper=(root/'tools/portable/adopt_safe_guest.py').read_text(encoding='utf-8')
sync=(root/'tools/portable/sync_portable.py').read_text(encoding='utf-8')
checks={
 'version_cpp36b1d':'cpp36b1d AUTO_PORTABLE_SYNC_FIX' in bat,
 'sync_flag':'--sync-portable' in bat and 'SYNC_ONLY=1' in bat,
 'root_marker':'.dcr_portable_guest_v1' in bat and 'PORTABLE_GAME_MARKER' in bat,
 'safe_helper':'adopt_safe_guest.py' in bat,
 'normalized_metadata':"def norm(" in helper,
 'sha256_pin':'sha256' in helper.lower() and 'MODULE_SHA256' in helper,
 'same_module_no_copy':'actual YA ES' in helper,
 'auto_sync':'call :sync_portable' in bat,
 'zip_latest':'_portable_latest.zip' in sync,
 'no_guest_target_runtime':'set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple"' in bat,
}
seed=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
if seed.is_file():
    checks['safe_seed_hash']=hashlib.sha256(seed.read_bytes()).hexdigest()=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4'
for k,v in checks.items(): print(f'{k}:', 'PASS' if v else 'FAIL')
raise SystemExit(0 if all(checks.values()) else 1)
