# Crazy Taxi 2: verified SH-4 callback loop and tick A/B experiment (2026-10-18)

**Status:** experimental, not v1.2 Stable; no unverified FPS claims.

## Provenance

Analyzed user-provided `BOOT.DISC.BIN` (SHA-256
`69f7bd24cc82752d0b4995290e91b2e0de8eeda7efefadd4bd227ec6ba2b9042`),
`function_map.csv` and a separate 445-heartbeat / 80,227-sample HOTPC log.
**Do not upload or distribute the copyrighted BOOT.DISC.BIN, IP.BIN, game
image, or a compiled copy of the guest in this experiment.** The files are used
only for local reverse engineering.

The boot image is staged at `0x8C010000`; SH-4 little-endian 16-bit code
at `0x8C157880` is within `sub_8C157720`.

## Native SH-4 evidence

| PC | Decoded instruction | Meaning |
| --- | --- | --- |
| `0x8C157878` | `mov.l @(132,pc),r3` | Address of callback pointer `0x0C300B38` |
| `0x8C15787A` | `mov.l @r3,r2` | Loads **mutable** callback target |
| `0x8C15787C` | `jsr @r2` | Executes callback; not skippable |
| `0x8C15787E` | `mov.l @(4,r3),r4` | Delay slot supplies callback argument |
| `0x8C157880` | `mov.l @(128,pc),r1` | Callback return address / hot sampled PC |
| `0x8C157882-890` | loads/add/compare | Reads `0x0C300AF4`, `0x0C300AF8`, `0x0C300C40` |
| `0x8C157898-89E` | flag read; `bf 0x8C157878` | Repeats while `0x0C300C44` indicates continuation |

**Critical correctness constraint:** never simply fast-forward the loop
or ignore the indirect callback. Its target and side effects can change.
The 55.3% HOTPC PC share does not prove 55.3% wall CPU consumption.

## Observed full log (new .BAT, 2026-10-18)

- 80,227 sampled full ticks at stride 2048; `0x8C157880`: 44,377 (55.3%).
- Roughly 27.6 mean and 28 median reported heartbeat FPS; scene changes and
  loading mean the value is not a controlled benchmark.
- About 165.8 million full SH-4 tick calls and 7.29 billion tick requests.
- Sample-estimated `host-sync` ~= 34 seconds, `tick` ~= 134.8 seconds
  for the session. These sampled figures are *diagnostic estimates* and overlap;
  do not add them as independent costs.
- `spg-ff=on/18` after startup: the SPG fast-forward is not a convincing
  ongoing cause of this game's slowdown.
- `perf=on`, `trace-history=off`, HOTPC actual samples present.

## Two safe, separately testable changes

**A. Native runtime: idempotent IP.BIN observer fast path.**
Inside `dc_runtime_tick_full`, an armed valid shadow whose game code has
been seen and whose reentry guard is not latched has nothing new to observe.
Avoid repeated `ipbin_observe_game_execution` calls in that exact state.
Preserve ordinary observation before arming, for first game-code sighting,
and when the reentry latch is set. This is a tiny path-level optimization; it
does not bypass the guest callback loop, device clocks, PVR, AICA, IRQs or
reentry checks. Existing bootstrap, audio and stage regression tests run in CI.

**B. Reversible host-sync A/B.** `--fast-sync` maps to already supported
`run_game.bat --sync-524`, changing sync quantum from 262,144 to 524,288
SH-4 cycles. The host-side firmware budget scales with that quantum, but the
change in scheduling granularity may affect audio/video timing. Test against
the default mode before recommending it. The stable baseline retains 262,144.

## Instructions

After extracting the full experimental ZIP into a **separate** directory
from Stable and verifying an existing Crazy Taxi 2 native cache:

```bat
TEST_CRAZY_TAXI_2_FPS.bat "C:\\Games\\Crazy Taxi 2.cdi"
TEST_CRAZY_TAXI_2_FPS.bat "C:\\Games\\Crazy Taxi 2.cdi" --fast-sync
```

Repeat each **without** profiler in the same scene, ideally an uninterrupted
90-120 second driving segment (ignore loading and first 10 seconds), three
times, with the same game options. Preserve logs and match run modes.

To diagnose instead of benchmarking:

