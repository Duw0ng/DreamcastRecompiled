# DreamcastRecomp 0.0.36 — validation

## Main suite

```text
47/47 CTest tests passed
```

## 155-ELF KallistiOS corpus

```text
ELF found:                     155
ELF loaded:                    155
ELF emitted with defaults:    155
ISA-clean all functions:      154
_main with RAW_SH4=0:         154
Unknown instruction words:      3
```

Only `lua/basic/lua.elf::_llex` contains the three remaining unknown words (`0x0118`, `0x0119`, `0x011B`).

## Real 2ndMix regression

Final 0.0.36 generation:

```text
Reachable functions: 190
RAW_SH4: 0
Start
Done
Starting display
```

Five-second deterministic native AICA capture:

```text
mix-frames=220500
fiq=16786
timer-rate=16/21
faults=0
formats-unsupported=0x0
bad-reads=0
pvr-sync-topups=0
pvr-sync-frames=0
dc-clock=virtual
```

SHA-256:

```text
0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567
```

The reachable count rose from the older 181-function direct-call graph because 0.0.36 intentionally includes address-taken `STT_FUNC` targets. The audio bytes did not change.

## Bumpmap regression

With the read-only ROMFS stdio bridge and deterministic controller A/Start probe:

```text
---KallistiOS PVR Bumpmap Example---
...
[DreamcastRecomp] returned to host
[PVR bootstrap] ... TA packets=20 ... logical-frames=1 ... render-done=1 ... vblanks=1 ...
```

This confirms that the previous `fopen/fread` asset-load failure is removed.

## Standard KOS sound-driver diagnostic

After making the KOS 10 ms spin sleep advance device time, `sound/sfx` reaches the standard firmware queue check but still reports:

```text
KOS assertion failed: g2_read_32_raw(qa + offsetof(aica_queue_t, valid))
at snd_iface.c:84 in snd_sh4_to_aica: Queue is not yet valid
```

That result is retained as the next native ARM/AICA correctness target rather than bypassed with a false queue-valid value.
