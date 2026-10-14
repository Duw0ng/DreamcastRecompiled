from pathlib import Path
import hashlib, sys, subprocess
root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, ok):
    checks.append((name,bool(ok)))
    print(('PASS' if ok else 'FAIL'), name)
rt=(root/'runtime/dc_runtime.cpp').read_text()
em=(root/'src/codegen/cpp_emitter.cpp').read_text()
run=(root/'run_game.bat').read_text(errors='ignore')
go=(root/'portable/launcher/main_windows.go').read_text()
check('runtime build id', 'v1.2-cpp38b1-pvr-superblock-recovery' in rt)
check('cpp38a index scratch restored runtime', 'auto& indices = gpu.index_scratch;' in rt and 'std::memcpy(mapped.pData, indices.data()' in rt)
check('cpp38a index scratch restored emitter', 'auto& indices = gpu.index_scratch;' in em and 'std::memcpy(mapped.pData, indices.data()' in em)
check('cpp38b mapped builder removed runtime', 'pvr_gpu_build_opaque_stream_mapped' not in rt and 'mapped PVR index stream overflow/empty' not in rt)
check('cpp38b mapped builder removed emitter', 'pvr_gpu_build_opaque_stream_mapped' not in em)
check('superblock runtime default', 'fpu_aot_mode = 2u;' in rt)
check('superblock emitter default', 'fpu_aot_mode = 2u;' in em)
check('FPU metrics default off runtime', 'fpu_region_metrics = false;' in rt)
check('FPU metrics default off emitter', 'fpu_region_metrics = false;' in em)
check('262144 host sync runtime', 'device_clock_host_sync_quantum_cycles = 262144u;' in rt)
check('262144 host sync emitter', 'device_clock_host_sync_quantum_cycles = 262144u;' in em)
check('run_game cpp38b1 defaults', 'DCR_BUILD_SHORT=cpp38b1' in run and 'FPU_MODE=superblock' in run and 'FPU_METRICS=0' in run and 'HOST_SYNC_QUANTUM=262144' in run)
check('run_game A/B flags', '--fpu-regionplus' in run and '--sync-524' in run and '--fpu-metrics' in run)
check('portable cpp38b1 defaults', 'DCR_BUILD_SHORT=cpp38b1' in go and 'DCR_FPU_MODE=superblock' in go and 'DCR_FPU_METRICS=0' in go and 'DCR_HOST_SYNC_QUANTUM=262144' in go)
check('recommended test script', (root/'scripts/tests/TEST_CPP38B1_FAST.bat').is_file())
check('Region+ A/B script', (root/'scripts/tests/TEST_CPP38B1_REGIONPLUS_AB.bat').is_file())
check('524 A/B script', (root/'scripts/tests/TEST_CPP38B1_SYNC524_AB.bat').is_file())
proc=subprocess.run([sys.executable, str(root/'tools/modular/guest_abi_fingerprint.py'),
                     str(root/'runtime/include/dc_runtime.hpp'),
                     str(root/'runtime/guest_abi/v1/dc_runtime.hpp')],
                    capture_output=True, text=True)
check('ABI1 fingerprint MATCH', proc.returncode==0 and 'MATCH' in proc.stdout and '00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f' in proc.stdout)
guest=root/'safe_guests/CHUCHU_ROCKET/dcr_game_module.dll'
sha=hashlib.sha256(guest.read_bytes()).hexdigest() if guest.is_file() else ''
check('SAFE guest hash', sha=='ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4')
exe=root/'portable/DreamcastRecompiled.exe'
check('portable launcher present', exe.is_file() and exe.read_bytes()[:2] == b'MZ')
if not all(ok for _,ok in checks): sys.exit(1)
