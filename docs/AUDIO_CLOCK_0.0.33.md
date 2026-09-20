# DreamcastRecomp 0.0.33 — PVR/AICA live clock bridge

## Windows symptom from 0.0.32

The Windows validation run reported:

```
[AICA WinMM] chunks=2101 | chunk-ms=46 | prefill-ms=1114 | min-queue=0 | max-queue=25 | underrun-restarts=29
```

The music itself was continuous and the calibrated tempo was correct, but playback repeatedly became silent and then resumed at the correct musical position.

The key clue is `max-queue=25` with a startup prefill of 24 chunks. The producer never built a meaningful reserve after startup. The queue was not being corrupted; it was being consumed faster than the SH-4-instruction-driven AICA scheduler replenished it during light frames / PVR host pacing.

## Root cause

0.0.32 advanced ARM7/AICA primarily from `dc_runtime_tick(runtime, sh4_instructions)`.

That is useful for early integration, but SH-4 instruction count is not elapsed Dreamcast time. Two 60 Hz video frames may execute very different amounts of SH-4 code while the physical AICA continues to advance continuously at the same rate.

With `--pvr-frame-sync`, host presentation also deliberately waits for a frame deadline. During that host pacing interval the old scheduler generated no additional AICA time. This turns AICA production into bursts and deficits even though WinMM consumes exactly 44,100 stereo frames per second.

## 0.0.33 bridge

`--aica-pvr-sync` adds a live-only minimum clock budget from the displayed PVR frame cadence.

At 60 Hz:

```
44100 / 60 = 735 AICA sample frames per displayed frame
```

The existing SH-4-driven scheduler is retained so ARM7 bootstrap and SH-4/AICA communication still work. PVR sync never removes or rewrites already generated audio. It keeps a cumulative target and only advances ARM7/AICA when the SH-4 path is behind that target.

Therefore:

- if SH-4 scheduling already generated enough AICA time, the bridge does nothing;
- if a light frame generated too little, the bridge fills exactly the deficit;
- if SH-4 ran ahead, the cumulative target catches up over later frames rather than adding extra audio.

## Real 2ndMix validation

With the supplied 2ndMix ELF and the existing 16/21 legacy-player timer calibration:

### 5-second run

```
mix-frames=220500
pvr-sync-topups=295
pvr-sync-frames=87369
faults=0
formats-unsupported=0x0
bad-reads=0
```

87,369 of 220,500 frames (~39.6%) had to be supplied by the PVR clock bridge. This demonstrates that the previous SH-4 instruction proxy was substantially under-producing live audio time in this workload.

### 15-second run

```
mix-frames=661500
pvr-sync-topups=895
pvr-sync-frames=215458
fiq=50358
faults=0
formats-unsupported=0x0
bad-reads=0
```

Most importantly, the 15-second WAV generated with PVR/AICA sync has the exact same SHA-256 as the known-good 0.0.32 tempo-correct capture:

```
971d4c79bbee59f6d9769fef4bbc0d69736175d4da1c9e74264c4b620b87d453
```

So the bridge changes *when* the audio is produced for live delivery, not the PCM sequence itself.

## WinMM diagnostics

0.0.33 adds `max-submit-gap-ms` to the final WinMM line:

```
[AICA WinMM] ... | min-queue=N | max-queue=N | max-submit-gap-ms=N | underrun-restarts=N
```

The desired result is:

```
min-queue > 0
underrun-restarts = 0
```

If underruns remain, `max-submit-gap-ms` identifies whether the producer is still pausing long enough to drain the device queue.

## Scope

This is an integration bridge, not the final Dreamcast clock model. The long-term design remains one common virtual timeline derived from hardware clocks rather than SH-4 instruction count or host wall time. The bridge is useful now because it fixes the specific category error that caused live audio starvation without changing deterministic PCM output.
