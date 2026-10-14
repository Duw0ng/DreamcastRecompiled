#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path
import shutil

ABI = "1"


def die(msg: str) -> None:
    raise SystemExit(msg)


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8")


def write(p: Path, s: str) -> None:
    data = s.replace("\r\n", "\n").encode("utf-8")
    if p.exists() and p.read_bytes() == data:
        return
    p.write_bytes(data)


def copy_if_different(src: Path, dst: Path) -> None:
    data = src.read_bytes()
    if dst.exists() and dst.read_bytes() == data:
        return
    dst.write_bytes(data)


def discover_registration(cpp: Path) -> str:
    candidates = [cpp / "generated_program.hpp"] + sorted(cpp.glob("generated_*.hpp"))
    for p in candidates:
        if not p.exists():
            continue
        text = read(p)
        m = re.search(r"\bvoid\s+(register_recompiled_program|register_generated_[A-Za-z0-9_]+)\s*\(DCRuntime&", text)
        if m:
            return m.group(1)
    die("[MODULAR] no pude encontrar la funcion de registro generada")


def discover_program_sources(cpp: Path) -> list[str]:
    shards = sorted(p.name for p in cpp.glob("generated_program_part_*.cpp"))
    out: list[str] = []
    if shards:
        if (cpp / "generated_program.cpp").exists():
            out.append("generated_program.cpp")
        out.extend(shards)
        return out
    if (cpp / "generated_program.cpp").exists():
        return ["generated_program.cpp"]
    # Single-function emitter: exclude runner/test glue.
    for p in sorted(cpp.glob("generated_*.cpp")):
        if p.name in {"generated_runner.cpp", "generated_compile_test.cpp", "generated_main.cpp"}:
            continue
        out.append(p.name)
    if (cpp / "generated_main.cpp").exists():
        out.append("generated_main.cpp")
    if not out:
        die("[MODULAR] no encontre fuentes guest generadas")
    return out


