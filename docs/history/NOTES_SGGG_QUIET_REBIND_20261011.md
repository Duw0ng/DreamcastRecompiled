# Dreamcast Recompiled v1.2 — SGGG SH-4 code relocation / quiet runtime

Baseline: `SGGG_PORTABLE_IPBIN_BOOT_FIX`, guest ABI v1 / `cpp38e` stable base.

## What changed

- The legacy `[DCR code-rebind]` and `[DCR code-reloc]` diagnostic lines were printed unconditionally on each copied-code match. This is now **OFF by default**. Optional flag: `--code-reloc-trace`, or **Diagnóstico > Traza de reubicación de código SH-4**.
- When tracing is enabled, the binding is printed only when its target→source correspondence is first established or changes. Other important runtime errors are not silenced.
- A runtime-local, 32-byte signature-validated memo prevents repeated O(number of registered SH-4 functions) template searches when a game frequently jumps into its copied IRQ handler at the same address. Every cache reuse checks the guest's current bytes and that the original compiled target still exists. A changed signature forces a fresh search.
- Both modular `runtime/dc_runtime.cpp` and the legacy emitted runtime in `src/codegen/cpp_emitter.cpp` were updated; **the frozen guest ABI1 was not changed**. The existing SH-4 game module and per-game limits remain untouched.
- Runtime stamp updated: `cpp38e-v8.5-portable-ipbin-rebind-cache-v1` so Windows will rebuild only the modified runtime and plugins as needed.

## Test in Windows

From the extracted root, using the same CDI and game cache as the prior SGGG successful boot:

```bat
run_game.bat "C:\ruta\SGGG.cdi" --reuse --no-portable-sync
```

Trace-on (only for investigating a new target mapping):

```bat
run_game.bat "C:\ruta\SGGG.cdi" --reuse --no-portable-sync --code-reloc-trace
```

No `--recompile-game` is necessary; do not delete the per-game cache. `--debug`, `--heartbeat-on`, `--profile` are separate controls. The regular runtime still logs serious faults and errors.

## Validation

- Linux: runtime plugin compiled, recompilation core compiled.
- Main C++ tests: 54/54 passed excluding the separately timed-out heavy `generated_program_build`; compilation of generated single-function program passed.
- Portable bootstrap regression: 1/1 passed (4 scenario assertions inside executable).
- New launcher contracts: 6/6 passed.
- Launcher suite under Xvfb: 121/124 passed; three previously existing contract tests still assert obsolete build strings/markers and are byte-identical to the previous package.
- **Windows/MSVC and actual SGGG FPS need a new test run.** No performance uplift percentage is claimed.
