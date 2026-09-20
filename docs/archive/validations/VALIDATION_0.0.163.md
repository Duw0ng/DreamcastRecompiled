# DreamcastRecomp 0.0.163 validation

## Why 0.0.162 was rejected

The 0.0.162 live log reduced host synchronization from the proven 262,144-cycle cadence to 1,048,576 cycles and removed post-Present synchronization. Under comparable TA load, sustained FPS fell by roughly 5–8 FPS in multiple heavy intervals and audio starvation rose materially. The `push_back` index experiment was also removed because it adds a capacity/size branch per emitted index.

## 0.0.163 policy

- Production GPR cache: OFF (same recovery policy as 0.0.161).
- Host/AICA sync cadence: 262,144 SH-4 cycles.
- Post-Present host/AICA sync: ON.
- PVR scheduler helper: deterministic deadline gated, timing unchanged.
- Win32 message service: guest-cycle gated before wall-clock check, timing-neutral.
- Opaque index emission: contiguous resize + pointer fill.
- PERF heartbeat I/O excluded from the next PVR/device timing bucket.

## Validation

- Release host build: PASS.
- CTest: 50/50 PASS.
- Fresh generated C++ project: PASS.
- generated_compile_test: PASS.
- Fresh generated native hello runner: PASS; returns to host with PC=0xFFFFFFFF.

Commercial ChuChu Rocket! was not executed in this environment; final FPS validation remains the user's Windows live run.