```bat
TEST_CRAZY_TAXI_2_FPS.bat "C:\\Games\\Crazy Taxi 2.cdi" --profile
TEST_CRAZY_TAXI_2_FPS.bat "C:\\Games\\Crazy Taxi 2.cdi" --fast-sync --profile
```

The profiler introduces overhead; its FPS must not be compared to clean runs.

**Acceptance:** higher stable FPS with no regressions in controller response,
rendering, FMV/menu transitions, audio-starves or main loop progress;
compare the host-sync counters, SH-4 tick counters, actual PVR frames, and
per-frame timing. If no measurable improvement, revert B, retain the safe
observer fast path only if independently validated.

**Boundary:** Even a perfect host-sync optimization is not likely by itself
to increase CT2 from ~28 FPS to 60 FPS. The indirect callback's work and
SH-4 scheduling still require deeper inspection and targeted benchmark data.


## Direct SH-4 analysis of `sub_8C157720`

The actual binary disassembly (using this repository's own SH-4 decoder) shows:

- `0x8C15786A`: stores zero to `0x0C300C40` before entering the
  first callback loop when `0x0C300C44` is nonzero.
- `0x8C157878–0x8C15787E`: loads the **mutable** function pointer from
  `0x0C300B38`, passes the adjacent `0x0C300B3C` as R4, calls `JSR @R2`.
  `0x8C157880` is the return PC.
- `0x8C157880–0x8C157898`: reads the values at `0x0C300AF4`,
  `0x0C300AF8`, `0x0C300C40`; compares the *unsigned* counter
  `C40 > AF4 + AF8 + 1` (SH-4 32-bit wraparound). This may set R12.
  The callback is otherwise repeated while `0x0C300C44 != 0`.
- `0x8C1578B8–0x8C157926`: a second callback loop is bounded by
  `C40 > 20` or the continuation flag clearing; it is **not** safe to
  skip this loop by forcing counter progress.
- `0x8C15793A–0x8C15793E`: increments `0x0C300C2C` and masks it
  modulo 4. The later section also calls `0x8C156D04`, a helper that
  writes to the PVR MMIO area `A05F8000` using an offset.
- Nearby `sub_8C149B2E` and `sub_8C157A60` reuse the same callback
  pointer and stop flags, confirming this is a recurring coordination
  mechanism, not a dead temporary loop.

**Interpretation, not a proved symbol name:** this resembles a rendering /
event synchronization pump. It interacts with state changed by other game
routines and PVR code, but without the live target and event timing we cannot
prove whether the callback polls hardware, pumps tasks, or performs useful
per-frame work. In HOTPC, both `ctx.pc` and `ctx.pr` spend much of the time
at `0x8C157880`, consistent with an indirect-call return site; it does
not provide a timed call graph.

### Exact next diagnostic

The experimental runtime adds an opt-in, **sampled** callback probe. It
does not alter the guest code, SH-4 cycles, interrupt order or persistent
save data. Run:

```bat
TEST_CRAZY_TAXI_2_FPS.bat "C:\\Games\\Crazy Taxi 2.cdi" --callback-probe
```

The `--callback-probe` option turns on low-frequency HOTPC sampling and
`DCR_CT2_CALLBACK_PROBE=1`. Log entries look like:

```text
[DCR CT2 CALLBACK] sampled=N return-pc=N flag-active=N above-gate=N ... | callback-ptr=0xXXXXXXXX:count@percent
```

- `callback-ptr` is a **live memory snapshot after or during a callback**;
  the callback itself is allowed to change the pointer, so this does *not*
  prove that every listed pointer was the one actually invoked.
- `flag-active` counts the samples with `0x0C300C44 != 0`.
- `above-gate` counts snapshots where `0x0C300C40` exceeds the unsigned
  comparison threshold `0x0C300AF4 + 0x0C300AF8 + 1`.
- An empty or zero pointer should be checked against a later heartbeat,
  but never assumed valid and called by the tooling.
- Only samples whose `ctx.pc` or `ctx.pr` equal `0x8C157880`
  are collected. This is an intentional biased *diagnostic* sample,
  not a statistically valid CPU-time fraction.
- Never benchmark FPS with `--callback-probe` or `--profile` enabled.

Then resolve the most common `callback-ptr` values in the provided
`function_map.csv` and inspect the called SH-4 code. Only after this
can a callback-specific, side-effect-preserving optimization be designed.
