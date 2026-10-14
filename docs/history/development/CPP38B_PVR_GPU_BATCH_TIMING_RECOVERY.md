# cpp38b — PVR GPU mapped-index + timing recovery

cpp38b keeps cpp38a's one-copy SQ capture, CH2 bulk RAM paths and SIMD strip scanning, then adds two targeted changes.

## 1. Direct mapped index stream

The D3D11 renderer already used persistent/triple dynamic vertex and index buffers. cpp38a still assembled every frame's compact R32 index stream in `gpu.index_scratch`, then mapped the D3D11 index buffer and copied the entire scratch vector a second time.

cpp38b maps the persistent index buffer first and materializes opaque late strips, legacy opaque index blocks and translucent triangles directly into that mapped memory. Submission order, strip-cut markers and run merging are unchanged.

This removes one frame-sized CPU index write/copy from the PVR path without changing Guest ABI1.

## 2. 0.0.161-era host-sync recovery

Historical ChuChu logs show an almost identical light/menu PVR workload around PC `0x8C0FE5A0`:

- 0.0.156: 353 PVR frames, 7,562 packets, 2,956 triangles, 81 FPS.
- cpp38a: 359 PVR frames, 7,595 packets, 2,968 triangles, 56 FPS.

The PVR bind counters are also nearly identical, so the missing headroom is not explained by more graphics work. The older stable 0.0.161/0.0.163 timing lineage used a 262,144 SH-4-cycle host/AICA synchronization cadence. Later 0.0.199-derived runtime code changed the default to 524,288 cycles.

cpp38b restores 262,144 at runtime without editing the frozen `DCRuntime` ABI header. `DCR_HOST_SYNC_QUANTUM` can override it for A/B testing; `TEST_CPP38B_TIMING_524.bat` reproduces the newer 524,288 setting.

This is a timing/pacing recovery experiment, not a guest-clock overclock. SH-4 batch, SPG/PVR guest timing and Guest ABI1 remain unchanged.

## Expected log markers

- `build=v1.2-cpp38b-pvr-gpu-batch-timing-recovery`
- `host-sync-q=262144`
- `pvr38=on/cpp38b/mapindex/capture1copy/ch2bulk/simdtag/homstrip`

## Benchmark request

Use `TEST_CPP38B_PVR_FAST.bat` in the same menu and Mouse Mania route used for cpp38a. For an exact control run, repeat the same route with `TEST_CPP38B_TIMING_524.bat`.
