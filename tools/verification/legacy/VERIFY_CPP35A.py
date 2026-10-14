from pathlib import Path
import re

root = Path(__file__).resolve().parents[3]
host = (root / "runtime" / "dcr_plugin_host.cpp").read_text(encoding="utf-8")
v2 = (root / "runtime" / "include" / "dcr_plugin_api_v2.hpp").read_text(encoding="utf-8")
maple = (root / "runtime" / "plugins" / "dcr_maple.cpp").read_text(encoding="utf-8")
cmake = (root / "runtime" / "CMakeLists.txt").read_text(encoding="utf-8")
launcher = (root / "run_game.bat").read_text(encoding="utf-8")
mod = (root / "tools" / "modular" / "modularize_generated.py").read_text(encoding="utf-8")

checks = {
    "ABI2 constant": "DCR_PLUGIN_ABI_V2" in v2,
    "versioned host services": "DCRHostServicesV2" in v2 and "struct_size" in v2,
    "opaque plugin create": "void* (*create)(const DCRHostServicesV2* host)" in v2,
    "generic V2 query": 'dcr_plugin_query_v2' in host,
    "generic fast-kind cache": "kFastKinds" in host,
    "Maple ABI2 plugin": 'DreamcastRecompiled Maple' in maple and 'dcr_plugin_query_v2' in maple,
    "Maple CMake target": "add_library(dcr_maple SHARED" in cmake,
    "launcher rebuild-maple": "--rebuild-maple" in launcher,
    "runner depends on Maple": "add_dependencies(dreamcast_program dcr_network dcr_maple)" in mod,
}
for name, ok in checks.items():
    print(f"{'PASS' if ok else 'FAIL'}  {name}")
if not all(checks.values()):
    raise SystemExit(1)
print("cpp35a source invariants: PASS")
