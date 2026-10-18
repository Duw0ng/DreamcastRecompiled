# Dreamcast Recompiled v1.2 — Project Layout

Critical files and directories (`run_game.bat`, `CMakeLists.txt`, `runtime/`, `src/`, `include/`, `safe_guests/`, and `portable/`) remain at their established locations. Historical scripts were relocated with their working directories and batch-file calls adjusted so they still resolve against the project root.

## Scripts

- `scripts/stable/TEST_CPP38E_FAST.bat` — fast stable runner. The recommended original root entry point is `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`.
- `scripts/stable/TEST_CPP38E_HOTPC_DIAG.bat` — hotspot diagnostics; profiling can reduce FPS.
- `scripts/stable/APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat` — first-use guest specialization, invoked internally by `run_game.bat`.
- `scripts/tests/` — historical cpp34–cpp38 tests, retained with their original options.
- `scripts/patches/` — legacy patches; avoid applying them to a stable cache unless you understand their effects.
- `scripts/maintenance/` — exceptional repair/restore utilities. Some clear runtime caches but not guest modules.
- `scripts/controller/` — controller tests.
- `scripts/portable/` — ChuChu portable startup.

## Verification tools

Run `python tools/verification/legacy/VERIFY_CPP38E.py` from any directory to check cpp38e code and invariants. Run `python tools/verification/check_layout.py` to verify this distribution's batch-file paths.

## Compatibility

No historical test script is launched automatically by the stable launcher. The internal `tools/` locations, game profiles, ABI1 guest layout, runtime, build configuration, and per-user cache location are unchanged.
