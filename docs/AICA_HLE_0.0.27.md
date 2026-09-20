# AICA audio validation — DreamcastRecomp 0.0.27

The standard KallistiOS SH-4/AICA command queue HLE remains available as a compatibility and regression path. 0.0.27 also adds a separate native ARM7 bootstrap (`--aica-arm7`); this document describes only the HLE reference path.

Supported initial path:

```text
KOS SH-4 code
  -> snd_sh4_to_aica
  -> AICA RAM queue
  -> host queue consumer
  -> START / STOP / UPDATE
  -> PCM16 / PCM8
  -> WAV / optional WinMM playback
```

Available options:

```text
--aica-kos-hle
--aica-wav=PATH
--aica-play
--aica-stop-after-starts=N
```

## Low-level queue test

`run_kos_aica_sfx_hle.bat` recompiles the real KOS `_snd_sh4_to_aica`, places a deterministic PCM16 fixture in sound RAM and verifies one channel START command.

## Full KOS SFX application-path probe

`run_kos_sfx_full_probe.bat` starts at the real `sound/sfx/sfx.elf::_main`, uses a deterministic virtual controller A press, and allows the real recompiled chain:

```text
_main
 -> snd_sfx_play
 -> snd_sfx_play_ex
 -> snd_sh4_to_aica
 -> AICA queue
 -> host PCM output
```

The only probe-specific substitution is **asset loading**: because DreamcastRecomp does not yet run the complete KOS startup/romdisk mount, `_snd_sfx_load` receives a deterministic PCM fixture instead of `/rd/beep-1.wav`. The fake handle is explicitly probe-only; the playback path after that handle is real SH-4 KOS code.

The probe stops immediately after the first AICA channel start, produces a 0.20 s, 22.05 kHz stereo PCM16 WAV, and uses WinMM for audible playback on Windows.

## 2ndMix

2ndMix is different: it uploads its own `s3mplay` ARM binary and releases the AICA ARM7. 0.0.27 now has the first native ARM7 execution core and AICA bus, but authentic tracker playback still requires the missing interrupt/timer/channel/mixer timing pieces (and Thumb if the blob reaches Thumb state). The project continues to prefer that native ARM7 route over a special-purpose S3M HLE.
