# DreamcastRecomp 0.0.112 — validation

## Scope

Performance-only change over 0.0.111. The Windows low-memory serial/incremental build remains unchanged. CDDA, PVR semantics, timing, Maple and VMU behavior are unchanged.

## Profile basis

Heavy ChuChu Rocket gameplay heartbeats around 30 FPS showed approximately per second:

- ~536k FTRV
- ~533k FIPR
- ~1.04M FMAC
- ~4.24M Store Queue writes
- ~509k PREF->TA commits

0.0.112 targets the first three operations because 0.0.111 still crossed out-of-line FR/XF helper boundaries many times per instruction.

## Changes

- FTRV resolves FR/XF physical banks once and performs raw-bit float conversion locally.
- FIPR resolves the FR bank once and preserves the previous per-lane float rounding sequence.
- FMAC resolves FR once and retains std::fma semantics.
- FPSCR.RM and FPSCR.PR unsupported-mode traps remain explicit.
- No general-purpose helper is force-inlined into dc_runtime.hpp.

## Commercial closure

- Registered functions: 2965
- Known SH-4 instructions: 295215
- RAW_SH4: 0
- Shards: 32
- Static FTRV sites: 165
- Static FIPR sites: 83
- Static FMAC sites: 1332

Generated shard source grew from 54,490,082 bytes (0.0.111) to 54,857,268 bytes (0.0.112), ~0.67%, while the runtime header strategy remains the low-memory 0.0.111 design.

## Tests

- 47/47 CTest PASS.
- 200,000 randomized FPU state comparisons: old helper path vs new specialized FMAC/FIPR/FTRV, exact FR/XF bit equality.
- Commercial generated shards 00..03 and part_30 compiled successfully with C++20/O2.
- Commercial runtime compiled successfully.
