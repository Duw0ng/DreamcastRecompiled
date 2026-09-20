# DreamcastRecomp 0.0.126 — validation

## Regression fixed

The supplied 0.0.125 live trace showed the PVR backend enabled but never entering the GPU path:

- `pvr-gpu=on/wait/0/...`
- `pvr-gpresent=direct/0/...`
- `pvr-gcpu-ms=0/0/0/0`
- roughly 13-14 FPS in the shown interval.

Root cause: 0.0.125 cached `pvr_gpu_fastpath_ready` and `pvr_mt_fastpath_ready`, but the emitted runner refreshed those latches before assigning the final `pvr_gpu_enabled` / `pvr_mt_enabled` command-line values. Both cached latches therefore remained false for the run.

## 0.0.126 correction

Both emitted runner variants now perform:

```cpp
runtime.pvr_gpu_enabled = pvr_gpu;
runtime.pvr_gpu_force_readback = pvr_gpu_force_readback;
runtime.pvr_mt_enabled = pvr_mt;
dc_pvr_refresh_fastpaths(runtime);
```

The 0.0.124 mixer optimization and 0.0.125 CPU-side PVR optimizations remain intact.

A heartbeat field `pvr-fast=GPU/MT` was added. The standard commercial runner is expected to report `pvr-fast=1/1` together with an advancing `pvr-gpu=on/ready`, `pvr-gbind`, and `pvr-gpresent` path.

## Validation

- Core CTest: 47/47 PASS.
- Fresh generated C++ project: configure/build PASS.
- Fresh `generated_compile_test`: exit 0.
- Generated runner inspected: fast-path refresh occurs after final PVR GPU/MT option assignment.
