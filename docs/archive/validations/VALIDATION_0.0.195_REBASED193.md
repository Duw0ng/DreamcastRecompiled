# DreamcastRecomp 0.0.195 validation — rebased from 0.0.193

## Compatibility parent

- Sole compatibility parent: 0.0.193.
- Crazy Taxi 2 SH-4 tick batch: 256 cycles in every supplied CT2 run script.
- No historical 512-cycle boot-regression scheduler change is included.

## SH-4 closure validation

The 0.0.194 menu-demo PERF log stopped at an indirect call to physical `0x0C06C35C` / canonical `0x8C06C35C`, with nearest registered native target only 8 bytes away.

Static analysis of the raw CT2 boot image found:

- `0x8C06C33A` returns literal `0x0C06C35C` in R0 via `MOV.L literal,R0 ; RTS ; NOP`.
- `0x8C06C35C` uses a two-entry indexed callback table at `0x8C112970`.
- The code proves the selector is non-negative and `< 2` before indexing the table.
- Table entries are `0x0C06C44A` and `0x0C06C4E6`.
- Direct local dependencies exposed by those callbacks are `0x8C06C71E` and `0x8C06C8EA`.

With the new generic closure rules and seed `0x8C06C33A`, `dc_raw_recomp` reports:

- Unknown SH-4: 0
- RAW_SH4: 0
- Returned literal targets: 1
- The function map contains `0x8C06C35C`, `0x8C06C44A`, and `0x8C06C4E6`.

The generated CT2 package additionally registers all five newly exposed functions and their internal CFG entry blocks.

## Performance changes

The build keeps guest timing unchanged and removes host-side work only when equivalence is proven:

- TMU power-of-two divider fast path.
- No-op scheduler helper gating.
- Guest-cycle-gated heartbeat clock sampling.
- Compact TA Type-7/EOS tags for strip scanning.

Reference 0.0.194 attract-mode PERF result at 7,312 frames:

- TA packets: 63,942,638
- TA staging: 70,119 ms
- tick: 6.462 ms/frame
- pvr-ta: 2.475 ms/frame
- pvr-state: 1.816 ms/frame
- pvr-geom: 1.825 ms/frame
- gpu-render: 3.041 ms/frame

The 0.0.195 target is to reduce these costs while preserving the same guest behavior; 60 FPS is not claimed until measured on the user's Windows run.

## Build/test validation

- Main project: builds successfully on Linux.
- CTest: 51/51 passed, 0 failures.
- New SH-4 recovered fragments compile individually (`0x8C06C35C`, `44A`, `4E6`, `71E`, `8EA`).
- Generated `dc_runtime.cpp`, `generated_program.cpp`, and `generated_runner.cpp` compile at unit/syntax level after integration.
- CT2 CMake configuration succeeds with all new sources registered.
- Full generated commercial build was attempted and progressed through runtime/image/native/program compilation; the environment execution limit terminated it before the large AOT shard build/link completed. No compiler error was emitted before termination.

## Next-log acceptance

A useful 0.0.195 menu-demo log should show:

1. Normal two code relocations and GD-ROM/PVR startup.
2. No unresolved target at `0x0C06C35C`.
3. Progress through `0x8C06C44A`/`0x8C06C4E6` without another immediate closure hole.
4. More than the previous ~7,312 PVR frames / 822 GD-ROM requests if the same attract-mode path is reached.
5. PERF comparison of `tick`, `pvr-ta`, `pvr-state`, `pvr-geom`, `gpu-render`, and normalized `pvr-ta-stage-ms / packets`.
