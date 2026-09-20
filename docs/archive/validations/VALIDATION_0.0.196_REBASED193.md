# DreamcastRecomp 0.0.196 (rebased from 0.0.193) validation

## Lineage and goal

- Functional lineage: `0.0.193 -> 0.0.194_rebased193 -> 0.0.195_rebased193 -> 0.0.196_rebased193`.
- `0.0.195_rebased193` completed the full Crazy Taxi 2 menu attract/demo with no `DreamcastRecomp ERROR`, no `DCR SH4-FAULT`, and no recurrence of the former unresolved `0x0C06C35C` path.
- 0.0.196 keeps that compatibility baseline and targets host overhead on the measured SH-4 tick/dispatch hot paths.

## Guest timing invariants

- CT2 SH-4 batch remains exactly `256` guest cycles.
- No Dreamcast CPU/PVR/AICA/TMU clock is increased to manufacture 60 FPS.
- IRQ ordering, Holly pending/ack semantics, GD-ROM ordering, render-done scheduling, and the proven wait157 fast-forward rules are unchanged.

## 0.0.196 optimizations

### Fixed commercial specialization

The CT2 target is compiled with validated commercial constants:

- `DCR_FIXED_SH4_TICK_BATCH=256`
- `DCR_FIXED_FPU_MODE=3` (`regionplus`)
- `DCR_FIXED_FPU_REGION_METRICS=0`
- `DCR_DISABLE_HOT_TICK_CALL_COUNTER=1`
- `DCR_LIGHTWEIGHT_HOT_METRICS=1`

The generic emitter remains runtime-selectable; these constants are specific to the tested CT2 package.

### SH-4 fast-tick overhead

The prior full-demo log executed roughly 1.75 billion fast-tick checks but only about 42.4 million full ticks. The commercial fast path now:

- constant-folds the 256-cycle threshold;
- omits the per-fast-call diagnostic hit counter;
- preserves the exact pending-cycle accumulation and full-tick boundary.

### TMU0 common case

CT2 consistently runs TMU0 with TPS=2 (core/256). A direct shift/mask path now handles that case without the generic 3-channel divisor loop. Underflow, reload, UNF, and interrupt behavior remain the same; all other configurations use the existing generic path.

### Profiler stride

The profile sampler recognizes power-of-two strides. The standard CT2 stride 64 uses `counter & 63` instead of a 64-bit modulo operation. Non-power-of-two strides keep the modulo fallback.

### Dynamic dispatch diagnostics

Successful dynamic calls previously updated multiple counters and the last-32-call ring even when trace output was disabled. For the CT2 performance build:

- call-history recording is opt-in (`DCR_TRACE_HISTORY=1`);
- `run_crazy_taxi_2_audit.bat` enables it;
- normal/perf runners avoid the ring writes;
- ultra-hot successful dispatch/direct hit counters use lightweight mode, while misses/fallbacks are still counted.

Because of this, some 0.0.196 heartbeat hit counters may be zero or lower by design. Compare `wall-ms`, `frames`, FPS, `DCR PERF BUDGET`, `sh4-full`, PVR timings, and errors rather than expecting hot diagnostic-hit counters to match 0.0.195.

## Validation performed

- Core CTest suite: **51/51 passed** after the scheduler/TMU/profiler/dispatch changes.
- CT2 `dc_runtime.cpp` compiled successfully with the 0.0.196 commercial definitions.
- CT2 `generated_runner.cpp` compiled successfully.
- The generated shard containing hot PC `0x8C080CE6` compiled successfully at a low optimization validation setting.
- A full optimized Linux commercial link was not claimed: compiling the very large hot shard at `-O3` exceeded the execution window before link, with no compiler error observed before timeout.

## Benchmark protocol

1. Place the user's own `ct2.cdi` beside the CT2 BAT files (the game image is not distributed).
2. Build with `build_windows.bat`.
3. Run `run_crazy_taxi_2_perf_profile.bat`.
4. Do not provide input; allow the same menu attract/demo to complete.
5. Compare primarily:
   - `wall-ms / frames`;
   - `fps=` distribution;
   - `DCR PERF BUDGET`, especially `tick`, `device`, `pvr-ta`, `pvr-state`, `pvr-geom`, `gpu-render`;
   - `sh4-full`;
   - any `DreamcastRecomp ERROR` or `DCR SH4-FAULT`.

No FPS improvement is claimed until measured on the user's Windows test system.
