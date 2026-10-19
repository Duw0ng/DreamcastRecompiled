# Crazy Taxi 2: SH-4 indirect callback probe (DCR V1.2)

This **opt-in, runtime-only** instrument records SH-4 indirect call targets
executed in/around `sub_8C157720`. It is not a speed fix. The goal is to
confirm the actual callback and whether the counter loop is a busy wait before
changing scheduler behavior.

## How to run

1. Build the **diagnostic branch's runtime**; keep the same existing Crazy Taxi 2
   AOT/native game DLL and cache (Guest ABI v1 is unchanged).
2. Run from the repository root:

   ```bat
   scripts\perf\PROBE_CT2_CALLBACK.bat "C:\Games\Crazy Taxi 2.cdi"
   ```

3. Share the newest `logs\DreamcastRecomp_*session*.log`. Play or let the
   title run for 15–30 seconds past the menu. Do **not** compare FPS from the
   instrumented run; use `scripts/perf/TEST_CRAZY_TAXI_2_FPS.bat` for an
   uninstrumented baseline.

## What is recorded

- `[DCR CT2 SLOT]`: guest PC and PR, value in RAM
  `0x0C300B38`, counters `0x0C300C40` and `0x0C300C44`, and
  SH-4 registers R0–R3 sampled at scheduler tick boundaries.
- `[DCR CT2 CALL]`: actual *dynamic dispatch* caller and target, sampled
  via the runtime's existing 32-entry call history; R8 and SP are retained
  as extra context, with **post-dispatch** RAM values for correlation.
- `[DCR CT2 SUMMARY]`: visits to the routine's address interval, observed
  dynamic calls, number of distinct targets, the most frequent target and
  trace-ring overwrite counts, every ten heartbeat reports.

The recorded region is `0x8C157720–0x8C1578FF`. The callback
instruction under investigation is near `0x8C15787C` and the frequently
observed PC is `0x8C157880`.

**Interpretation limits:** a native direct call (not going through dynamic
dispatch) will not be present in the call ring; large numbers of
`overwritten-trace-events` mean some dynamic targets may have been missed.
In that case, `SLOT` data can still show how counters evolve, but cannot
prove the dynamic call target. RAM counters are read at tick boundaries,
not atomically at the JSR instruction.

The patch does **not** force any sleep, skip SH-4 iterations, modify
PVR/Holly interrupts, move AICA time or alter host-sync. The probe is
disabled for normal launches unless `DCR_CT2_CALLBACK_PROBE=1`.
