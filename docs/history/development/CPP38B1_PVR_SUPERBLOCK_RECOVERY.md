# cpp38b1 — PVR Superblock Recovery

cpp38b1 is a regression-recovery checkpoint built from cpp38b after live Windows logs showed that cpp38b's direct construction of the R32 index stream inside a `D3D11_MAP_WRITE_DISCARD` mapping reduced FPS under nearly identical PVR workload.

## Changes

- Revert the cpp38b mapped-index builder.
- Restore cpp38a's CPU `index_scratch` generation followed by one bulk upload into the mapped D3D11 index buffer.
- Keep cpp38a's SQ one-copy capture, CH2→TA bulk, CH2→VRAM bulk, SIMD strip scan and homogeneous-source strip path.
- Restore static FPU `superblock` as the default runtime mode.
- Keep `DCR_FPU_METRICS=0` by default so hot FPU counters do not add diagnostic writes during normal benchmarking.
- Keep the 262144-cycle host/AICA synchronization cadence.
- Preserve runtime A/B switches without rebuilding the SH-4 guest: `--fpu-regionplus`, `--fpu-metrics`, `--sync-524`.
- Guest ABI v1 remains frozen and the pinned ChuChu SAFE guest is unchanged.

## Recommended live test

```bat
TEST_CPP38B1_FAST.bat "C:\path\ccr.cdi"
```

Expected heartbeat markers:

```text
build=v1.2-cpp38b1-pvr-superblock-recovery
fpu-mode=superblock
fpu-metrics=off
host-sync-q=262144
pvr38=on/cpp38b1/indexscratch/capture1copy/ch2bulk/simdtag/homstrip
```

The first few heartbeats may show `fpu-super=0/...` before the game reaches FPU-heavy code. Later gameplay should show non-zero superblock counters.
