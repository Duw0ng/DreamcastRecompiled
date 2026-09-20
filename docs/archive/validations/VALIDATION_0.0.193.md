# DreamcastRecomp 0.0.193 validation

## Compatibility / crash fixes
- Supplemental dynamic-call closure validator: 9003 callable literal references are covered by 42819 registered AOT targets.
- CT2 invalid asset lookup `0x87B1CA68`: diagnostic/recovery is placed on the exact guest lookup path after the game's own branch/sentinel checks; it does not remap the bad address.
- Recovery returns through `BB_8C07529E`, the original function epilogue, preserving saved SH-4 registers/PR/stack restoration.

## Performance
- Latest 0.0.192 profile measured `tick=12.587 ms/frame`, `pvr-ta=1.091 ms/frame`, `gpu-render=2.914 ms/frame`; HOTPC was dominated by `0x8C157880`.
- 0.0.193 fast-forwards only the known no-op CT2 wait callback (`0x0C1560B0`, arg 0) while C44 is busy and no Holly interrupt is pending.
- Skip chunks are bounded to 0x40000 SH-4 cycles and clamped to pending render-done and next PVR frame event. Due events disable skipping.
- Heartbeat telemetry: `wait157-ff=hits/cycles/max_chunk`.

## Build validation
- Targeted Linux C++20 compilation: `generated_program_part_15.cpp`, `generated_program_part_26.cpp`, and `dc_runtime.cpp` PASS.
- Core CTest suite: 51/51 PASS.
- Supplemental literal closure validator: PASS (9003/9003 callable references covered).
- Full CT2 generated build was started serially; unmodified huge AOT shards exceed this environment's per-command execution window. Modified translation units compile independently.
