# DreamcastRecomp 0.0.156 — validation

## FPS-first changes

- Same-block complete 32-byte Store Queue producers can be captured and submitted directly to TA at their specialized `PREF`.
- Runtime guards validate P4 SQ aperture, queue identity, QACR TA mapping, exact 32-byte coverage, non-overlap and width/alignment. Failed guards replay captured stores and execute the previous native-PREF path.
- Natural backward CFG edges are compiled as adaptive hot-trace candidates. Promotion preserves the existing 256-cycle scheduler quantum; only redundant PC/current-PC stores are elided below the full-tick boundary.
- Static FPU superblocks integrate with the hot-trace scheduler-safe tick path.
- `DCR_HOT_TRACE=0` disables hot-trace promotion in the already-compiled executable.

## Required validation gates

- Release project build + complete CTest suite.
- Positive and negative SQ-fusion emitter probes.
- Generated hot-trace and fused-SQ source compiled as real C++ objects.
- Fresh generated runtime build and `generated_compile_test` RC=0.
- Region/superblock generated runner final architectural state comparison.

## Windows test fields

Compare `fps=`, `hot-trace=`, `sq-fuse=`, `fpu-mode=`, `fpu-super=`, `pvr-ta-stage-ms=`, `pvr-cpu-est-ms=` and `pvr-gcpu-ms=`. `sq-fuse` detailed hit/capture counts require `--perf-profile`; the normal runner avoids that per-packet counter overhead.
## Final clean validation (2026-09-01)

- Fresh Linux Release configure/build from an empty `build156`: PASS.
- Complete CTest suite: **50/50 PASS**.
- Fresh 0.0.156 generated project from `samples/sh4_fpu_arith.elf`: compile/link PASS.
- Fresh `generated_compile_test`: **RC=0**, including an actual call to `dc_pref_ta_fused_packet` with valid QACR/SQ coverage.
- Generated runner A/B: `DCR_FPU_MODE=region,DCR_HOT_TRACE=0` and `DCR_FPU_MODE=superblock,DCR_HOT_TRACE=1` both finish with `R0=42`, `FR0=2`, `SR=0x1`, `GBR=0x0`, `PC=0xFFFFFFFF`.
- `cpp_emitter_tests` covers positive complete-32-byte SQ fusion, barrier rejection, and natural-loop hot-trace emission.

