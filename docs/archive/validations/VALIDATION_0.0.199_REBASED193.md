# Validation — DreamcastRecomp 0.0.199_rebased193

## Lineage and timing
- Parent: 0.0.198_rebased193.
- CT2 SH-4 batch remains 256 cycles.
- Guest SPG/PVR/IRQ timing is unchanged.
- Periodic host AICA synchronization is coalesced from 262144 to 524288 SH-4 cycles, but explicit Present/sleep syncs remain and re-arm the periodic deadline.
- ARM7 catch-up budget scales by the host-sync quantum so maximum executable firmware work per unit time is not reduced.

## Host UI
- Guest-cycle UI gate: 2,000,000 cycles (10 ms @ 200 MHz).
- Wall-clock UI cap: 10,000,000 ns (~100 Hz).

## Automated validation
- Main CMake Release build completed.
- CTest: 51/51 passed.
- Commercial CT2 objects compiled with the same fixed performance definitions used by CMake:
  - dc_runtime.cpp
  - generated_runner.cpp
  - generated_program_part_17.cpp
  - generated_sub_8C06C41C.cpp
- Dedicated deadline test:
  - initial host-sync quantum = 524288 cycles
  - initial deadline = 524288
  - explicit sync at guest cycle 123456 re-armed deadline to 647744
  - repeated explicit sync at the same guest cycle did not leave a stale/past periodic deadline

## Test request
Run `run_crazy_taxi_2_perf_profile.bat` through the same attract/demo path and compare wall-ms/frame, ui, host-sync, device, audio starvation/underrun counters and host-sync count against 0.0.198.
