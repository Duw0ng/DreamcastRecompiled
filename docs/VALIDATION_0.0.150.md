# DreamcastRecomp 0.0.150 validation

## Scope

0.0.150 keeps the 0.0.149 PVR/TA path unchanged and targets the next measured SH-4 hotspot from the full-session Mouse Mania log.

- FTRV: direct four-row matrix/vector dot products, `double` accumulation, one final `float` writeback per lane, no temporary `v[4]`, `m[16]`, `o[4]` arrays or nested emitted loops.
- FIPR: direct four-product dot product with `double` accumulation and final `float` writeback.
- FPSCR.PR/RM checks and FR/XF bank selection remain unchanged.
- FSRRA, FMAC, SQ/PREF, TA ordering, PVR, AICA/CDDA, GD-ROM and Maple are unchanged.
- Generated native runners automatically tee stdout/stderr-style iostream output to `logs/DreamcastRecomp_0.0.150_session_YYYYMMDD-HHMMSS.log` while keeping the live console visible.

## Flycast audit basis

Compared against current Flycast SH-4 sources during development:

- `core/hw/sh4/interpr/sh4_fpu.cpp`: FTRV and FIPR use widened accumulation before final single-precision writeback in the reference interpreter.
- `core/hw/sh4/dyna/shil_canonical.h`: FTRV/FIPR are represented as vector inner-product operations in the dynamic recompiler path; FSRRA remains `1/sqrtf` and FMAC remains fused multiply-add semantics.

0.0.150 therefore does not introduce approximate reciprocal-square-root or relaxed vector math.

## Validation results

- Release core build: PASS.
- CTest: **50/50 PASS**.
- `fpu_unary_tests`: PASS with structural checks for direct double-accumulated FTRV/FIPR and absence of old temporary arrays.
- Fresh generated runtime configure/build: PASS.
- Generated `generated_compile_test`: RC=0.
- Generated FPU unary `dreamcast_program`: RC=0, R0=42, PC=0xFFFFFFFF.
- Automatic session logger: PASS; generated runner created a timestamped `logs/DreamcastRecomp_0.0.150_session_*.log` whose contents matched the live console output.

## Rollback

- Immediate experimental rollback: **0.0.149**.
- Accepted stable rollback: **0.0.131**.
