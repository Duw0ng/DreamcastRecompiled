# DCR V1.2 — CT2 wait-loop coalescing experiment

This is an opt-in **runtime-only** experimental build. The v1.2 Stable
release and precompiled game ABI1 remain unchanged. No commercial media
or new SH-4 game DLL is distributed.

## Verified guest sequence

From user-provided `generated_program_part_27.cpp`:

- `0x8C157878` (5 cycles): load dynamic callback address from
  `0x0C300B38`, load its argument at `0x0C300B3C`, set PR to
  `0x8C157880` and dispatch callback.
- `0x8C1560B0` (3 cycles): exactly RTS/NOP, leaves registers unchanged,
  sets PC=PR unless the scheduler redirects.
- `0x8C157880` (10 cycles): read `0x0C300AF4`, `0x0C300AF8`,
  `0x0C300C40`, compare count40 with AF4+AF8+1.
- `0x8C157898` (4 cycles): test `0x0C300C44` and loop while nonzero.

**Each complete retry therefore costs 22 emulated SH-4 cycles.**

## Algorithm

Enable `DCR_CT2_WAIT_COALESCE=1` (off by default). The existing
cached RTS/NOP dispatch is redirected to a runtime shim only at the
validated address, with exact live instruction bytes `0B 00 09 00`.
The shim executes the ordinary 3-cycle tick, then checks the **previous
iteration's register state**, dynamic callback pointer, callback argument,
and the exact branch predicates.

If all guards match, the shim accounts for a bounded number of 22-cycle
retry iterations *without* executing their C++ call/compare blocks.
The count never crosses the already-unflushed SH-4 device tick batch
(<=256 cycles): if there is insufficient headroom, the original path is
executed. This leaves guest scheduler order, IRQ opportunity, and all
subsequent AICA/PVR/TMU/GD-ROM ticks with their original tick boundary.

The callback still returns to `0x8C157880`, where its original
comparison and register writes execute normally. Normal fallback
is used whenever guard conditions fail, the callback changes, or an
asynchronous redirect is pending.

Limitations: this shortcut relies on the same single-threaded, tick-bound
guest RAM/device visibility as the existing commercial runtime. It should
remain opt-in until A/B gameplay and timing tests validate its benefits.
It is **not** a general game speed modifier or 60-FPS unlock.

## A/B test

Run:

```bat
scripts\perf\TEST_CT2_WAIT_AB.bat "C:\Games\Crazy Taxi 2.cdi"
```

The launcher runs the same game once with coalescing OFF and then ON,
with `DCR_SH4_RETFAST=0` in both. Run the same in-game scene for at least
60 seconds each, close each window normally, and upload both timestamped
session logs. On the enabled run search for `[DCR CT2 WAIT]`; its
`skipped-iterations` count must be positive for the optimization to be
considered active. Compare sustained gameplay FPS, frames per elapsed
real second, audio-starves per second, crashes and graphical regressions.

Unit test cases: `dcr_ct2_wait_off`, `dcr_ct2_wait_on`.
Both validate the original callback PR/PC; the ON case validates that
coalescing stays strictly below 256 cycles and does not activate once
the counter changes. Original RTS/NOP fastpath regression cases are
retained separately.

No FPS improvement is claimed without comparable game logs.
