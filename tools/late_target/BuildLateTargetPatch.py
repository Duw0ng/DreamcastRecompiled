#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def parse_addr(s: str) -> int:
    return int(s.strip(), 0) & 0xFFFFFFFF


def fmt_addr(v: int) -> str:
    return f"0x{v:08X}"


def run(cmd: list[str], cwd: Path | None = None) -> None:
    print("[LATE-AOT] $ " + " ".join(f'\"{x}\"' if " " in x else x for x in cmd), flush=True)
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def find_raw_recomp(root: Path) -> Path:
    candidates = [
        root / "build" / "Release" / "dc_raw_recomp.exe",
        root / "build" / "dc_raw_recomp.exe",
        root / "build" / "Release" / "dc_raw_recomp",
        root / "build" / "dc_raw_recomp",
    ]
    for p in candidates:
        if p.exists():
            return p
    raise FileNotFoundError("No encuentro dc_raw_recomp. Ejecuta primero el launcher/cmake del paquete.")


def read_seed_file(path: Path) -> list[int]:
    out: list[int] = []
    if not path.exists():
        return out
    for raw in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        try:
            value = parse_addr(line)
        except ValueError:
            continue
        if value not in out:
            out.append(value)
    return out


def write_seed_file(path: Path, seeds: list[int]) -> None:
    text = "# DreamcastRecompiled Late-AOT targets. One SH-4 address per line.\n"
    text += "\n".join(fmt_addr(x) for x in seeds) + "\n"
    path.write_text(text, encoding="utf-8")


def quote_cmake(p: Path) -> str:
    return p.resolve().as_posix().replace('"', '\\"')


