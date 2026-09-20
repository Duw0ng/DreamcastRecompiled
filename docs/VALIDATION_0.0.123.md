# DreamcastRecomp 0.0.123 — validation

## Live evidence driving the rollback

The 0.0.122 run reported `host-aica-pollskip=0/0`, so the new ARM7 polling-probe skip never fired. The user nevertheless observed a maximum-rat low near 30 FPS, below the 0.0.120 live baseline (~43-47 FPS in the same stress event). Therefore 0.0.122 is rejected rather than tuned further.

0.0.121 also failed to demonstrate a material live gain, so 0.0.123 starts from the exact 0.0.120 source baseline.

## Diagnostic correction

The previous `perf_device_sampled_ns` bucket covered both guest PVR clock/VBlank scheduling and occasional wall-clock host synchronization. 0.0.123 preserves that aggregate counter for compatibility but additionally samples `perf_pvr_clock_sampled_ns` and `perf_host_sync_sampled_ns` separately. The heartbeat exposes `perf-device=PVR_CLOCK/HOST_SYNC/IRQ`.

Instrumentation remains opt-in under `--perf-profile`; normal gameplay does not pay the new timing calls.

## Local validation

- CTest: 47/47 PASS.
- Freshly generated host runtime/program: CMake configure/build PASS.
- `generated_compile_test`: exit 0.
