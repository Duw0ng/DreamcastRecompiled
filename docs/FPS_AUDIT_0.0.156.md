# DreamcastRecomp 0.0.156 — FPS audit

## Current priority

The target remains 60 FPS minimum in ChuChu Rocket! Mouse Mania. 0.0.156 attacks two high-frequency host costs without changing PVR/FPU numerical semantics:

1. **SQ -> PREF -> TA packet materialization:** eliminate four-to-eight architectural SQ helper writes plus the 32-byte PREF reload when a complete producer is statically proven and runtime guards agree.
2. **Natural-loop PC bookkeeping:** avoid repeated host stores of `ctx.pc` and `runtime.current_pc` on internal hot-loop edges that do not cross the existing scheduler quantum.

## Safety boundaries

SQ fusion is intentionally same-basic-block only. Crossing a block boundary would cross a scheduler observation point and delaying Store Queue state there could be architecturally visible. Memory loads, unsupported stores, calls/branches, PREF, RAW_SH4 and FPSCR bank/mode changes break recognition.

Hot traces do not skip guest cycles or device scheduling. They update the same pending cycle accumulator and synchronize PC before `dc_runtime_tick_full`, external control-flow barriers and other architectural boundaries.

## Next candidates if 60 FPS is not reached

- Measure actual SQ-fusion site count/fallback rate and extend only proven producer forms, not scheduler-crossing cases.
- GPR register allocation/liveness alongside FR/XF static superblocks.
- Profile-guided compile-time hot-function layout/constant propagation using a saved profile without adding runtime per-instruction telemetry.
- Inspect MSVC assembly/spills for FTRV/FIPR/FM​AC/FSRRA hot functions before introducing explicit SIMD intrinsics.
- GPU vertex/index upload ring and remaining TA decode/materialization copies.
