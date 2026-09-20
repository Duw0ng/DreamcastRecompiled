# DreamcastRecomp 0.0.45 — validation

## Core regression suite

Release source was configured and built on Linux in Release mode, then the complete CTest suite was executed.

```text
47/47 tests passed
0 failures
```

The 0.0.45 C++ emitter tests include a dedicated `FPSCR.SZ=1` odd-register FMOV check and verify that generated code uses the DR/XD-aware `dc_set_fmov64_bits` / `dc_get_fmov64_bits` helpers rather than the former runtime trap.

## Exact supplied KallistiOS corpus

The exact demo archive supplied for the project was re-extracted and scanned with the 0.0.45 corpus scanner.

```text
ELF encontrados:                155
ELF cargados:                   155
ELF ISA-clean (all functions):  155
Funciones escaneadas:           202431
Instrucciones conocidas:        17953662
Instrucciones desconocidas:     0
_main call graphs escaneados:   155
_main con RAW_SH4=0:             155
```

This remains a static ISA/DCIR regression, not a claim that every KallistiOS runtime service is complete.

## ChuChu Rocket! symbol-free closure

The supplied retail CDI is used only as local test input and is not redistributed. `dc_disc_probe` extracts the local boot data and emits the external `disc.map`; `dc_boot_prepare` creates the combined IP.BIN + boot image used by `dc_raw_recomp`.

Final 0.0.45 closure:

```text
Closure passes:         23
Synthetic entries:      1453
Reachable functions:    1453
Call-graph edges:       2282
External/unresolved:    189
Reachable instructions: 83588
Known SH-4:             83588
Unknown SH-4:           0
CFG blocks:             14814
DCIR ops:               85423
RAW_SH4:                0
Closure resolved calls: 48395
Closure unresolved:     2704
Rejected nonlocal:      325
Inline BSRF thunks:     11
Literal callbacks:      3
Callback objects:       111
Relocatable templates:  3
Argument callbacks:     33
Promoted CFG fragments: 48
VBR vector callbacks:   3
Sparse callback targets:11
```

A fresh generated Debug runner compiled successfully after the final version update.

## Commercial execution checkpoint

The final local acceptance run used the original CDI through `disc.map`, `--commercial-boot`, real BIOS GD-ROM HLE, ARM7 enabled and the host-synchronized device clock.

The run passes the previous 0.0.44 boundary `0x8C1414DE`, several later symbol-free callback/handler boundaries and the former `FPSCR.SZ=1` XD-register FMOV failure.

Observed GD-ROM progress before the next boundary:

```text
requests=7
execs=7
sector-reads=2
bytes=4096
current-fad=170
```

The final trace ends at:

```text
[CALL] 0x8C0DB6C6 -> 0x8C0FD600
[CALL] 0x8C0FD600 -> 0x8C138620
[DreamcastRecomp ERROR] No recompiled/native target registered for Dreamcast address 0x8C138620 (guest_pc=0x8C0FD600)
```

The call counter at this boundary is approximately 12,676,230 guest calls.

Static inspection shows `0x8C0FD600` obtains its target from an indexed table whose base is loaded from a PC-relative literal pointing near `0x8C185D58`. Consecutive entries include `0x8C138620`, `0x8C138724`, `0x8C138920`, `0x8C138A84`, `0x8C138C20`, `0x8C138DA0`, `0x8C1390A0`, `0x8C139380`, and `0x8C139620`. These are the next general dense-dispatch discovery target, not title-specific seeds.

## Known commercial ARM7 issue

Commercial ARM7/AICA is not yet clean. The same run records:

```text
pc=0x00200000
faults=1
halted=yes
last=AICA ARM7 unmapped access @ 0x200000
```

This remains an explicit 0.0.46+ investigation item. It does not invalidate the SH-4/GD-ROM closure checkpoint, but 0.0.45 does not claim working commercial sound.
