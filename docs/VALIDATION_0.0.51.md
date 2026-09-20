# DreamcastRecomp 0.0.51 — validation

## Unit/regression gate

Final release gate: **47/47 CTest PASS**.

## Commercial ChuChu Rocket! gate

Static closure remains unchanged from 0.0.50:

- reachable functions: **2,194**;
- known SH-4 instructions: **165,555**;
- unknown SH-4 instructions: **0**;
- `RAW_SH4`: **0**.

Runtime acceptance after TMU implementation:

- the former deterministic freeze at **5,185 packets / 144 frames** is passed;
- bounded probe: **6,500 packets / 256 frames / 256 renders / 255 page flips**;
- final bounded counters include **772 list-end IRQs** and **257 guest STARTRENDER/ISP starts**;
- GD-ROM reaches request 31 and the `0x15` PLAY_SECTORS path;
- CDDA reaches `done/terminated` instead of remaining in PLAY;
- probe terminates only because `--pvr-stop-after-packets=6500` intentionally fires;
- longer run exceeds **8,600 packets / 449 frames** without a guest error before external timeout.

## SH-4 TMU

0.0.51 adds runtime-backed TMU0-TMU2 `TSTR/TCOR/TCNT/TCR`, guest-cycle decrementing for internal TPSC clock selections 0-4, reload-on-underflow and `TCR.UNF`. External clock sources and full timer-underflow IRQ delivery remain explicit gaps.

The ChuChu wait configures TMU0 with `TCOR0=TCNT0=0xFFFFFFFF`, `TCR0=2`, starts channel 0 and polls `TCNT0`. Heartbeat confirms the counter now decrements continuously.

## KallistiOS corpus

Static analysis/CFG/DCIR did not change in 0.0.51, so the already-generated 0.0.50 cached-corpus result remains the applicable static regression without re-extracting or rescanning the large source archive:

```text
ELF found / loaded:           155 / 155
ISA-clean:                    155 / 155
Known SH-4 instructions:      17,953,662
Unknown SH-4 instructions:    0
_main RAW_SH4=0:              155 / 155
```

The historical 0.0.50 corpus report is retained rather than duplicated byte-for-byte under a new filename.

## Audio regression

The 0.0.50 AICA clock/catch-up model remains unchanged by TMU. A fresh real KallistiOS `sound/sfx` run after the 0.0.51 runtime changes completed cleanly:

```text
runner RC=0
reachable functions=182
RAW_SH4=0
ARM7 faults=0
native-starts=1
mix-frames=3366
nonzero PCM frames=2602
unsupported AICA formats=0
bad AICA reads=0
```

A freshly regenerated commercial C++ project reports the 0.0.51 banner, preserves the 2,194 / 165,555 / 0 / RAW=0 closure, compiles successfully, and its `generated_compile_test` exits **RC=0**.

## Packaging

The archive excludes `build/`, `generated/`, CDI contents, `IP.BIN`, `BOOTSTRAP.BIN`, commercial generated C++, logs and framebuffer dumps. A SHA-256 duplicate-content pass is performed on staging before ZIP creation.
