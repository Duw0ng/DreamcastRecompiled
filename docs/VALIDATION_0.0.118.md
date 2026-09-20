# DreamcastRecomp 0.0.118 — validation

## Goal

Optimize the CPU-side deferred PVR path on top of 0.0.117 without restoring the rejected 0.0.113 TA/PREF shortcut and without changing guest rendering semantics.

## Changes under test

- Event-driven deferred-state invalidation on VRAM/palette/stride mutation; same-surface reuse no longer compares two epochs per triangle.
- GPU run descriptors reference `PVRDeferredState` by index instead of copying `PVRSoftState` and a texture `shared_ptr` per run.
- Translucent stable-sort depth sum cached once per queued triangle.
- Sampled `pvr-cpu-est-ms=TA/STATE/GEOM/GPU@SAMPLES` telemetry piggybacks on the existing performance stride. STATE/GEOM are nested portions of TA.

## Live acceptance target

Use the same ChuChu Rocket heavy-rat scene used for 0.0.115-0.0.117. Preserve image/audio/input behavior and compare minimum/normal/peak FPS. Also capture `pvr-dstate`, `pvr-cpu-est-ms`, `pvr-gbind`, `pvr-gpresent`, `pvr-gpu`, `sq-writes`, `pref-ta`, and `audio-starves`.

A flat FPS result is still useful: the new sampled breakdown should identify whether TA parsing/state/geometry or GPU submission deserves the next version.

## Local regression result

- Core CTest: **47/47 PASS**.
- Fresh generated sample runtime: CMake Release build PASS.
- `generated_compile_test`: exit 0.
- Source-package hygiene: no CDI, IP.BIN, 1ST_READ.BIN or locally generated commercial C++ is included.

Windows live FPS/D3D11 validation remains the acceptance test to be performed with the user's existing ChuChu Rocket CDI.
