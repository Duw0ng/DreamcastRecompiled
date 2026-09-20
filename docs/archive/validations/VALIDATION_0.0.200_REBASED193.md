# Validation — DreamcastRecomp 0.0.200_rebased193

## Lineage and timing
- Parent: 0.0.199_rebased193.
- CT2 SH-4 batch remains exactly 256 cycles.
- Host-sync quantum remains 524288 SH-4 cycles.
- UI cadence remains 10 ms and profiler stride remains 67.
- Guest SPG/PVR/IRQ timing is unchanged.

## Gameplay fault targeted
The 0.0.199 mixed gameplay+demo run reproduced the historical CT2 failure at guest PC `0x8C080CD8` with `R1=0x58AC43B5`, `R4=0x0C7FF090`, `R14=0x0C7FF060`, and `FPSCR=0x140000`. The invalid pointer was already present before the failing `FMOV @R1+` read, so the memory mapper was not treated as the root cause.

The original SH-4 stream around `0x8C080C96–0x8C080CE0` was decoded and the branch/delay-slot lowering was checked. An initial concern that `FSCHG` might let the persistent FPU superblock cross an SZ mode transition was ruled out: `sfc_flush` already clears `sfc_active` before that transition.

## FPSCR.DN parity fix
The crash occurs with `FPSCR.DN=1`. Prior DreamcastRecomp builds did not synchronize DN with the host floating-point environment. Flycast does: DN=1 enables host denormal flush-to-zero and DN=0 preserves denormals.

0.0.200 adds `dc_sync_host_fpu_mode()` / `dc_write_fpscr()` and routes full FPSCR writes through them, including:
- `LDS Rm,FPSCR`
- `LDS.L @Rm+,FPSCR`
- runtime reset/default FPSCR
- generated-runner FPSCR override

On MSVC this uses `_controlfp(..., _MCW_DN)`; on SSE2 hosts it updates MXCSR FTZ. Existing RM handling is intentionally left unchanged.

## FPU fast-path hardening
FPU cached/superblock paths now require the current PR/SZ/RM mode to remain eligible. A previously active cache no longer bypasses a mode change. This is defensive generic correctness hardening in addition to the DN fix.

## Validation performed
- Main CMake build completed incrementally after the large translation unit exceeded one single execution window.
- CTest: 51/51 passed.
- Dedicated host-FPU smoke test against the actual CT2 runtime: `DN=0` preserved a subnormal result; `DN=1` flushed the same operation to zero (`DN_SMOKE_OK`).
- Selected CT2 commercial compile succeeded with the same fixed definitions used by the Windows package for:
  - `dc_runtime.cpp`
  - `generated_runner.cpp`
  - hot `generated_program_part_17.cpp`
  - FPSCR-heavy `generated_program_part_24.cpp`
  - FPSCR-stack-load `generated_program_part_30.cpp`
  - `generated_sub_8C06C41C.cpp`

## Performance claim
No Windows performance gain is claimed yet. 0.0.200 is primarily a correctness build for the gameplay route that exposed the historical fault. A new mixed gameplay+demo log is required before deciding whether to extend optimized FPU handling in the SZ=1 geometry path.
