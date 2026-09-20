# DreamcastRecomp 0.0.197 (rebased from 0.0.193) validation

## Lineage and measured target

- Functional lineage: `0.0.193 -> 0.0.194_rebased193 -> 0.0.195_rebased193 -> 0.0.196_rebased193 -> 0.0.197_rebased193`.
- 0.0.195 completed the entire Crazy Taxi 2 menu attract/demo.
- 0.0.196 reduced the measured `tick` budget from 8.898 to 6.012 ms/frame, but near the end exposed one supplemental AOT packaging hole: indirect callback `0x0C06C41C` (`P1 0x8C06C41C`).
- 0.0.197 keeps the 0.0.196 performance work, closes that target, and targets two additional host costs: eager TMU0 accounting and normal-RAM PREF helper calls.

## Guest timing invariants

- SH-4 scheduler quantum remains exactly **256 guest cycles**.
- No Dreamcast CPU, PVR, AICA or TMU clock is increased.
- Holly IRQ ordering, GD-ROM ordering, SPG/PVR cadence, render-done scheduling and the existing wait157 rules are unchanged.

## SH-4/AOT coverage

The current analyzer closure rooted at `0x8C06C35C` already recognizes `0x8C06C41C` as executable code. The 0.0.196 runtime failure therefore came from the manually assembled supplemental CT2 package omitting a target the analyzer had already found.

`0x8C06C41C` is a six-instruction thunk that loads the already-known target `0x0C05B3D4` and tail-jumps to it. 0.0.197 adds its generated source/header to the CT2 target, includes its registration, and adds a compile-test lookup for `0x8C06C41C`.

The newly exposed local chain packaged for CT2 is now:

- `0x8C06C35C`
- `0x8C06C41C`
- `0x8C06C44A`
- `0x8C06C4E6`
- `0x8C06C71E`
- `0x8C06C8EA`

Other functions in that analyzer closure were already present in the primary AOT image.

## TMU0 lazy exact path

CT2's measured steady state uses TMU0 with `TPS=2` (one decrement per 256 SH-4 core cycles) and `UNIE=0`. In this state, advancing `TCNT` every scheduler slice cannot itself deliver a timer IRQ.

With `DCR_LAZY_TMU0_TPS2=1`, the runtime therefore accumulates exact SH-4 cycles while this precise mode is active and materializes them before any TMU MMIO read/write or reconfiguration. Materialization uses the existing decrement/underflow/reload routine, so `TCNT`, `UNF`, reload count and sub-tick remainder remain exact. All other timer modes retain the eager generic path.

A dedicated host-side validation performed:

1. ten `/256` decrements accumulated lazily, followed by TCNT read; expected 1000 -> 990, observed 990;
2. a forced underflow from TCNT=2 with TCOR=5 and four decrements; expected reload result TCNT=4 plus `UNF=1`, observed exactly that.

Result: **TMU lazy exactness OK**.

## Guest PREF fast no-op

On SH-4, `PREF @Rn` is operationally important to this runtime for Store Queue addresses (`0xE0xxxxxx`) because it commits SQ data. For ordinary cached RAM it is a cache hint, and this host model does not emulate a data cache.

Generated code now uses `dc_guest_pref(...)`:

- normal/non-SQ address: returns inline, with no cross-TU runtime helper call in the lightweight performance build;
- Store Queue address: calls the existing `dc_pref(...)` path unchanged.

This specifically targets the geometry HOTPC family around `0x8C080CE6` and `0x8C080D3A`, where normal-memory PREF occurs inside heavily repeated transform loops.

## Validation performed

- Core CTest suite after generic emitter/test updates: **51/51 passed**.
- `pvr_pref_guard_tests`: passed as part of the suite.
- CT2 `dc_runtime.cpp`: compiled with the commercial definitions.
- CT2 hot shard `generated_program_part_17.cpp`: compiled after the PREF change.
- `generated_sub_8C06C41C.cpp`: compiled successfully.
- `generated_runner.cpp`: compiled successfully.
- CMake commercial build compiled the runtime/auxiliary units and all six supplemental callback sources before the execution window repeatedly stopped optimization of the very large `generated_program.cpp`. A full optimized Linux link is therefore **not claimed**.

## Benchmark protocol

1. Put the user's own `ct2.cdi` in `experimental_ct2_0.0.197_rebased193`.
2. Run `build_windows.bat`.
3. Run `run_crazy_taxi_2_perf_profile.bat`.
4. Provide no input; allow the same menu attract/demo to run to completion.
5. Verify first that no unresolved `0x0C06C41C` target appears.
6. Compare against 0.0.196 primarily using `wall-ms / frames`, `tick`, `tmu-gd`, `pvr-ta`, `pvr-state`, `pvr-geom`, `gpu-render`, and any `DreamcastRecomp ERROR`.

0.0.196 reference before its callback failure: `wall-ms=213371`, `frames=7697`, `tick=6.012 ms/frame`, `tmu-gd=0.241`, `pvr-ta=2.432`, `pvr-state=1.749`, `pvr-geom=1.760`, `gpu-render=3.088`.

No FPS improvement is claimed for 0.0.197 until measured on the user's Windows benchmark.
