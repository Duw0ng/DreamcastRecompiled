# DreamcastRecomp 0.0.48 — validation

## Core regression

```text
CTest: 47/47 PASS
```

## KallistiOS corpus

The exact cached 155-ELF regression corpus was rescanned with the final 0.0.48 tools:

```text
ELF found/loaded:              155/155
ISA-clean:                    155/155
Symbolized functions:         202431
Known SH-4 instructions:    17953662
Unknown SH-4 instructions:         0
_main RAW_SH4=0:              155/155
```

## Commercial symbol-free closure — ChuChu Rocket!

The supplied retail CDI is used only as a local validation input and is not redistributed. The final 0.0.48 closure is generated from the normal combined `IP.BIN + boot` image with the commercial entry as the only manual seed.

```text
Synthetic entries:        2194
Reachable functions:      2194
Call-graph edges:          3956
External/unresolved:        404
Reachable instructions:  165555
Known SH-4:              165555
Unknown SH-4:                 0
CFG blocks:               26271
DCIR ops:                169240
RAW_SH4:                      0
Stored callback targets:     22
Argument callbacks:          95
Dense dispatch targets:     219
```

## Native commercial acceptance

The prior 0.0.47 state initialized PVR but never delivered TA geometry. 0.0.48 passes the G2/AICA wait, preserves registers across dynamically relocated Katana IRQ code, and enters sustained TA/PVR submission.

A bounded headless run reached the following heartbeat high-water marks before the external host timeout stopped the process:

```text
sq-writes:      2646
pref:            389
pref-ta:         301
pvr-packets:     429
pvr-frames:        9
pvr-renders:       9
pvr-flips:         9
```

No `DreamcastRecomp ERROR` or unresolved native target was observed in that bounded interval.

## Remaining limitations

- The observed retail path still reports `isp-start=0`; current logical scene/list completion is therefore responsible for the advancing render/frame lifecycle.
- Commercial ARM7 execution reaches real uploaded firmware but still halts on the unsupported `LDM/STM ^` user-bank transfer form.
- Headless validation proves the render pipeline advances; it does not by itself claim that the displayed frame is yet visually correct or that ChuChu is playable.
