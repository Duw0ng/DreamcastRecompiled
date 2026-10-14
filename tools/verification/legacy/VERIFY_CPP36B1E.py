from pathlib import Path
import hashlib, sys
root=Path(__file__).resolve().parents[3]
bat=(root/'run_game.bat').read_text(encoding='utf-8-sig')
rt=(root/'runtime/dc_runtime.cpp').read_text()
go=(root/'portable/launcher/main_windows.go').read_text()
checks={
 'version': 'cpp36b1e RUNTIME_EXTERNAL_RAW_ENV' in bat,
 'runtime_stamp': 'cpp36b1e-runtime-external-raw-env' in bat,
 'bat_env_image': 'DCR_EXTERNAL_RAW_IMAGE' in bat,
 'bat_no_external_cli_exec': '!EXTERNAL_RAW_ARGS!' not in bat,
 'runtime_env_load': 'external raw image loaded by Runtime ENV' in rt,
 'runtime_env_base': 'DCR_EXTERNAL_RAW_BASE' in rt,
 'launcher_v05': 'const launcherVersion = "0.5"' in go,
 'launcher_env_image': 'DCR_EXTERNAL_RAW_IMAGE=' in go,
 'launcher_no_external_cli': '"--external-raw-image=" + bootstrap' not in go,
}
safe=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
if safe.exists():
    checks['safe_hash']=hashlib.sha256(safe.read_bytes()).hexdigest()=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4'
else: checks['safe_hash']=False
for k,v in checks.items(): print(('PASS' if v else 'FAIL'), k)
sys.exit(0 if all(checks.values()) else 1)
