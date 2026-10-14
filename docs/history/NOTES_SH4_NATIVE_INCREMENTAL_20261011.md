# Dreamcast Recompiled v1.2 — SH-4 Native Incremental (seed patches v1)

**Base:** `SH4_INTERP_FALLBACK`, itself based on `SGGG_QUIET_REBIND_CACHE_FIX` and cpp38e / Guest ABI1.

## Goal

Promote previously learned static SH-4 interpreter seeds to native code **without recompiling the existing AOT guest** (`dcr_game_module.dll`). Only the *new* functions compile in small independent `dcr_sh4_patch_<hash>.dll` modules. The game runner loads compatible modules before dispatch, keeping its original AOT function pointers intact.

## Pipeline

1. The interpreter records previously unknown, *verified original-image* SH-4 entrypoints in `<game cache>/interp_seeds.txt`. A `# runtime-code` line refers to modified/overlaid guest memory and deliberately is **not** compiled from the static bootstrap.
2. On the **next** `run_game.bat` startup (or "Preparar SH-4"), after the base ABI1 guest cache is validated, `tools/incremental/build_native_patches.py` compares active seeds with the original closure's `function_map.csv` and with its existing patch manifests.
3. Only missing seeds go to `dc_incremental_recomp`, which analyzes and emits the requested functions (no complete retail discovery pass). Native C++ may call the regular `call_recompiled` dispatcher for functions outside its seed batch.
4. MSVC builds small DLLs in a **separate staging build tree** under `<game cache>/incremental/staging`. A successful module is published in `<game cache>/incremental/native`. Original `generated/cpp`, `build/Release/dcr_game_module.dll`, and their `.obj` files remain untouched.
5. On game launch, `DreamcastRuntime.dll` checks the incremental module ABI and the exact SHA-256 of the staged `BOOTSTRAP.BIN` before loading and registering new functions. Existing AOT addresses take precedence over patch addresses. Changed code signatures skip stale incremental AOT and fall back to interpreter/rebind.
6. A rejected batch is divided in half automatically. A repeatedly failing **individual** seed is quarantined per hash/tool version and continues using the interpreter. Future tool versions can try it again.

## Usage on Windows

Normal (auto-promote seeds; remains compatible with `--reuse`):

```bat
run_game.bat "C:\games\game.cdi" --reuse --no-portable-sync
```

Disable incremental compilation while retaining SH-4 interpreter:

```bat
run_game.bat "C:\games\game.cdi" --reuse --no-native-seeds --no-portable-sync
```

To suppress **both** incremental compilation and loading cached patch DLLs, use `--no-native-patches`. `--no-native-seeds` only disables generating **new** patch DLLs.
Disable interpreter itself: `--no-interp` (may cause a missing native target to abort).

Expected console output:

```text
[NATIVE SEEDS] recorded=3 base=4102 native-cache=1 pending=2
[INCREMENTAL SH4] native-only seeds=2 instructions=... sources=1
[NATIVE SEEDS] published dcr_sh4_patch_....dll: 2 seeds
[DCR SH4-INCREMENTAL] loaded dcr_sh4_patch_....dll new-native-blocks=...
```

**Initial compilation:** the *first-ever* play of a game still needs the original whole-game AOT bootstrap. Incremental compilation requires a valid, existing Guest ABI1 cache. An explicit `--recompile-game`, `--clean`, or incompatible ABI/cache necessarily triggers the historical rebuild path.

**Native patches are not JIT:** they are generated at launch, using newly recorded seeds from previous runs. Newly discovered entrypoints continue interpreted until the next launch. No compiler is invoked inside the game process.

## Cache / safety

- The source image is **never** copied into patch DLLs; generated guest code is built with `embed_raw_image=false` and remains compatible with external bootstrap delivery.
- Patches are namespaced by game cache and full SHA-256 of its staged bootstrap; stale modules are rejected at load time.
- Guest ABI1 `DCRuntime`/`SH4Context` layout and the original `SGGG_QUIET_REBIND_CACHE_FIX` are not modified by the code generator.
- Only **static verified entries** are eligible; dynamically loaded overlays with different bytes stay in interpreter mode until a safe overlay-aware generator is implemented.
- If MSVC cannot compile one seed, the base guest remains usable.
- The portable synchronizer also includes native patch DLLs; the updated Go portable launcher computes the staged image SHA-256 and passes the native-patch directory to runtime.

## Validation performed in this environment

- `dc_incremental_recomp` builds successfully using GCC/CMake on Linux.
- `DreamcastRuntime` builds with a patch-DLL loader without modifying Guest ABI1.
- A small synthetic SH-4 bootstrap generated C++, built it as a separate `.so`, and successfully registered new code in `DreamcastRuntime` without clobbering a simulated existing AOT address.
- Changing a byte of a registered native patch was detected by `dc_registered_static_code_overwritten()`.
- `dcr_sh4_interp_tests`: passed (271 assertions in source suite).
- 5 incremental orchestration tests: passed.
- Portable launcher builds for `windows/amd64` using Go.

**Unverified:** MSVC DLL link/test on Windows; real CDI gameplay; FPS and overlays under live game load. These must be validated on the target PC. A successful Linux test is not a Windows compatibility guarantee.