def patch_runner(cpp: Path) -> None:
    p = cpp / "generated_runner.cpp"
    if not p.exists():
        return
    s = read(p)
    if '#include "dcr_game_api.hpp"' not in s:
        # Keep the generated header if other declarations are referenced; add stable API.
        first_include_end = s.find("\n", s.find("#include"))
        s = s[: first_include_end + 1] + '#include "dcr_game_api.hpp"\n' + s[first_include_end + 1 :]
    if '#include <cstdlib>' not in s:
        anchor = '#include <chrono>\n'
        s = s.replace(anchor, '#include <cstdlib>\n' + anchor, 1)
    s = re.sub(
        r"\s*load_embedded_elf_image\(runtime\);\s*\n\s*register_(?:recompiled_program|generated_[A-Za-z0-9_]+)\(runtime\);",
        '\n        if (!dcr_game_attach_v1(&runtime)) throw std::runtime_error("DCR guest module ABI attach failed");',
        s,
        count=1,
    )
    if "dc_plugins_load_default(runtime);" not in s:
        s = s.replace(
            "        if (!dcr_game_attach_v1(&runtime)) throw std::runtime_error(\"DCR guest module ABI attach failed\");",
            "        if (!dcr_game_attach_v1(&runtime)) throw std::runtime_error(\"DCR guest module ABI attach failed\");\n"
            "        dc_plugins_load_default(runtime);",
            1,
        )
    # Make plugin shutdown deterministic on normal exit and exceptions with a tiny RAII guard.
    if "DCRPluginShutdownGuard" not in s:
        marker = "int main(int argc, char** argv) {"
        guard = (
            "struct DCRPluginShutdownGuard {\n"
            "    dcrecomp_generated::DCRuntime* runtime{};\n"
            "    ~DCRPluginShutdownGuard() { if (runtime) dcrecomp_generated::dc_plugins_shutdown(*runtime); }\n"
            "};\n\n"
        )
        s = s.replace(marker, guard + marker, 1)
        s = s.replace("    DCRuntime runtime;", "    DCRuntime runtime;\n    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};", 1)
    if "DCR_SYSTEM_DATA_DIR" not in s:
        save_setup = (
            '    if (const char* dcr_system_dir = std::getenv("DCR_SYSTEM_DATA_DIR"); dcr_system_dir && *dcr_system_dir) {\n'
            '        std::error_code dcr_save_ec;\n'
            '        const std::filesystem::path dcr_save_root(dcr_system_dir);\n'
            '        std::filesystem::create_directories(dcr_save_root, dcr_save_ec);\n'
            '        runtime.bios_flash_path = (dcr_save_root / "dreamcast_flash.bin").string();\n'
            '        runtime.maple_vmu_path = (dcr_save_root / "dreamcast_vmu_a1.bin").string();\n'
            '        std::cout << "[DCR DATA] system=" << dcr_save_root.string() << "\\n";\n'
            '    }\n'
        )
        s = s.replace("    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};\n",
                      "    DCRPluginShutdownGuard dcr_plugin_guard{&runtime};\n" + save_setup, 1)
    # Version display only; guest code itself is unchanged. Accept cached
    # runners from cpp33c through cpp34f so --rebuild-runtime cannot accidentally
    # keep an old banner after the DLL has been updated.
    for old_banner in (
        "cpp33c PLANETWEB_DUAL_ISP",
        "cpp34a MODULAR_RUNTIME_ABI1",
        "cpp34b MODULAR_FULL_ISP_ABI1",
        "cpp34c FLASH_BASE0_FIX_ABI1",
        "cpp34d IPBIN_REENTRY_SHADOW_GUARD_ABI1",
        "cpp34e BOOTSAFE_IPBIN_REENTRY_ABI1",
        "cpp34f ALIAS_SAFE_IPBIN_REENTRY_ABI1",
        "cpp34g PRESERVE_RELOC_IPBIN_ABI1",
        "cpp34h BOOTROM_RO_FLYCAST_MODEM_TRACE_ABI1",
        "cpp34i FLYCAST_MODEM_FULLSTACK_ABI1",
        "cpp34i1 FLYCAST_MODEM_FULLSTACK_MSVC_FIX_ABI1",
    ):
        s = s.replace(old_banner, "cpp34k STACKSAFE_IPBIN_REENTRY_ABI1")
    s = s.replace("v1.2-cpp33c-network", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34c-modular-full-isp", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34d-ipbin-reentry-shadow-guard", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34e-bootsafe-ipbin-reentry", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34f-alias-safe-ipbin-reentry", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34g-preserve-reloc-ipbin", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34h-bootrom-ro-flycast-modem-trace", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    s = s.replace("v1.2-cpp34i-flycast-modem-fullstack", "v1.2-cpp34k-stacksafe-ipbin-reentry")
    write(p, s)


def patch_compile_test(cpp: Path) -> None:
    p = cpp / "generated_compile_test.cpp"
    if not p.exists():
        return
    s = read(p)
    if '#include "dcr_game_api.hpp"' not in s:
        s = '#include "dcr_game_api.hpp"\n' + s
    s = re.sub(
        r"\s*dcrecomp_generated::load_embedded_elf_image\(runtime\);\s*\n\s*dcrecomp_generated::register_(?:recompiled_program|generated_[A-Za-z0-9_]+)\(runtime\);",
        '\n    if (!dcrecomp_generated::dcr_game_attach_v1(&runtime)) return 90;',
        s,
        count=1,
    )
    write(p, s)


def emit_game_api(cpp: Path, registration: str) -> None:
    hdr = r'''#pragma once
#include "dc_runtime.hpp"
#include <cstdint>

namespace dcrecomp_generated {
inline constexpr std::uint32_t DCR_GUEST_ABI_V1 = 0x00010000u;

#if defined(_WIN32)
#  if defined(DCR_GAME_MODULE_BUILD)
#    define DCR_GAME_EXPORT extern "C" __declspec(dllexport)
#  else
#    define DCR_GAME_EXPORT extern "C" __declspec(dllimport)
#  endif
#else
#  define DCR_GAME_EXPORT extern "C" __attribute__((visibility("default")))
#endif

DCR_GAME_EXPORT std::uint32_t dcr_game_abi_version_v1();
DCR_GAME_EXPORT bool dcr_game_attach_v1(DCRuntime* runtime);
} // namespace dcrecomp_generated
'''
    src = f'''#include "dcr_game_api.hpp"\n#include "dc_image.hpp"\n#include "generated_program.hpp"\n\nnamespace dcrecomp_generated {{\n\nDCR_GAME_EXPORT std::uint32_t dcr_game_abi_version_v1() {{ return DCR_GUEST_ABI_V1; }}\n\nDCR_GAME_EXPORT bool dcr_game_attach_v1(DCRuntime* runtime) {{\n    if (!runtime) return false;\n    load_embedded_elf_image(*runtime);\n    {registration}(*runtime);\n    return true;\n}}\n\n}} // namespace dcrecomp_generated\n'''
    write(cpp / "dcr_game_api.hpp", hdr)
    write(cpp / "dcr_game_exports.cpp", src)


def emit_cmake(cpp: Path, sources: list[str]) -> None:
    src_lines = "\n".join(f"    {x}" for x in sources)
    cm = f'''cmake_minimum_required(VERSION 3.20)
project(DreamcastRecompGeneratedModular LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

set(DCR_RUNTIME_ROOT "" CACHE PATH "DreamcastRecompiled modular runtime source directory")
if(NOT DCR_RUNTIME_ROOT OR NOT EXISTS "${{DCR_RUNTIME_ROOT}}/CMakeLists.txt")
    message(FATAL_ERROR "DCR_RUNTIME_ROOT must point to the cpp34+ runtime directory")
endif()
set(DCR_RUNTIME_OUTPUT_DIR "${{CMAKE_BINARY_DIR}}" CACHE PATH "" FORCE)
add_subdirectory("${{DCR_RUNTIME_ROOT}}" "${{CMAKE_BINARY_DIR}}/dcr_runtime_build")

set(DCR_GUEST_SOURCES
{src_lines}
)

# The SH-4 closure is now its own DLL. Its object files are independent from
# runtime/device/plugin changes and remain reusable across DreamcastRecompiled releases.
add_library(dcr_game_module SHARED
    dcr_game_exports.cpp
    dc_image.cpp
    ${{DCR_GUEST_SOURCES}}
)
target_compile_definitions(dcr_game_module PRIVATE DCR_GAME_MODULE_BUILD=1 DCR_GUEST_ABI_VERSION=1)
target_compile_definitions(dcr_game_module PRIVATE DCR_FIXED_FPU_REGION_METRICS=0)
target_include_directories(dcr_game_module PRIVATE "${{CMAKE_CURRENT_SOURCE_DIR}}")
target_link_libraries(dcr_game_module PRIVATE DreamcastRuntime)
set_target_properties(dcr_game_module PROPERTIES
    PREFIX ""
    INTERPROCEDURAL_OPTIMIZATION FALSE
)
option(DCR_HOST_AVX2 "Tune guest code for AVX2/FMA hosts" ON)
if(MSVC)
    target_compile_options(dcr_game_module PRIVATE
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi> $<$<CONFIG:Release>:/Ot> $<$<CONFIG:Release>:/Gy>)
    if(DCR_HOST_AVX2)
        target_compile_options(dcr_game_module PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
        target_compile_definitions(dcr_game_module PRIVATE DCR_HOST_AVX2=1)
    endif()
else()
    target_compile_options(dcr_game_module PRIVATE $<$<CONFIG:Release>:-O3> -pipe)
endif()

add_executable(generated_compile_test generated_compile_test.cpp)
target_link_libraries(generated_compile_test PRIVATE dcr_game_module DreamcastRuntime)

add_executable(dreamcast_program generated_runner.cpp)
target_link_libraries(dreamcast_program PRIVATE dcr_game_module DreamcastRuntime)
add_dependencies(dreamcast_program dcr_network dcr_maple)
if(MSVC)
    target_link_options(dreamcast_program PRIVATE $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()
'''
    write(cpp / "CMakeLists.txt", cm)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("cpp_dir", type=Path)
    ap.add_argument("--runtime-root", type=Path, required=True)
    ns = ap.parse_args()
    cpp = ns.cpp_dir.resolve()
    runtime = ns.runtime_root.resolve()
    if not cpp.is_dir():
        die(f"[MODULAR] cpp_dir no existe: {cpp}")
    # cpp35b1: generated SH-4 compiles against the frozen Guest ABI v1 headers.
    # Runtime/plugin headers may evolve freely without invalidating thousands of
    # guest translation units. Existing valid caches are not passed through this
    # modularizer at all by run_game.bat.
    guest_abi_root = runtime / "guest_abi" / "v1"
    for name in ("dc_runtime.hpp", "dc_arm7.hpp", "dc_native_overrides.hpp"):
        src = guest_abi_root / name
        if not src.exists():
            die(f"[MODULAR] falta Guest ABI v1 congelado: {src}")
        copy_if_different(src, cpp / name)
    # Stable plugin ABI ships next to generated sources for diagnostics/dev builds.
    copy_if_different(runtime / "include" / "dcr_plugin_api.hpp", cpp / "dcr_plugin_api.hpp")

    registration = discover_registration(cpp)
    sources = discover_program_sources(cpp)
    emit_game_api(cpp, registration)
    patch_runner(cpp)
    patch_compile_test(cpp)
    emit_cmake(cpp, sources)
    
    import hashlib
    frozen = (guest_abi_root / "dc_runtime.hpp").read_text(encoding="utf-8", errors="replace")
    # Keep the marker human-readable; run_game validates the actual cached struct
    # with guest_abi_fingerprint.py so legacy cpp34/cpp35 ABI1 caches remain reusable.
    write(cpp / ".dcr_guest_abi", f"DCR_GUEST_ABI={ABI}\nDCR_GUEST_HEADER=FROZEN_V1\n")
    print(f"[MODULAR] guest ABI v{ABI} ready | registration={registration} | sources={len(sources)}")


if __name__ == "__main__":
    main()
