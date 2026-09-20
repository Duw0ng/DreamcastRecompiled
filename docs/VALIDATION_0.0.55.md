# DreamcastRecomp 0.0.55 validation

## Regression

- Linux CMake build of modified emitter/runtime: PASS.
- CTest: **47/47 PASS**.
- Commercial ChuChu raw closure: **2,273 functions**, **170,808 known SH-4 instructions**, **0 unknown**, **RAW_SH4=0**.
- Commercial generated `dc_runtime.cpp` and `dc_native_overrides.cpp`: standalone C++ compile PASS.
- Full 2,273-function commercial generated program: Clang 17 `-O0` compile/link PASS (`dreamcast_program`).
- Bounded commercial headless smoke: reached the intentional 100-TA-packet probe stop with no preceding DreamcastRecomp runtime failure.
- Closure delta from 0.0.54: **0 functions / 0 instructions**; 0.0.55 is runtime/input/performance work, not a new title seed.

## BIOS-font regression

A generated-runtime probe constructs `DCRuntime`, sets the reported crash PC (`0x8C0227D0`) and reads both `0xA010A070` and `0xA0100020`. Both accesses complete and return deterministic synthetic ROM data instead of throwing a memory-map exception.

The aperture is read-only and contains no Dreamcast BIOS bytes.

## Maple regression

A generated-runtime packet probe round-trips A, B, X, Y, START and all four D-pad bits through the Maple GETCOND response. Every cooked pressed bit is emitted with the expected active-low raw mask.

The Windows host bridge additionally reports keyboard/XInput source masks and recent button activity in the heartbeat so the next acceptance run can verify actual host capture.

## PVR performance regression

A controlled local full-screen 640x480 RGB565 bilinear raster microbenchmark was run interleaved against the untouched 0.0.54 runtime. Representative five-frame pairs produced approximately:

- 0.0.54: 40.4-42.1 ms/frame, median ~41.2 ms/frame
- 0.0.55: 24.4-25.6 ms/frame, median ~24.7 ms/frame

This is about a **40% reduction in raster time** (~1.67x throughput) for that texture-heavy synthetic workload. Untextured framebuffer checksum remains identical in the corresponding raster probe. The optimization does not change the 640x480 software target.

## Limits of local validation

The full 2,273-function commercial program now compiles and links successfully in a low-optimization Clang validation build. The bounded Linux smoke test also reaches the intentional TA packet limit. Linux still cannot reproduce the actual Win32 keyboard/XInput path or provide an authoritative save-menu/FPS acceptance result, so those remain Windows test items.
