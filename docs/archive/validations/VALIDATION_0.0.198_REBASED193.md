# DreamcastRecomp 0.0.199_rebased193 validation

## Intent

Preserve the 0.0.197 compatibility baseline while fixing profiler phase aliasing and reducing redundant per-basic-block host stores. No SH-4 tick-batch, guest clock, IRQ, PVR timing, or AICA timing change is made.

## Changes

1. Full-tick profiler uses a decrementing countdown and defaults to prime stride 67. The previous stride 64 aligned with the 1024-full-tick host synchronization cadence.
2. CT2 enables `DCR_LIGHTWEIGHT_CURRENT_PC=1`. Generated blocks continue to set `ctx.pc`; `runtime.current_pc` is synchronized at full scheduler boundaries instead of every block. Dynamic dispatch and memory-fault reports take their caller/fault PC from the live SH-4 context.
3. All 0.0.197 compatibility fixes and performance fast paths remain enabled, including batch 256.

## Validation

- Main CMake Release/Ninja build completed successfully.
- CTest: 51/51 passed.
- CT2 `dc_runtime.cpp`, `generated_program.cpp`, support units and recovered callbacks compiled in the commercial CMake build before the environment time slice reached the large program shards.
- `generated_program_part_17.cpp` (contains the main 0x8C080Cxx/0x8C080Dxx geometry HOTPC region) compiled successfully with all CT2 fixed compile definitions including `DCR_LIGHTWEIGHT_CURRENT_PC=1`.
- `generated_runner.cpp` compiled successfully with the same CT2 definitions.

## Benchmark guidance

Run `run_crazy_taxi_2_perf_profile.bat`; it now passes `--perf-sample-stride 67`. Treat this as the first reliable de-aliased component-budget baseline.
