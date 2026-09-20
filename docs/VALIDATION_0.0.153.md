# DreamcastRecomp 0.0.153 — validation

0.0.153 combines the two correlated post-0.0.152 experiments into one test build: TA finite-coordinate acceptance and a guarded AOT FPU register cache. The 0.0.152 strip-GPU path is retained unchanged.

## TA acceptance

The runtime still classifies finite/extreme coordinates with the existing IEEE bit test, but `plausible` now means **finite**. Large finite X/Y/Z values increment `pvr_large_finite_vertices_accepted`; only NaN/Inf increment BAD-VTX. The software rasterizer already rejects fully off-screen triangles and clamps min/max X/Y in float before integer conversion.

The generated TA staging self-test submits a four-vertex strip containing two coordinates near `(700000, -1000000)` and requires:

- `pvr_vertices == 4`
- `pvr_bad_vertices == 0`
- at least two large finite vertices accepted
- one completed strip
- zero short strips

## FPU register cache

The emitter finds cache-safe regions separated by FPSCR mutations, dynamic calls/branches and unsupported paths. A cache region is emitted only when it contains at least one hot operation (FTRV/FIPR/FMAC/FSRRA) and at least two FPU operations. Runtime entry requires `PR=0`, `SZ=0`, `RM=0`; otherwise the previous path runs unchanged.

Per-region analysis tracks first-use liveness so a register written before its first read is not loaded from `ctx`. Dirty FR/XF lanes are written back at boundaries and before control-flow exits.

## Results

- Main Release build: PASS.
- Project CTest suite: PASS.
- Generated FPU code contains live `fc_fr*` locals plus explicit fallback.
- Fresh generated runtime compile/self-test: PASS / exit 0.
- FPU A/B: same generated sample built once normally and once with the cache guard forced false. Both return `R0=42 | FR0=2 | SR=0x1 | PC=0xFFFFFFFF`.

Windows/D3D11 performance still requires the supplied ChuChu Rocket! full-session test. Inspect `fps`, `pvr-xaccept`, `pvr-badv`, `pvr-shortbad`, `pvr-stripgpu`, `fpu-cache`, `fpu3d`, `pvr-cpu-est-ms` and `pvr-gcpu-ms`.
