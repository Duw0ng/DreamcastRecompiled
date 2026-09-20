# DreamcastRecomp 0.0.46 — validation

## Scope

0.0.46 is a symbol-free closure and ARM7-diagnostics checkpoint. It does not add title-specific function seeds and it does not claim a retail frame until the external ChuChu Rocket! CDI is run again on the Windows path.

## Native/unit regression

Release build and CTest on the development host:

```text
47/47 tests passed
```

The new `function_analysis_tests` case constructs a dense callable table selected by:

```text
MOV.L @(table_literal,PC),Rn
MOV.L @(R0,Rn),Rn
JSR @Rn
```

Its three targets each begin with 16 valid SH-4 instructions (32 bytes) and deliberately have no early `RTS`. All three are recovered as dynamic dispatch targets. This protects the generic long-function case that 0.0.45 missed.

## Exact supplied KallistiOS corpus

To avoid repeatedly unpacking unrelated textures/fonts/assets, the regression cache contains only the 155 ELF files from the exact supplied KallistiOS archive. The full scanner result is:

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

This is byte-for-byte the same source corpus baseline used by 0.0.45; the cache only prevents duplicate resource processing.

## Real KallistiOS sound/sfx runtime

A fresh generated runner from the supplied `sound/sfx/sfx.elf` completed normally after the ARM7 instrumentation changes:

```text
runner return code: 0
ARM7 faults:         0
ARM7 releases:       2
native channel starts: 1
non-zero mixed PCM:  yes
```

Representative final diagnostic fields included `release-nonzero=2205`, `release-first-nz=0x0`, `pc-high=0x840`, `zero-run-max=0` and `faults=0`. This confirms the reset snapshot and PC/zero-run tracking do not regress the standard KOS firmware path.

## Retail ChuChu Rocket! acceptance status

The commercial CDI is not part of the source package and was not present in the clean current workspace. Therefore the 0.0.46 package intentionally makes **no false claim** that the 0.0.45 stop

```text
[CALL] 0x8C0FD600 -> 0x8C138620
```

has already passed.

What changed is the generic closure rule needed by the previously observed shape: the dense absolute table around `0x8C185D58` can now contribute callable targets through indexed `JSR/JMP @Rn`, subject to a clean 32-byte code-prefix check. The next Windows retail run is the acceptance test.

## ARM7 commercial fault classification

When the ARM7 leaves reset, the runtime snapshots:

- SH-4 write epochs and total bytes written to AICA RAM;
- number and first location of non-zero bytes;
- first eight 32-bit words at the ARM7 vector area;
- subsequent ARM7 PC high-water, zero-opcode count and longest contiguous zero-opcode run.

If instruction fetch reaches exactly `0x00200000`, the error explicitly says that this is the first byte beyond the 2 MiB AICA RAM and that no implicit RAM wrap was applied. It also marks blank reset vectors and long zero-opcode linear fall-through when the evidence supports those classifications.
