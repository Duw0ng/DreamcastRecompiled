# 2ndMix native audio integration — DreamcastRecomp 0.0.29

## Input

Validation used the supplied real KallistiOS `2ndmix.elf`, not a synthetic test program.

Important ELF content discovered during analysis:

```text
_main       0x8C0110D0
_play_s3m   0x8C010544
_s3mplay    0x8C091AAC  size 11200 bytes
_romdisk_data present
/rd/e-79014.s3m present inside ROMFS
```

The ARM blob is embedded in the ELF as the `s3mplay` byte array. The music module is embedded in the ROM disk.

## Real software flow

The SH-4 side:

1. disables the SPU;
2. opens `/rd/e-79014.s3m`;
3. copies the module to AICA RAM starting at `0x10000`;
4. copies `s3mplay` to AICA RAM address `0`;
5. enables the SPU / releases ARM reset;
6. waits on the `snd_dbg` handshake in AICA RAM;
7. continues to the display loop after the ARM player advances the handshake.

The ARM side uses Timer A FIQs to advance the tracker and programs AICA slots directly. It does not need the standard KOS command-queue HLE for this music path.

## Blockers found and fixed

### 1. ARM execution was access-driven

Before 0.0.29 the ARM mainly advanced when SH-4 code accessed AICA state. That is insufficient once the SH-4 returns to its graphics loop.

Fix: generated SH-4 basic blocks feed an ongoing ARM execution budget.

### 2. Timer/interrupt state was missing at `_main`

DreamcastRecomp currently begins directly at `_main`. KallistiOS' earlier sound bootstrap therefore had not populated the persistent AICA timer/interrupt registers that 2ndMix expects to inherit when it replaces the ARM program.

Fix: explicit `--probe-kos-aica-defaults` startup profile. It seeds the known KOS Timer A / SCIEB / SCILV state before guest `_main` starts.

This is bootstrap scaffolding, not a 2ndMix-specific song hack.

### 3. The S3M was not reaching AICA RAM

The ARM program itself was copied correctly because `_s3mplay` is ordinary ELF data. However, the S3M is obtained through the embedded romdisk. Direct `_main` execution had bypassed the normal KOS mount environment, leaving the region at AICA RAM `0x10000` empty.

Fix: generated-project ROMFS bridge. If `_romdisk_data` is present, `/rd/...` `fs_open/fs_read/fs_close` calls can be served from the ELF's own read-only ROMFS image.

After this fix the real module bytes reached AICA RAM and channel programming changed from zero/invalid source parameters to meaningful sample addresses and lengths.

### 4. Native AICA slots/mixer were missing

Fix: 64-slot native playback state with PCM16/PCM8, base address, loop range, pitch, pan, total level, position and stereo mixing.

## Final five-second test

Command used in the Linux validation environment:

```text
./dreamcast_program \
  --probe-video-default \
  --probe-no-input \
  --probe-kos-aica-defaults \
  --aica-arm7 \
  --aica-wav=2ndmix_0029_final.wav \
  --aica-capture-ms=5000 \
  --aica-sh4-div=4
```

Observed guest output:

```text
2ndMix/KallistiOS starting
Initializing new PVR system
Initializing stars
Init font
Image is 256x256 (65536 bytes)
Drawing into 0xa4156020
Loading music
Loading /rd/e-79014.s3m
Loading ARM program
Start
Done
Starting display
```

ARM/AICA summary:

```text
instructions=225792000
slices=95862195
pc=0xCC
reset=no
halted=no
fiq=22026
irq=0
timerA=22026
timerB=861
timerC=861
faults=0
native-starts=67
mix-frames=220500
nonzero=213134
formats-unsupported=0x0
bad-reads=0
```

### WAV properties

```text
Duration:             5.000 s
Sample rate:          44100 Hz
Channels:             2
Format:               signed PCM16
Frames:               220500
Non-zero frames:      213134 (96.66%)
Peak L/R:             22110 / 21733
RMS L/R:              ~4093 / ~3798
```

The capture contains sustained stereo signal and no unsupported AICA sample format or out-of-range sample read was observed during this window.

This is enough to call **native 2ndMix audio bootstrap working**. It is not enough to claim bit-exact Dreamcast output: interpolation, precise attenuation/pan curves, key-on semantics, envelopes and scheduler/cycle timing still need fidelity work.
