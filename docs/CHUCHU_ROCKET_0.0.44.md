# ChuChu Rocket! commercial status — 0.0.44

0.0.44 is a closure/interrupt-relocation checkpoint. It deliberately keeps the commercial image external and uses the supplied CDI only during local generation/testing.

## What moved forward

The 0.0.43 Windows boundary `0x8C0F0596 -> 0x8C0F05A0` is resolved without a game-address seed. The target is recovered from a literal-referenced callback object whose first words form a dense callback table.

Katana later copies exception/interrupt code from the executable into VBR-relative RAM. The first observed destination, `0x8C00FA00`, is not code in the original static image, so the runtime now matches its copied bytes uniquely to the recompiled source template `0x8C14094C` and dispatches the native template.

The next registered handler, `0x8C13E090`, is recovered as a function pointer passed in an argument register to a registration helper. Execution enters it and continues until the new boundary:

```text
0x8C1090FA -> 0x8C1414DE
```

`0x8C1414DE` is real local SH-4 and is loaded immediately before an indirect call, but that literal lives in a reachable CFG fragment not yet promoted by the function-level closure logic. This is the next discovery problem.

## Current subsystem notes

GD-ROM HLE still passes the initial REQ_MODE/SET_MODE sequence. The dynamically relocated SH-4 interrupt path is now exercised. The commercial ARM7 path still faults at `0x00200000`, so commercial audio is not considered solved. PVR remains an active target after closure/interrupt delivery is stable enough to keep the game running.

## Rule for future fixes

Do not add `0x8C1414DE` as a ChuChu-specific seed. The next version should make CFG-fragment literal callbacks visible to the generic raw closure, then run until the next actual hardware/runtime dependency.
