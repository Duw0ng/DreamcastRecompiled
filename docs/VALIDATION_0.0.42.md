# DreamcastRecomp 0.0.42 — validation

## Core tests

Linux CTest result:

```text
47/47 PASS
```

## KallistiOS corpus

Exact supplied corpus:

```text
ELF found:                    155
ELF loaded:                   155
ISA-clean:                    155
Symbolized functions:         202431
Known SH-4 instructions:      17953662
Unknown SH-4 instructions:    0
_main call graphs scanned:    155
_main RAW_SH4=0:              155
```

`ISA-clean` is an instruction/IR statement only; it does not claim every demo is already behaviorally complete.

## Commercial boot preparation

For the supplied ChuChu Rocket! CDI, IP.BIN identifies a GD-ROM image. `dc_boot_prepare --mode=auto` selected pass-through and the prepared boot matched the extracted disc boot byte-for-byte.

The self-boot transform was also cross-checked in forced `--mode=selfboot` mode against the existing reference output; it matched exactly.

## Commercial static closure

Combined image (`IP.BIN + retail 1ST_READ.BIN`):

```text
base:                       0x8C008000
bootstrap entry:            0x8C008300
future game seed:           0x8C010000
Synthetic entries:          774
Reachable functions:        806
Reachable SH-4:              53925
Known:                       53925
Unknown:                     0
CFG blocks:                  9486
DCIR ops:                    53742
RAW_SH4:                     0
```

## Commercial runtime

The generated project builds natively. Execution with `--commercial-boot --trace-calls` advances through the IP.BIN bootstrap, P2 continuation, on-chip RAM use and local BSRF thunk dispatch, and deliberately stops at the first unimplemented BIOS GD-ROM service:

```text
[CALL] 0x8C00DBE0 -> 0x8C001006
[DreamcastRecomp ERROR] Dreamcast BIOS GD-ROM syscall reached before GD-ROM HLE is implemented
(r4=0x1E r5=0x8CFFFFF4 r6=0x0 r7=0x0)
```

This is the acceptance frontier for 0.0.42 and the starting point for 0.0.43.

## Release hygiene

The source release contains no CDI, IP.BIN, 1ST_READ.BIN, generated commercial C++, commercial maps, or other extracted game bytes. All commercial artifacts are generated locally from a user-supplied disc image.
