# DreamcastRecomp 0.0.191 validation

## Focus
- Keep the 0.0.186 cycle-driven SPG implementation.
- Add an opt-in, guest-cycle scheduled PVR render-complete path modeled on Flycast's Dreamcast timing.
- `--pvr-render-done-scheduled` delays only the hardware-visible Holly render-done events; host GPU/CPU raster work remains synchronous.
- Delay formula: `min(450000 + TA_bytes * 100, 1500000)` SH-4 cycles.
- Heartbeat telemetry: `pvr-rdone=MODE/SCHEDULED/FIRED/PENDING/LAST_DELAY/LAST_TA_BYTES/LATE_CYCLES`.
- CT2 normal/perf launchers enable the scheduled path. `run_crazy_taxi_2_perf_immediate.bat` is the A/B control.

## Validation
- Core build: PASS.
- CTest: 51/51 PASS.
- Generated CT2 runtime: full compile/link PASS.
- `generated_compile_test`: PASS (RC=0).
- No game/disc image is included.
