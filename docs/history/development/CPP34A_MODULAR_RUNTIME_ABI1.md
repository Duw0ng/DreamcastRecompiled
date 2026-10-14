# cpp34a — MODULAR_RUNTIME_ABI1

## Goal

Stop recompiling thousands of generated SH-4 functions for changes that only affect Dreamcast devices or host services.

## Build products on Windows

- `dreamcast_program.exe` — thin per-game runner.
- `dcr_game_module.dll` — persistent generated SH-4 + embedded guest image.
- `DreamcastRuntime.dll` — shared runtime and built-in compatibility device layer.
- `plugins\\Release\\dcr_network.dll` — replaceable network plugin.

## Cache policy

The launcher defaults to `%LOCALAPPDATA%\DreamcastRecompiled\games`. Each title keeps its generated C++ and build directory across DreamcastRecompiled zip/release folders.

A `.dcr_guest_abi` file marks the generated source as Guest ABI v1. If present, normal runs skip `dc_raw_recomp` entirely and only invoke an incremental CMake build.

## Fast loops

```bat
run_game.bat "ccr.cdi" --rebuild-network
run_game.bat "ccr.cdi" --rebuild-runtime
```

The first command only rebuilds `dcr_network.dll`. The second rebuilds the shared runtime/device binaries while preserving guest objects.

## One-time migration

cpp33 and older builds do not have a Guest ABI marker, so moving to cpp34 requires one full game recompile. After that, ABI-compatible cpp34+ runtime/plugin releases can reuse the same game module.

## Persistent system data

The launcher also moves console-wide state out of the release folder. By default it uses:

```text
%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_flash.bin
%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_vmu_a1.bin
```

This keeps the virtual Dreamcast ID, PlanetWeb/DreamKey ISP data and VMU intact when a new ZIP is extracted. Set `DCR_SYSTEM_DATA_ROOT` to override the destination. For a one-time import from an older cpp33 folder, set `DCR_IMPORT_SYSTEM_DIR` to that folder before launching cpp34.

## Safety

The frozen cpp33c modem implementation remains inside `DreamcastRuntime.dll` as a compatibility fallback. The external plugin is preferred when successfully loaded. Network development from cpp34 onward happens only in `dcr_network.dll`; the fallback is intentionally left untouched. A missing/corrupt network plugin therefore cannot prevent a title from booting.

## Validation

- Legacy/core suite: 54/54 PASS.
- Modular generated fixture: PASS.
- Dynamic plugin load: PASS (`kind=1`, `cpp34a-network-abi1`).
- Network-only incremental rebuild: plugin object + plugin shared library only; no generated guest object recompiled.
- Runtime-only incremental rebuild: runtime relink + dependent DLL/executable relinks; no generated guest object recompiled.
- Cross-release-path test: moving the byte-identical runtime source to another folder recompiles runtime sources only; **0 generated SH-4 objects** are recompiled.
- Persistent console-data path test: runner reads `DCR_SYSTEM_DATA_DIR` and redirects flash/VMU outside the release folder.
