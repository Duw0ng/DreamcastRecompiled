# VALIDATION 0.0.116

## Baseline

Compare against **0.0.115** in the same ChuChu Rocket gameplay sequence. 0.0.115 live reference: normal gameplay ~55-60 FPS, peak ~63-66 FPS, maximum rat-wave minimum ~35-36 FPS.

0.0.116 must keep the 0.0.115 texture-cache/run batching changes. TA vertex fast-path and generated PREF-inline from 0.0.113 remain removed.

## What changed

1. FSRRA resolves FPSCR and the active FR bank locally while preserving exact `1.0f / std::sqrt(x)` output.
2. FMOV64 pair helpers access the selected FR/XF pair directly.
3. 64-bit RAM/SQ common paths use one 8-byte copy.
4. D3D11 frame vertex/run vectors retain capacity across frames.
5. Redundant constant-buffer and pipeline-state binds are skipped. `pvr-gbind=C/B/D/S/T` reports actual constant/blend/depth/sampler/SRV updates.

## Windows live test

Run `run_commercial_recompiled_perf.bat "...\\ChuChu Rocket!.cdi"` and capture:

- normal gameplay FPS,
- peak FPS,
- absolute minimum at the instant the maximum rat wave is fully visible,
- 3-5 consecutive heartbeats covering maximum load -> recovery.

Check `fps=`, `fpu3d=`, `fmov64=`, `pvr-gpu=`, `pvr-gbind=`, `pvr-gtex=`, `pvr-tcache=`, `pvr-tfast=` and `audio-starves=`.

Regression criteria: no visual/audio/timing regression and no material drop below the 0.0.115 reference under an equivalent scene.
