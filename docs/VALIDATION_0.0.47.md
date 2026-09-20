# DreamcastRecomp 0.0.47 — validation

## Scope

0.0.47 fixes generic raw commercial control-flow/runtime problems exposed by the real ChuChu Rocket! trace: shared/overlapping tails, callback/task-entry closure, full-context non-local returns, and PVR VBlank progress in host-clock mode. It does not add title-address seeds or a runtime SH-4 interpreter fallback.

## Native/unit regression

Final Release build:

```text
47/47 tests passed
0 failures
```

The regression suite includes the shared-tail raw-function case and generated CALL tests that distinguish an ordinary return/HLE call from a callee that intentionally restores a different Dreamcast PC.

## Exact supplied KallistiOS corpus

The cached regression set contains only the 155 ELF files from the supplied KallistiOS archive, avoiding repeated decompression/hashing of unrelated resources. Final 0.0.47 scan:

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

## Standard KallistiOS sound/sfx runtime

A freshly generated 0.0.47 runner from the supplied `sound/sfx/sfx.elf` was configured, built and executed through the real standard ARM7/AICA path:

```text
Reachable functions:     182
RAW_SH4:                 0
runner return code:      0
ARM7 faults:             0
ARM7 releases:           2
native-starts:           1
mix-frames:              3302
nonzero:                 2696
formats-unsupported:     0
bad-reads:               0
```

The generated compile test also exits successfully. This confirms that the CALL/context-switch and PVR clock changes did not regress the established standard-KOS audio path.

## Final ChuChu static closure

The 0.0.47 `dc_raw_recomp` run against the externally extracted bootstrap reports:

```text
Closure passes:              23
Synthetic entries:           1630
Reachable functions:         1630
Call-graph edges:            2875
External/unresolved:         338
Reachable instructions:      107868
Known SH-4:                  107868
Unknown SH-4:                0
CFG blocks:                  19539
DCIR ops:                    110172
RAW_SH4:                     0
Closure resolved calls:      61570
Closure unresolved:          5205
Argument callbacks:          52
Dense dispatch targets:      77
```

Its generated commercial compile test exits successfully.

## Real commercial runtime acceptance

The final 0.0.47 Debug-generated runner was executed against the external CDI/disc map with Maple host input, native ARM7 and host device clock. The harness captured 36 one-second heartbeats before terminating the diagnostic run; no `DreamcastRecomp ERROR`, unmapped Dreamcast-memory access, or unregistered dynamic target appeared.

Observed progress includes:

```text
GD-ROM requests:         at least 14
largest observed read:   18 sectors (FAD 34901)
code relocations:        2
Maple host polls:        24+
PVR VBlanks:             2098 by heartbeat #36
TA packets:              0
PVR frames:              0
PVR renders:             0
```

The same logic had already survived a longer development diagnostic window; the version-final rerun above is the conservative acceptance record for the packaged source.

The original 0.0.46 `0x50000FAC` crash and the later misleading `0xC8800000` path are not reproduced. Context switching reaches the real task-entry path instead of fabricating the previous `0x8C00F300` dynamic target.

## Known open commercial boundaries

1. PVR VBlank now advances independently of MMIO polling, but the retail path still has not submitted a TA packet/render. Commercial Kamui/TA initialization, Store Queue destination setup and Holly event flow are the next primary investigation.
2. Commercial ARM7 can still be released with blank AICA vector/program state and run zero opcodes to `0x00200000`. The runtime intentionally does not wrap the 2 MiB AICA RAM address; the Katana firmware/upload/reset sequence remains to be traced.
