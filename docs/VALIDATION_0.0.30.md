# Validation — DreamcastRecomp 0.0.30

## Core build/tests

```text
CMake Release build: PASS
CTest:              47/47 PASS
ARM7 unaligned LDR regression: PASS
```

## Real 2ndMix ELF

Input used for integration: the supplied real KallistiOS 2ndMix ELF.

```text
Root:                _main @ 0x8C0110D0
Reachable functions: 181
Call-graph edges:     376
CFG blocks:           2524
DCIR operations:      14586
RAW_SH4:              0
Generated project:    PASS
Generated smoke test: PASS
```

## Native AICA five-second capture

Run profile:

```text
--probe-video-default
--probe-kos-aica-defaults
--aica-arm7
--aica-sh4-div=1
--probe-no-input
--aica-capture-ms=5000
```

Observed result:

```text
Start
Done
Starting display
instructions=6836635
fiq=22026
timerA=22026
faults=0
idle-skipped=218955363
native-starts=171
mix-frames=220500
nonzero=213641
formats-unsupported=0x0
bad-reads=0
```

Host elapsed time in the Linux Release test: about 3.5 seconds for 5.0 seconds of generated PCM using the temporary live-throughput divisor.

WAV SHA-256 used for this milestone:

```text
6ecbdd93bcd028a5aaeb76696ad52aaedbac81c5d1f8cd0d6bbf3936bd29a760
```

PCM measurements over 220,500 frames:

```text
mean L/R:   -72.92 / -98.99
RMS L/R:   1866.50 / 1369.00
peak L/R:  13829 / 14316
clipping:  0 samples at int16 rails
non-zero:  96.889% frames
```

## Current limitations

- `--aica-sh4-div` remains an instruction-ratio bridge, not a common clock-domain scheduler.
- Windows WinMM live output is emitted but cannot be compiled/listened to in the Linux validation environment.
- AICA envelope generators, LFO, filters, ADPCM and DSP are not yet complete.
- `--probe-kos-aica-defaults` and `--probe-video-default` remain startup scaffolding because generated programs enter `_main` directly.
- Audio is substantially corrected, not claimed bit-perfect to physical Dreamcast AICA.
