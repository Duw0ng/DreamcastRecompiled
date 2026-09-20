# Validation — 0.0.159

## Scope

0.0.159 is a conservative correctness/diagnostic checkpoint built directly from 0.0.158. It deliberately does not change the normal PVR/TA, AICA/CDDA or commercial-runner performance defaults.

## SH-4 GPR cache hardening

- Multi-entry AOT functions still reload destination-only cached registers.
- The invariant now lives in the reload emitter (`source-read || writeback`) instead of altering source-read analysis. This makes the 0.0.157 failure mode harder to reintroduce through later liveness/source-use optimization.
- Added a generated-code regression around a static `CALL`: `gpc_flush(2u)` must precede the call and `gpc_reload()` must follow the barrier.
- `DCR_GPR_CACHE=0` remains the exact compile-time CPU-safe fallback.

## Diagnostics

- Missing recompiled/native target errors report target, guest PC, PR, SP/R15, SR, R0, R4 and R8.
- Existing rolling 32-call history remains unchanged.

## Commercial smoke helper

- New opt-in `--probe-controller-start-burst` overrides the high-level KOS controller status only for the smoke path.
- It waits for controller polling, emits six short START pulses with release gaps, then falls back to normal host input.
- Normal `run_commercial_recompiled.bat` is unchanged apart from the version label.

## Required validation

- Linux Release host build.
- Full CTest suite.
- Generated-code compile/runtime regression suite (`cpp_emitter_tests`).
- Package audit: no build trees, object files, temporary logs or commercial disc data.

Immediate experimental rollback: 0.0.156. Stable rollback: 0.0.131.

## Validation result — 2026-09-05

- Linux Release host build: **PASS**.
- CTest: **50/50 PASS**.
- Generated C++ project configure/build: **PASS**.
- `generated_compile_test`: **PASS**.
- Generated literal-pool runner execution: **PASS** and reports `DreamcastRecomp 0.0.159 native runner`.
- Generated runner accepts `--probe-controller-start-burst`: **PASS**.
