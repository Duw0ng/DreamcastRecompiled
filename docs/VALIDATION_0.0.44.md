# DreamcastRecomp 0.0.44 — validation

## Commercial closure

Input used locally: the user-supplied ChuChu Rocket! CDI. Commercial bytes are not part of the source package.

`dc_raw_recomp` final checkpoint:

```text
Synthetic entries:       875
Reachable functions:     907
Call-graph edges:         1424
External/unresolved:      388
Reachable instructions:  60620
Known SH-4:               60620
Unknown SH-4:                 0
CFG blocks:               10596
DCIR ops:                 61860
RAW_SH4:                      0
Inline BSRF thunks:          11
Literal callback targets:     1
Callback-object targets:      4
Relocatable templates:        3
Argument callbacks:          18
```

The generated Linux runner compiled successfully and a bounded commercial run reproduced the following progression:

```text
[GDROM] request #1 ... cmd=0x1E
[GDROM] request #2 ... cmd=0x1F
[DCR code-reloc] 0x8C00FA00 -> template 0x8C14094C
...
[CALL] 0x8C140A84 -> 0x8C13E090
...
[CALL] 0x8C1090FA -> 0x8C1414DE
[DreamcastRecomp ERROR] No recompiled/native target registered for Dreamcast address 0x8C1414DE
```

The run accumulated about 2.16 million guest-call trace entries before the new boundary. `0x8C1414DE` is inside the static commercial image and begins with clean SH-4 code; it is intentionally not seeded by title-specific address.

## AICA note

The same commercial run currently reports:

```text
pc=0x00200000
faults=1
last=AICA ARM7 unmapped access @ 0x200000
```

Therefore 0.0.44 does not claim clean commercial AICA execution. The SH-4/PVR/GD-ROM closure can continue independently, but this fault remains on the roadmap.

## Regression

```text
CTest:                         47/47 PASS
KallistiOS ELF found/loaded:   155/155
ISA-clean:                     155/155
Symbolized functions:          202431
Known SH-4 instructions:       17953662
Unknown SH-4 instructions:     0
_main RAW_SH4=0:               155/155
```

## Windows acceptance

Run:

```bat
build_windows.bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

Expected 0.0.44 baseline is to pass `0x8C0F05A0`, print a `DCR code-reloc` line for the copied VBR handler, reach `0x8C13E090`, and then stop at the next unresolved target (locally `0x8C1414DE`) unless Windows timing exposes another dependency first.
