#!/usr/bin/env python3
"""Create an AICA Rev 2 full DreamcastRecompiled v1.2 test distribution.

Repack the exact published 52 MB full Stable ZIP, preserving its Aqua/Python
launcher, assets, tools, precompiled guest, system plugins and data. Replace
only the matching runtime DLL, runtime source/tests, checksum entries and add
explicit experimental test utilities. No commercial disc/BIOS is included.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import sys
import zipfile

STABLE_SHA = "42d08b83ad718214ef19a597332d690c4b5f446fdb7422dd8455df55cca1c127"
RUNTIME_SUFFIX = "/bin/DreamcastRuntime.dll"
SH4_SUFFIX = "/bin/dcr_game_module.dll"
SOURCE_FILES = [
    "runtime/dc_runtime.cpp",
    "runtime/dc_arm7.cpp",
    "runtime/include/dc_runtime.hpp",
    "runtime/include/dcr_aica_arm_memory.hpp",
    "runtime/CMakeLists.txt",
    "runtime/tests_aica_arm_bus.cpp",
    "runtime/tests_aica_one_shot_keyon.cpp",
    "runtime/tests_portable_ipbin_stage.cpp",
]
ADD_FILES = {
    "TEST_CRAZY_TAXI_2_FPS.bat": "scripts/perf/TEST_CRAZY_TAXI_2_FPS.bat",
    "REV2_AICA_FULL_TEST_README.md": "docs/DCRV12_REV2_FULL_AUDIO_CT2_TEST.md",
}
GAME_IMAGES = (".cdi", ".gdi", ".chd", ".iso", ".nrg", ".mdf", ".mds")

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def patch_sha_index(index: bytes, replacements: dict[str, tuple[str, str]]) -> bytes:
    """Update only old SHA tokens matching a genuinely replaced game binary."""
    text = index.decode("utf-8-sig")
    output=[]
    for line in text.splitlines(keepends=True):
        original = line.rstrip("\r\n")
        match=re.match(r"(?i)^([0-9a-f]{64})(\s+\*?)(.+)$",original)
        if match:
            path=match.group(3).replace("\\","/").removeprefix("./")
            full = next((repl for k,repl in replacements.items()
                         if path==k or k.endswith("/"+path)), None)
            if full and match.group(1).lower()==full[0]:
                line=full[1]+original[64:]+line[len(original):]
        output.append(line)
    return "".join(output).encode("utf-8")

def main() -> None:
    parser=argparse.ArgumentParser()
    parser.add_argument("--stable",type=pathlib.Path,required=True)
    parser.add_argument("--runtime",type=pathlib.Path,required=True)
    parser.add_argument("--repository",type=pathlib.Path,default=pathlib.Path("."))
    parser.add_argument("--output",type=pathlib.Path,required=True)
    args=parser.parse_args()

    stable=args.stable.read_bytes()
    assert sha(stable)==STABLE_SHA, "Full Stable v1.2 must be the exact officially published ZIP"
    runtime=args.runtime.read_bytes()
    assert runtime.startswith(b"MZ") and len(runtime)>500000, "Missing compiled Win64 runtime DLL"
    new_hash=sha(runtime)

    source={f:(args.repository/f).read_bytes() for f in SOURCE_FILES}
    extras={f:(args.repository/local).read_bytes() for f,local in ADD_FILES.items()}
    if any(x in source["runtime/dc_runtime.cpp"] for x in
           [b"One-shot never starts"]):
        raise AssertionError("Unexpected unverified audio source")
    assert b"aica_native_stop_finished_slot" in source["runtime/dc_runtime.cpp"]
    assert b"KYONB" in source["runtime/tests_aica_one_shot_keyon.cpp"]

    args.output.parent.mkdir(parents=True,exist_ok=True)
    changed=[]
    original_guest={}
    original_launcher=None
    old_runtime_hashes={}
    with zipfile.ZipFile(args.stable) as z:
        assert z.testzip() is None, "Original stable ZIP CRC error"
        names=set(z.namelist())
        assert "DreamcastRecompiled.exe" in names
        assert "launcher/vmu_standalone.py" in names
        assert "games/CHUCHU_ROCKET/bin/dcr_game_module.dll" in names
        assert "games/CHUCHU_ROCKET/bin/DreamcastRuntime.dll" in names
        assert all(s in names for s in SOURCE_FILES if s != "runtime/tests_aica_one_shot_keyon.cpp"), "Stable ZIP runtime sources incomplete"
        assert not any(n.lower().endswith(GAME_IMAGES) for n in names), "Game disc unexpectedly included in source"
        assert not (set(ADD_FILES) & names), "Source already has experimental files"
        for name in names:
            p=pathlib.PurePosixPath(name)
            assert not p.is_absolute() and ".." not in p.parts and "\\" not in name
        original_launcher=sha(z.read("DreamcastRecompiled.exe"))
        original_guest={n:sha(z.read(n)) for n in names if n.endswith(SH4_SUFFIX)}
        for name in names:
            if name.endswith(RUNTIME_SUFFIX):
                old_runtime_hashes[name]=sha(z.read(name))
        assert old_runtime_hashes, "No replaceable DreamcastRuntime.dll found"
        changed_pairs={k:(old,new_hash) for k,old in old_runtime_hashes.items()}

        with zipfile.ZipFile(args.output,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as out:
            for member in z.infolist():
                name=member.filename
                if member.is_dir():
                    out.writestr(member,b"")
                    continue
                payload=z.read(name)
                if name.endswith(RUNTIME_SUFFIX):
                    payload=runtime
                    changed.append(name)
                elif name in source:
                    payload=source[name]
                    changed.append(name)
                elif name.endswith("checksums.sha256") or name.endswith(".sha256"):
                    payload=patch_sha_index(payload,changed_pairs)
                out.writestr(member,payload)
            # New regression source file missing from the original stable.
            for path,data in source.items():
                if path not in names:
                    out.writestr(path,data)
            for path,data in extras.items():
                out.writestr(path,data)

    with zipfile.ZipFile(args.output) as done:
        assert done.testzip() is None, "Repacked ZIP CRC error"
        assert sha(done.read("DreamcastRecompiled.exe"))==original_launcher
        assert all(sha(done.read(n))==h for n,h in original_guest.items())
        assert all(sha(done.read(n))==new_hash for n in old_runtime_hashes)
        assert b"aica_native_stop_finished_slot" in done.read("runtime/dc_runtime.cpp")
        assert "TEST_CRAZY_TAXI_2_FPS.bat" in done.namelist()
        assert "REV2_AICA_FULL_TEST_README.md" in done.namelist()

    result_sha=sha(args.output.read_bytes())
    checksum=args.output.with_name(args.output.name+".sha256")
    checksum.write_text(result_sha+"  "+args.output.name+"\n",encoding="ascii")
    print("ORIGINAL_STABLE_SHA256="+STABLE_SHA)
    print("PRESERVED_LAUNCHER_SHA256="+original_launcher)
    print("PRESERVED_SH4_GUESTS="+str(len(original_guest)))
    print("UPDATED_RUNTIME_SHA256="+new_hash)
    print("UPDATED_RUNTIME_INSTANCES="+str(len(old_runtime_hashes)))
    for f in sorted(set(changed)): print("UPDATED="+f)
    print("RESULT="+str(args.output))
    print("RESULT_SHA256="+result_sha)
    print("RESULT_BYTES="+str(args.output.stat().st_size))

if __name__=="__main__":
    try:
        main()
    except Exception as e:
        print("FULL REV2 PACKAGING FAILED:",str(e),file=sys.stderr)
        raise
