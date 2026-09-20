# DreamcastRecomp 0.0.34 — validation

## Core suite

```text
CTest: 47/47 passed
```

## Real supplied 2ndMix ELF

Code generation:

```text
reachable functions: 181
RAW_SH4: 0
embedded ELF data: 556612 bytes
```

Generated standalone project:

```text
dreamcast_generated: builds
generated_compile_test: passes
dreamcast_program: builds
```

### Deterministic common clock, 5 seconds

Options of interest:

```text
--aica-arm7
--aica-timer-rate=16/21
--device-clock
--aica-capture-ms=5000
```

Observed:

```text
Start
Done
Starting display
mix-frames=220500
fiq=16786
faults=0
formats-unsupported=0x0
bad-reads=0
pvr-sync-topups=0
pvr-sync-frames=0
dc-clock=virtual
dc-pvr-anchors=296
```

The resulting 5-second PCM payload is byte-identical to the first five seconds of the validated 0.0.33/0.0.32 tempo-correct capture.

### Host common clock, 5 seconds

Options of interest:

```text
--aica-arm7
--aica-timer-rate=16/21
--device-clock-host
--aica-capture-ms=5000
```

Observed on the Linux validation host:

```text
wall time: ~5.18 s
mix-frames=220500
fiq=16786
faults=0
formats-unsupported=0x0
bad-reads=0
dc-clock=host
dc-host-syncs=2067
dc-host-catchup=227274832
```

The host-clock and deterministic-clock WAV files are byte-identical.

## Windows acceptance test

Use:

```bat
run_homebrew_2ndmix_live.bat path\to\2ndmix.elf
```

The helper no longer supplies `--aica-sh4-div` or `--aica-pvr-sync`. It uses:

```text
--device-clock-host
--aica-timer-rate=16/21
--aica-play
--pvr-window
--pvr-frame-sync
```

Primary acceptance signals:

```text
underrun-restarts=0   (target)
min-queue > 0         (ideal steady-state target)
```

Even if an isolated host scheduling stall still causes a recovery, AICA tempo/progression must remain continuous and must not depend on actual rendered FPS.
