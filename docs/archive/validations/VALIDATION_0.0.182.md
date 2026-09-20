# DreamcastRecomp 0.0.182 — CT2 performance unlock

## Runtime evidence from 0.0.181

The first confirmed fully in-game Crazy Taxi 2 run is functionally stable but the experimental launcher accidentally leaves every PVR acceleration path disabled for the whole session:

- `pvr-fast=0/0`
- `pvr-gpu=off`
- `pvr-mt=off`
- `pvr-gpresent=off`
- `direct=0/...`
- `sh4tick=4096/...`
- `audio-clock=arm/...` / `host-syncs=0`

During heavy gameplay the software-only renderer falls to roughly 1–2 FPS while processing thousands of triangles per rendered frame. This makes PVR CPU raster the first-order bottleneck before any new TA/SH-4 optimization.

## 0.0.182 change

No compatibility/AOT semantics from 0.0.181 are removed. The normal CT2 launcher now restores the already-developed commercial performance path:

- `--pvr-gpu`: D3D11 raster path.
- `--pvr-mt`: multithreaded software fallback.
- `--direct-dispatch`: direct SH-4 target dispatch in addition to fast/inline dispatch.
- `--sh4-tick-batch 256`: established commercial timing quantum.
- `--device-clock-host --device-clock-host-max-catchup 4096`: established host-paced device clock policy.
- Keeps frame sync, host input, ARM7 and live audio.

`run_crazy_taxi_2_software.bat` preserves the old 0.0.181 launch policy for a clean A/B visual/compatibility rollback.

## Acceptance target

A successful Windows run should show `pvr-fast=1/1`, `pvr-gpu=on/ready`, advancing GPU bind/draw/present counters and `pvr-gpresent=direct`. If GPU initialization or frame eligibility fails, `--pvr-mt` should provide the fallback instead of the previous single-threaded software path.

This milestone is deliberately a configuration/performance-path correction, not a speculative renderer rewrite. Further TA/SH-4 optimization should be based on the 0.0.182 GPU-enabled log.