def main() -> int:
    ap = argparse.ArgumentParser(description="Build a tiny ABI1 guest target overlay without recompiling the full game.")
    ap.add_argument("--root", required=True, help="DreamcastRecompiled package root")
    ap.add_argument("--game-dir", required=True, help="Per-game cache directory")
    ap.add_argument("--target", action="append", required=True,
                    help="Missing SH-4 target. Repeat --target to build multiple entries in one overlay.")
    ap.add_argument("--base", default="0x8C008000")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    game_dir = Path(args.game_dir).resolve()
    requested_targets = [parse_addr(x) for x in args.target]
    base = parse_addr(args.base)

    image = game_dir / "generated" / "BOOTSTRAP.BIN"
    if not image.exists():
        raise FileNotFoundError(f"No encuentro el bootstrap cacheado: {image}")

    runtime_header = root / "runtime" / "include" / "dc_runtime.hpp"
    plugin_api = root / "runtime" / "include" / "dcr_plugin_api.hpp"
    if not runtime_header.exists() or not plugin_api.exists():
        raise FileNotFoundError("Faltan headers ABI1 del runtime.")

    raw_recomp = find_raw_recomp(root)
    late_root = game_dir / "late_aot"
    seed_file = late_root / "late_target_seeds.txt"
    seeds = read_seed_file(seed_file)
    for target in requested_targets:
        if target not in seeds:
            seeds.append(target)
    # Deterministic order keeps the patch stable across rebuilds.
    seeds = sorted(set(seeds))

    src_dir = late_root / "generated_patch"
    build_dir = late_root / "build_patch"
    map_file = late_root / "late_target_map.csv"
    src_dir.mkdir(parents=True, exist_ok=True)
    late_root.mkdir(parents=True, exist_ok=True)

    # Recreate only the tiny overlay source. The original 5185-function guest cache is untouched.
    if src_dir.exists():
        shutil.rmtree(src_dir)
    src_dir.mkdir(parents=True, exist_ok=True)

    entry = seeds[0]
    cmd = [
        str(raw_recomp), str(image),
        f"--base={fmt_addr(base)}",
        f"--entry={fmt_addr(entry)}",
    ]
    for extra in seeds[1:]:
        cmd.append(f"--seed={fmt_addr(extra)}")
    cmd += [
        f"--max-functions={max(64, len(seeds) + 32)}",
        f"--max-closure-entries={max(1, len(seeds))}",
        "--closure-passes=1",
        f"--map={map_file}",
        f"--output={src_dir}",
    ]
    run(cmd, root)

    generated_program = src_dir / "generated_program.cpp"
    generated_header = src_dir / "generated_program.hpp"
    if not generated_program.exists() or not generated_header.exists():
        raise RuntimeError("dc_raw_recomp no genero generated_program.cpp/.hpp")

    # The raw emitter carries an embedded legacy header. Replace it with the exact ABI1 runtime header.
    shutil.copy2(runtime_header, src_dir / "dc_runtime.hpp")

    targets_text = ",".join(fmt_addr(x) for x in seeds)
    wrapper = f'''#include "generated_program.hpp"\n#include "dcr_plugin_api.hpp"\n\n#include <iostream>\n\nnamespace dcrecomp_generated {{\nnamespace {{\n\nbool late_aot_startup(DCRuntime* runtime) {{\n    if (!runtime) return false;\n    register_recompiled_program(*runtime);\n    std::cout << "[GUEST PATCH] cpp34x Late-AOT overlay active | targets={targets_text}" << "\\n";\n    return true;\n}}\n\nvoid late_aot_shutdown(DCRuntime*) {{}}\n\nconst DCRPluginDescriptorV1 kLateAotDescriptor{{\n    DCR_PLUGIN_ABI_V1,\n    sizeof(DCRPluginDescriptorV1),\n    100u,\n    0u,\n    "DreamcastRecompiled Guest Late-AOT Overlay",\n    "cpp34x-late-aot-overlay-abi1",\n    &late_aot_startup,\n    &late_aot_shutdown,\n    nullptr, nullptr, nullptr, nullptr\n}};\n\n}} // namespace\n\nDCR_PLUGIN_EXPORT const DCRPluginDescriptorV1* dcr_plugin_query_v1() {{\n    return &kLateAotDescriptor;\n}}\n\n}} // namespace dcrecomp_generated\n'''
    (src_dir / "guest_patch_plugin.cpp").write_text(wrapper, encoding="utf-8")

    generated_sources = sorted(p.name for p in src_dir.glob("generated_program*.cpp"))
    if not generated_sources:
        raise RuntimeError("No encontre fuentes generated_program*.cpp")

    game_build = game_dir / "build"
    plugin_out = game_build / "plugins" / "Release"
    plugin_out.mkdir(parents=True, exist_ok=True)

    is_windows = os.name == "nt"
    if is_windows:
        runtime_implib = game_build / "Release" / "DreamcastRuntime.lib"
        runtime_dll = game_build / "Release" / "DreamcastRuntime.dll"
        if not runtime_implib.exists() or not runtime_dll.exists():
            raise FileNotFoundError(
                "Falta DreamcastRuntime.lib/.dll en el cache. Ejecuta una vez TEST_CPP34P/--rebuild-runtime y vuelve a intentar."
            )
        imported_props = f'''\nset_target_properties(DreamcastRuntime PROPERTIES\n    IMPORTED_CONFIGURATIONS RELEASE\n    IMPORTED_IMPLIB_RELEASE "{quote_cmake(runtime_implib)}"\n    IMPORTED_LOCATION_RELEASE "{quote_cmake(runtime_dll)}"\n)\n'''
    else:
        candidates = [
            game_build / "Release" / "libDreamcastRuntime.so",
            game_build / "libDreamcastRuntime.so",
            root.parent / "build_cpp34p_runtime" / "Release" / "libDreamcastRuntime.so",
        ]
        runtime_so = next((p for p in candidates if p.exists()), None)
        if runtime_so is None:
            raise FileNotFoundError("No encuentro libDreamcastRuntime.so para validar el overlay.")
        imported_props = f'''\nset_target_properties(DreamcastRuntime PROPERTIES\n    IMPORTED_LOCATION "{quote_cmake(runtime_so)}"\n)\n'''

    src_lines = "\n    ".join(generated_sources + ["guest_patch_plugin.cpp"])
    cmake_text = f'''cmake_minimum_required(VERSION 3.20)\nproject(DCRLateAOTPatch LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nset(CMAKE_CXX_STANDARD_REQUIRED ON)\nset(CMAKE_CXX_EXTENSIONS OFF)\n\nadd_library(DreamcastRuntime SHARED IMPORTED GLOBAL)\n{imported_props}\nadd_library(dcr_guest_patch SHARED\n    {src_lines}\n)\ntarget_include_directories(dcr_guest_patch PRIVATE\n    "{quote_cmake(src_dir)}"\n    "{quote_cmake(root / 'runtime' / 'include')}"\n)\ntarget_compile_definitions(dcr_guest_patch PRIVATE DCR_PLUGIN_BUILD=1 NOMINMAX WIN32_LEAN_AND_MEAN)\ntarget_link_libraries(dcr_guest_patch PRIVATE DreamcastRuntime)\nset_target_properties(dcr_guest_patch PROPERTIES\n    PREFIX ""\n    OUTPUT_NAME "dcr_guest_patch"\n    INTERPROCEDURAL_OPTIMIZATION FALSE\n    RUNTIME_OUTPUT_DIRECTORY "{quote_cmake(plugin_out)}"\n    LIBRARY_OUTPUT_DIRECTORY "{quote_cmake(plugin_out)}"\n    ARCHIVE_OUTPUT_DIRECTORY "{quote_cmake(plugin_out)}"\n    RUNTIME_OUTPUT_DIRECTORY_RELEASE "{quote_cmake(plugin_out)}"\n    LIBRARY_OUTPUT_DIRECTORY_RELEASE "{quote_cmake(plugin_out)}"\n    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "{quote_cmake(plugin_out)}"\n)\nif(MSVC)\n    target_compile_options(dcr_guest_patch PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)\nelse()\n    target_compile_options(dcr_guest_patch PRIVATE $<$<CONFIG:Release>:-O3> -pipe)\nendif()\n'''
    (src_dir / "CMakeLists.txt").write_text(cmake_text, encoding="utf-8")

    if build_dir.exists():
        shutil.rmtree(build_dir)
    configure = ["cmake", "-S", str(src_dir), "-B", str(build_dir)]
    if is_windows:
        configure += ["-A", "x64"]
    run(configure, root)
    run(["cmake", "--build", str(build_dir), "--config", "Release", "--target", "dcr_guest_patch", "--parallel", "1"], root)

    dll_name = "dcr_guest_patch.dll" if is_windows else ("dcr_guest_patch.dylib" if platform.system() == "Darwin" else "dcr_guest_patch.so")
    built = plugin_out / dll_name
    if not built.exists():
        # Single-config generators may add no Release subdir despite properties.
        candidates = list(build_dir.rglob(dll_name))
        if candidates:
            shutil.copy2(candidates[0], built)
    if not built.exists():
        raise RuntimeError(f"El build termino pero no encuentro {dll_name}")

    write_seed_file(seed_file, seeds)
    print(f"[LATE-AOT] OK: {built}")
    print(f"[LATE-AOT] Targets: {targets_text}")
    print("[LATE-AOT] El guest principal NO fue regenerado ni recompilado.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as e:
        print(f"[LATE-AOT ERROR] comando fallo con codigo {e.returncode}", file=sys.stderr)
        raise SystemExit(e.returncode)
    except Exception as e:
        print(f"[LATE-AOT ERROR] {e}", file=sys.stderr)
        raise SystemExit(1)
