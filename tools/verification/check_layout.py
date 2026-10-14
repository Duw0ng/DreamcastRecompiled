from pathlib import Path
import hashlib,re,sys
root=Path(__file__).resolve().parents[2]
required=['run_game.bat','build_windows.bat','CMakeLists.txt','runtime/dc_runtime.cpp','safe_guests/CHUCHU_ROCKET/dcr_game_module.dll','scripts/stable/APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat','scripts/stable/TEST_CPP38E_FAST.bat','scripts/stable/TEST_CPP38E_HOTPC_DIAG.bat','tools/verification/legacy/VERIFY_CPP38E.py']
errors=[]
for rel in required:
 if not (root/rel).is_file():errors.append('Falta '+rel)
run=(root/'run_game.bat').read_text(encoding='utf-8-sig')
entry=(root/'INICIAR_DREAMCAST_RECOMPILED_V1_2.bat').read_text(encoding='utf-8-sig')
if 'scripts\\stable\\APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat' not in run: errors.append('Ruta APPLY del runner')
if 'scripts\\stable\\TEST_CPP38E_FAST.bat' not in entry: errors.append('Ruta TEST FAST del inicio')
for p in (root/'scripts').rglob('*.bat'):
 t=p.read_text(encoding='utf-8-sig',errors='replace')
 if ('cd /d "%~dp0"' in t or '%~dp0run_game.bat' in t):errors.append('Ruta BAT no migrada '+p.name)
for p in root.glob('VERIFY_CPP*.py'):errors.append('Verificador en raiz '+p.name)
print('Estructura:',len(list((root/'scripts').rglob('*.bat'))),'scripts en scripts/')
if errors:
 print('ERRORES:');print('\n'.join(errors));sys.exit(1)
print('PASS: estructura cpp38e, entradas y archivos criticos')
