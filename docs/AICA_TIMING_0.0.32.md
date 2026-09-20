# AICA / 2ndMix timing — DreamcastRecomp 0.0.32

## Why this milestone exists

Windows validation of 0.0.31 reported 86 WinMM underrun restarts and, independently, audible music that progressed faster than an original/reference playback. These are related through the incomplete scheduler, but they are not the same variable: the host output device always consumes 44,100 stereo frames per second, while the embedded tracker advances its song state from AICA Timer-A/FIQ events.

0.0.32 therefore keeps those clocks separable.

## Real 2ndMix trace

The supplied ELF contains the original embedded `s3mplay` ARM firmware and `/rd/e-79014.s3m`.

The firmware correctly observes:

- initial S3M speed = 5;
- initial S3M tempo = 125;
- first-pattern speed = 7;
- first-pattern tempo = 112.

So the tempo mismatch is not another ROMFS/S3M parser corruption.

The historical ARM player converts tempo internally and advances its tracker tick using Timer-A waits. With the current DreamcastRecomp 1/1 timer model, the resulting musical progression is about 1.31x faster than a standard render of the exact same S3M.

## Independent timer scaling

New runner option:

```text
--aica-timer-rate=N/D
```

This scales Timer A/B/C advancement only. The native mixer remains exactly 44.1 kHz, so changing the timer ratio does not pitch-shift or resample PCM.

Generic default:

```text
--aica-timer-rate=1/1
```

2ndMix validation helper:

```text
--aica-timer-rate=16/21
```

`16/21` is deliberately scoped to the current 2ndMix timing experiment. It is not claimed as the physical AICA timer ratio.

## Objective comparison

15-second captures of the exact same S3M were compared through onset-envelope time mapping and chroma dynamic time warping.

0.0.31 / timer 1/1:

- best onset time mapping: about 0.765;
- equivalently, DreamcastRecomp progressed about 1/0.765 ~= 1.31x too fast.

0.0.32 / timer 16/21:

- best onset time mapping: about 1.003;
- chroma-DTW progression slope: about 0.997;
- the remaining global progression error is therefore below roughly one percent in this 15-second comparison.

This does not prove cycle-accurate AICA hardware timing. It does prove that the audible tempo complaint can be measured and that the new timer/mixer clock separation can correct the 2ndMix integration test without changing sample rate.

## WinMM changes

0.0.32 also increases the live host reservoir:

- chunk: 2048 frames (~46.4 ms);
- startup prefill: 24 chunks (~1.11 s);
- bounded maximum: 64 chunks (~2.97 s).

Shutdown prints:

```text
[AICA WinMM] chunks=... | chunk-ms=46 | prefill-ms=1114 | min-queue=... | max-queue=... | underrun-restarts=...
```

A successful long live run should ideally end with `underrun-restarts=0` and `min-queue > 0`.

## What remains unresolved

The timer calibration is intentionally a bridge. The correct long-term fix is one common device-time scheduler that advances SH-4, ARM7, AICA timers, the 44.1 kHz mixer and PVR scanout from explicit clock domains rather than from configurable SH-4 instruction ratios.
