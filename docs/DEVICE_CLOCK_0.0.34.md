# DreamcastRecomp 0.0.34 — common device-time scheduler

## Why 0.0.34 exists

0.0.33 reduced the user's Windows live-audio failures from frequent stutter to four short underruns over roughly 45+ seconds, with correct musical tempo. Its PVR bridge proved the diagnosis, but it still treated displayed frames as an audio refill trigger instead of representing one shared Dreamcast timeline.

0.0.34 moves time ownership into the runtime.

## Clock domains

The current scheduler uses these nominal domains:

- SH-4 I-clock: 200,000,000 Hz
- AICA/ARM scheduler domain: 45,158,400 Hz
- AICA mixer: 44,100 stereo frames/s
- therefore 1,024 AICA scheduler clocks per mixer sample

Generated SH-4 blocks now pass estimated **issue cycles** to `dc_runtime_tick()` instead of raw DCIR operation counts. Synthetic DCIR helpers (`SaveT`, dynamic-target capture, RTE preparation) contribute no guest time.

The initial issue-rate table is intentionally conservative. It models important multi-issue-cost classes such as multiplication, register control operations, TRAPA/SLEEP/RTE, while ordinary EX/LS/BR/FPU operations start at one issue cycle. Pipeline dependencies, cache misses and bus stalls are future refinements.

## Deterministic mode: `--device-clock`

`dc_runtime_tick(runtime, sh4_cycles)` advances the master SH-4 cycle count. Integer rational accumulation converts each SH-4 interval into owed AICA clocks without floating point drift.

PVR VBlank is an anchor, not an audio generator. If guest CPU accounting has not yet reached the minimum time represented by the next frame, the **master clock** advances through the missing idle cycles. AICA then advances because time advanced; no PCM samples are directly inserted or repeated.

This makes deterministic capture independent of host execution speed.

## Live mode: `--device-clock-host`

Live playback has a different problem: the software rasterizer and Win32 presentation can take more wall time than a nominal 60 Hz frame. Real AICA hardware would continue advancing while the SH-4/PVR side is busy.

Host mode therefore uses `std::chrono::steady_clock` as the wall-time anchor. At periodic scheduler points and after PVR presentation/sleep, the runtime computes how many AICA clocks should have elapsed and runs exactly the deficit.

This means a 30 ms host frame produces roughly 30 ms of AICA time rather than always producing only 1/60 s of audio.

The ARM polling fast-forward remains active, so most of the catch-up interval can skip the historical s3mplay wait loop without changing timer/FIQ boundaries.

## Legacy paths

These remain for comparison only:

- `--aica-sh4-div=N`
- `--aica-pvr-sync`

The normal 2ndMix live/capture helpers no longer use them.

## Diagnostics

Native AICA stats now add:

- `dc-clock=virtual|host`
- `dc-sh4-cycles`
- `dc-aica-steps`
- `dc-pvr-anchors`
- `dc-idle-cycles`
- `dc-host-syncs`
- `dc-host-catchup`

The existing WinMM line remains the direct acceptance signal for Windows:

```text
[AICA WinMM] ... min-queue=... max-queue=... max-submit-gap-ms=... underrun-restarts=...
```

## 2ndMix result

With the supplied real ELF:

- 181 reachable functions
- RAW_SH4 = 0
- 5 s = 220,500 mixer frames
- Timer calibration remains 16/21 for the historical embedded s3mplay
- ARM faults = 0
- unsupported native formats = 0
- invalid sample reads = 0
- legacy PVR audio top-ups = 0

A deterministic `--device-clock` five-second WAV and a wall-time `--device-clock-host` five-second WAV are byte-identical to each other and to the corresponding validated 0.0.33 interval.

## Remaining limitations

This is the first common clock, not final cycle accuracy:

- SH-4 dependency/cache/bus stalls are not yet modeled.
- ARM7 interpreted instructions are still treated as scheduler clocks, with polling fast-forward preserving interrupt boundaries.
- 2ndMix still uses the measured 16/21 timer calibration.
- PVR interrupt delivery is not yet tied into a complete Holly interrupt controller.
- Maple is still probe-only.
