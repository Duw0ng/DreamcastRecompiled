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
