# Dedicated Windows audio worker — 0.0.41

## Why this release exists

The first Windows run of 0.0.40.3 made the standard KallistiOS `sound/sfx` demo genuinely usable, but the final telemetry still showed a host-side delivery problem:

```text
chunk-ms=23
prefill-ms=92
max-queue=6
max-submit-gap-ms=1271
underrun-restarts=124
```

The AICA/ARM7 path itself was running and button presses produced native key-ons. The remaining problem was that the same host thread responsible for SH-4/AICA work was also directly feeding WinMM. A long emulation/scheduler stall therefore emptied the WinMM queue. Repeated pause/re-prefill cycles could make multiple already-generated beeps arrive late in a burst.

## 0.0.41 architecture

Live host playback is now split into two timing domains:

```text
SH-4 / KOS / ARM7 / AICA mixer
            |
            | stereo PCM16
            v
   bounded PCM ring (16 x 512 frames)
            |
            v
 dedicated Windows audio worker
            |
            v
 short WinMM queue (target 4 x 512 frames)
            |
            v
          speakers
```

The emulation thread no longer calls `waveOutWrite` directly. It stages 512 stereo frames and enqueues the chunk into a bounded circular ring. A dedicated Windows thread owns the WinMM handle, cleans completed `WAVEHDR`s, and keeps a short device queue fed.

Current live profile:

- 512-frame chunks: about 11.6 ms each.
- 3-chunk startup prefill: about 34.8 ms.
- 4-chunk normal WinMM target: about 46.4 ms of queued device audio.
- 16-chunk producer ring capacity: about 185.8 ms.
- Low-water mark: 2 device chunks.

The Windows worker requests `THREAD_PRIORITY_HIGHEST`; this only affects host delivery and does not alter Dreamcast guest time.

## Starvation policy

Low latency and unlimited buffering are mutually incompatible. The live path therefore chooses freshness:

1. If the emulator briefly stops producing PCM while WinMM approaches its low-water mark, the audio worker inserts zero PCM to keep the device running instead of allowing a hard underrun/restart.
2. Synthetic silence is **host playback only**. It is never inserted into the deterministic/capture WAV.
3. If host-clock catch-up produces more live PCM than the bounded ring can hold, the oldest host-playback chunk is dropped. The capture WAV is still untouched.
4. The target is that a newly generated interactive beep stays near the front of a ~46 ms device queue instead of sitting behind seconds of stale audio.

This means an extremely slow emulator can still produce an audible late response corresponding to the actual emulation stall, but it should no longer turn one stall into a multi-second WinMM rebuffer train.

## New diagnostics

Heartbeat fields on Windows:

```text
audio-ring=N/16
audio-starves=N
audio-overruns=N
producer-gap-ms=N
winmm-gap-ms=N
```

Final WinMM report:

```text
[AICA WinMM]
chunks=...
chunk-ms=11
prefill-ms=34
device-target-ms=46
ring-cap-ms=185
min-queue=...
max-queue=...
max-ring=...
producer-gap-max-ms=...
winmm-gap-max-ms=...
audio-starves=...
silence-chunks=...
audio-overruns=...
underrun-restarts=...
```

Interpretation:

- `producer-gap-max-ms`: longest gap between AICA PCM chunks produced by the emulator.
- `winmm-gap-max-ms`: longest gap between chunks submitted by the dedicated worker to WinMM.
- `audio-starves`: worker had to bridge a real producer shortage with host-only silence.
- `audio-overruns`: the bounded live ring was full and stale host-playback PCM was discarded.
- `underrun-restarts`: WinMM actually reached zero queued buffers despite the worker. The target is zero.

## Acceptance target on Windows

For `run_homebrew_sfx_live.bat`:

- A/B/X/Y and repeated rapid presses should remain responsive.
- `underrun-restarts` should ideally remain `0`.
- `winmm-gap-max-ms` should be dramatically smaller than the old producer gap.
- `audio-starves` may be non-zero when the emulator is momentarily behind; that is now an explicit observable condition instead of a hidden rebuffer.
- `audio-overruns` should normally remain `0`. A non-zero value means host-clock catch-up outran real-time playback and the freshness policy was used.

Physical WinMM latency still requires the user's Windows machine. The Linux regression path validates generated C++ and deterministic AICA output, not the Windows audio device.
