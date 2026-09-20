# Native AICA audio architecture — 0.0.29

## Purpose

0.0.29 turns the ARM7/AICA foundation from 0.0.27/0.0.28 into an audible native path capable of running the supplied 2ndMix S3M player.

The architecture remains deliberately split from `--aica-kos-hle` so a successful native test cannot accidentally be produced by HLE commands.

## Device flow

```text
recompiled SH-4 basic blocks
        |
        v
runtime scheduler budget
        |
        +------> ARM7TDMI interpreter
        |             |
        |             +--> Timer A/B/C
        |             +--> FIQ/IRQ
        |             +--> AICA register writes
        |
        v
AICA 64-slot state
        |
        +--> read PCM from 2 MiB AICA RAM
        +--> pitch / loop / pan / total level
        v
44.1 kHz stereo PCM mixer
        |
        +--> WAV capture
        +--> WinMM waveOut on Windows
```

## Slot state currently modeled

Per slot:

```text
playing/key state
register 0 control
sample address
loop start
loop end
pitch
pan
total level
sample format
loop enable
fractional playback position
start counter
```

Formats implemented:

```text
0 = PCM16
1 = PCM8
```

2ndMix did not request another format in the validated five-second run. ADPCM remains required for broader compatibility.

## Pitch

The current native mixer interprets the AICA pitch register as exponent + 10-bit mantissa and converts that into a fractional step relative to 44.1 kHz.

This is functionally sufficient for the tracker test but should be measured against hardware/reference emulators before it is treated as fidelity-complete.

## Pan and total level

The mixer applies:

- approximate logarithmic total-level attenuation;
- hardware-style pan side/magnitude decoding;
- independent left/right gains.

The current implementation intentionally prioritizes audible functional behavior over exact analog/DSP output equivalence.

## Looping and monitor position

Playback position advances continuously. On a looping slot the position returns to LSA when it reaches LEA. Non-looping slots stop at the end.

The common AICA monitor-position window is updated from the selected slot so ARM software that queries sample position can observe progress.

## Timing

### ARM scheduling

Every emitted SH-4 basic block advances the runtime using its instruction count. A configurable ratio converts SH-4 progress into ARM execution budget:

```text
--aica-sh4-div=N
```

This solves the major functional problem where ARM playback stopped after SH-4 code left an AICA access path.

It remains an instruction-ratio scheduler, not a Dreamcast cycle scheduler.

### Audio clock

A stereo output frame is generated periodically from ARM/device progress at 44.1 kHz. AICA timers are advanced from the same runtime path.

The current default Timer A divider was selected to produce the expected tracker FIQ cadence in the 2ndMix firmware. 0.0.30 should move toward explicit CPU/device frequencies and accumulated cycles.

## KOS startup profile

Runner option:

```text
--probe-kos-aica-defaults
```

seeds the pre-`_main` sound state normally established by KallistiOS startup, including Timer A and sound interrupt routing.

This option exists because current generated execution enters `_main` directly. The long-term goal is to represent normal console/runtime startup well enough that this profile becomes unnecessary.

## Embedded ROMFS bridge

The emitter detects `_romdisk_data` and can emit a tiny read-only parser for the ROMFS image. It handles the file-access subset currently needed by 2ndMix:

```text
/rd/... open
read
close
```

This makes the generated program self-contained with respect to data already embedded in its own ELF image and avoids hard-coding the module bytes.

Future VFS work should add seek/stat/directory APIs and a more general mount abstraction.

## Live Windows backend

Generated Windows projects link:

```text
user32
gdi32
winmm
```

`--aica-play` uses `waveOut` with bounded PCM chunks. The queue provides backpressure so host execution is naturally kept near playback consumption instead of racing arbitrarily ahead.

The code is conditionally compiled under `_WIN32`. Linux milestone validation used WAV capture, so MSVC/WinMM is the first host-specific validation item for the next iteration.
