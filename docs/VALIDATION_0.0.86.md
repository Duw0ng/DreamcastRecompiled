# Validation — DreamcastRecomp 0.0.87

## Goal

Attack the measured 9-11 FPS software-PVR ceiling with frame-level multicore rasterization while retaining the 0.0.83 Store Queue/FMOV64 correctness fix and 0.0.84 closure fix.

## Changes validated

- Normal commercial runner enables `--pvr-mt`.
- Non-background normal triangles are queued with immutable `PVRSoftState` plus shared decoded-texture ownership.
- Opaque/punch triangles keep submission order.
- Translucent list 2 keeps stable inverse-depth sorting.
- Horizontal framebuffer bands are disjoint, allowing worker threads to update color/depth without pixel races while preserving primitive order inside each band.
- Diagnostic/profile/probe modes fall back to the legacy single-thread path.
- A same-version `run_commercial_recompiled_singlethread.bat` is included for direct A/B testing.
- MSVC `/Ob3` was removed; Release uses CMake's normal `/Ob2` plus `/O2 /Oi /Ot`, eliminating warning D9025 from the generated project.
- Heartbeat includes `pvr-mt=enabled/workers/frames/cumulative-ms`.

## Regression suite

```text
47/47 CTest PASS
```

## Synthetic host-side raster check

A small 640x480 textured/bilinear workload using the generated runtime was executed both ways on the validation host. The row-band MT path was about 2.9x faster than the same single-thread raster path. This is only a host-side synthetic check, not a prediction of ChuChu Rocket FPS on Windows; the included single-thread BAT is the intended A/B control for the real title.

## Commercial closure

Fresh generation from the supplied ChuChu Rocket image remains:

```text
Reachable functions:    2965
Reachable instructions: 295215
Known SH-4:             295215
Unknown SH-4:           0
RAW_SH4:                0
Branch-selected calls:  1
```

Generated `dc_runtime.cpp` compiles successfully with GCC `-O3`, including the multithread band renderer. `generated_runner.cpp` passes syntax validation. Full optimization of the giant `generated_program.cpp` exceeds the container validation timeout; the generated SH-4 closure itself is unchanged by this PVR host-side change.

## Windows verification

Run the normal BAT first and inspect `fps=` plus `pvr-mt=`. If graphics differ, immediately compare with `run_commercial_recompiled_singlethread.bat` from the same archive. The profile BAT intentionally disables the MT fast path so detailed historical counters remain comparable.
