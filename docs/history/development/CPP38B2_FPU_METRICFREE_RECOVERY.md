# cpp38b2 — FPU Metric-Free Recovery

cpp38b2 isolates the next historical-performance variable after the live cpp38b1 log proved that `DCR_FPU_METRICS=0` was not actually disabling Superblock telemetry. The old macro implicitly enabled FPU counters for every mode except RegionPlus, so Superblock still executed millions of counter updates.

## Changes

- Keeps cpp38b1 `indexscratch`; cpp38b `mapindex` remains reverted.
- Keeps `DCR_FPU_MODE=superblock` and `DCR_HOST_SYNC_QUANTUM=262144` defaults.
- Makes FPU telemetry independent from the FPU execution mode.
- Modular guest builds define `DCR_FIXED_FPU_REGION_METRICS=0`, allowing the compiler to remove generated FPU metric updates from the hot path.
- Adds metric helper macros to generated code and wraps scalar, Region and Superblock counters.
- Heartbeat now reports the real telemetry switch rather than inferring it from the selected FPU mode.
- Guest ABI v1 structure is unchanged.

## One-time cached guest migration

This optimization changes generated C++ implementation but not the SH-4 closure or Guest ABI layout. Therefore cpp38b2 can migrate an existing valid cache in place:

1. patch the existing `generated/cpp` sources;
2. add the fixed-off compile definition;
3. rebuild `dcr_game_module.dll`/runner once;
4. write `.dcr_guest_impl_revision=cpp38b2-metricfree-v1`.

No SH-4 discovery/reanalysis is performed during this migration. Subsequent `--reuse` runs do not rebuild the guest.

The historical SAFE seed is intentionally not auto-adopted in cpp38b2 because that DLL was compiled before the metric-free generated implementation existed.

## Expected log

```text
build=v1.2-cpp38b2-fpu-metricfree-recovery
fpu-mode=superblock
fpu-metrics=off
host-sync-q=262144
pvr38=on/cpp38b2/indexscratch/capture1copy/ch2bulk/simdtag/homstrip
```

With the production guest, FPU telemetry counters should remain zero (or stop changing if a previously initialized runtime value exists) while Superblock behavior remains active.
