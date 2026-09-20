# Validation 0.0.191

## Goals
1. Keep the 0.0.190 GPU/depth and 0.0.189 STARTRENDER gains intact.
2. Make performance profiling use the production Type-7/8 TA bulk path.
3. Add SH-4 MMU/UTLB semantics for P0/P3 virtual addresses without penalizing MMU-off retail code.
4. Turn the CT2 0x8C080CAA invalid access into an actionable pointer-provenance diagnostic instead of masking it.

## SH-4 / MMU
- `MMUCR` at 0xFF000010 is now retained explicitly.
- UTLB address/data arrays at 0xF6000000 / 0xF7000000 / 0xF7800000 are retained as 64-entry SH-4 state.
- P0/U0 and P3 accesses consult valid UTLB entries only while MMUCR.AT is set.
- P1/P2 remain direct and P4 remains on-chip.
- MMUCR.TI invalidates UTLB state and self-clears in the runtime model.
- Heartbeat `sh4-mmu=` reports AT state, successful translations and misses.

## Crash diagnostics
- Invalid memory accesses print the complete SH-4 integer register file plus PR, SR, GBR, FPUL and FPSCR.
- CT2 experimental runtime recognizes the known 0x8C080CAA FMOV fault and logs whether the pointer came from the direct or relative record path plus nine source words.
- No loose/wrapping address fallback is enabled by default: an actually corrupt pointer still fails loudly.

## TA profiling
- Type-7/8 complete strips remain on the contiguous bulk append path with `--perf-profile`.
- `pvr-ta-stage-ms` continues timing the complete captured-stream parse. Fine per-packet TA sampling is intentionally less intrusive.

## Validation
- Core build completed with single-worker build.
- 51/51 CTest tests pass.
- Experimental CT2 `dc_runtime.cpp` compiles standalone with C++20.
- No `DCRuntime` layout fields were added, so generated SH-4 shard ABI remains unchanged from 0.0.190.
