# ChuChu Rocket! commercial progress — 0.0.42

## Why the 0.0.41 failure mattered

0.0.41 called the retail `1ST_READ.BIN` directly at `0x8C010000`. The user run reached `0x8C0DA540` and then attempted to dereference `0x2D2D2D0A`. That was useful evidence: a retail Katana executable assumes the Dreamcast BIOS/IP.BIN bootstrap has already established system RAM, CPU state and BIOS services.

0.0.42 therefore stops treating `1ST_READ.BIN` as a normal standalone ELF-like entry point.

## Correct boot model now used

For the supplied image the IP.BIN reports a GD-ROM device. Its boot file is already executable data and must not be passed through the MIL-CD self-boot descrambler.

The local pipeline is now:

```text
ChuChu Rocket!.cdi
  -> IP.BIN
  -> 1ST_READ.BIN (retail GD-ROM: byte-for-byte)
  -> combined BOOTSTRAP.BIN
       0x8C008000 : IP.BIN
       0x8C010000 : 1ST_READ.BIN
  -> bootstrap PC 0xAC008300 / canonical generated PC 0x8C008300
```

`dc_boot_prepare --mode=auto` makes the GD-ROM/self-boot distinction from IP.BIN and can be overridden explicitly for diagnostic images.

## New runtime pieces exercised by the real bootstrap

The retail IP.BIN forced several generic Dreamcast features that the direct game launch did not model:

- SH-4 `0x7C000000-0x7FFFFFFF` on-chip/operand-cache RAM, used here around `0x7E001000`;
- P2/P1 uncached/cached aliases;
- low-memory BIOS vector table;
- post-BIOS register state (`SP`, `GBR`, `VBR`, `SR`, `FPSCR`, etc.);
- system/font/flash/misc BIOS HLE entry points;
- absolute jump-table targets in symbol-free code;
- PR continuations loaded from literals;
- compact runtime-selected BSRF/RTS helper packs;
- continuation from one generated bootstrap stage into the next Dreamcast PC.

These are reusable console mechanisms, not ChuChu address patches.

## Static recompilation baseline

With IP.BIN and the retail boot image combined, and with the game entry included as a future callable seed, the local 0.0.42 validation produced:

```text
Synthetic entries:       774
Reachable functions:     806
Call-graph edges:        1260
External/unresolved:     368
Reachable instructions:  53925
Known SH-4:              53925
Unknown SH-4:            0
CFG blocks:              9486
DCIR ops:                53742
RAW_SH4:                 0
Inline BSRF thunks:      11
```

The game bytes are not part of the DreamcastRecomp release; these figures come from the locally supplied CDI.

## Current runtime frontier

The native generated executable now follows the actual IP.BIN path, including its P2 continuation, and reaches:

```text
[CALL] 0x8C00D83A -> 0x8C00DB40
[CALL] 0x8C00DB40 -> 0x8C00DBE0
[CALL] 0x8C00DBE0 -> 0x8C001006
[DreamcastRecomp ERROR] Dreamcast BIOS GD-ROM syscall reached before GD-ROM HLE is implemented
(r4=0x1E r5=0x8CFFFFF4 r6=0x0 r7=0x0)
```

This is a significantly cleaner frontier than the 0.0.41 invalid RAM pointer: the next missing subsystem is now explicit and hardware-relevant.

## Next commercial step

0.0.43 should attach the user's CDI to the generated runner and implement the BIOS GD-ROM request/status/read behavior actually demanded by this bootstrap. The goal is not to return a hard-coded success value: it is to let the same generated runtime service real sector requests so later Katana `gdFs` code can build on it.
