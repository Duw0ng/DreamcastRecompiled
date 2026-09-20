# DreamcastRecomp 0.0.135 validation

Status: experimental. Stable rollback remains 0.0.131. 0.0.134 remains the previous experimental checkpoint.

## Change

0.0.135 adds conservative generated-code recognition for SH-4 Store Queue TA producer sites. A PREF is specialized only when the same address register has an explicit Store/FMOV producer within the current basic block. The generated call `dc_pref_ta_native()` still validates the live Store Queue aperture and QACR-derived TA FIFO target; any mismatch falls back to `dc_pref()`.

The hit path preserves the existing PREF/SQ/TA counters and submits the same 32-byte Store Queue payload through `pvr_submit_packet()`.

## Diagnostics

- `pvr-ta-fast=vtx7/le32/src4/v64cache/specdec/nativepref`
- `pvr-ta-native=<native TA hits>/<guard fallbacks>`

## Validation

- Main project Release build: PASS, strict serial (`--parallel 1`).
- CTest: 49/49 PASS.
- Emitter test builds a synthetic `Store32 -> PREF` block and verifies that it emits `dc_pref_ta_native()`.
- Fresh generated runtime: PASS.
- `generated_compile_test`: RC=0, including native TA PREF hit/fallback checks.
- Fresh generated `dreamcast_program`: RC=0 and returns normally to host.
- Generated CMake: no active MSVC `/MP`; serial/low-memory policy retained.
