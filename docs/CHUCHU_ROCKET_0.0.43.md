# ChuChu Rocket! commercial checkpoint — DreamcastRecomp 0.0.43

0.0.43 is the first source checkpoint where the generated native runner can answer the initial Katana BIOS GD-ROM requests instead of deliberately stopping at `0x8C001006`.

The disc is attached externally through `disc.map`. The map records the original CDI path plus the discovered contiguous data-track mapping, so DreamcastRecomp never embeds the game inside generated source or the release ZIP.

## Confirmed progression

```text
IP.BIN @ 0x8C008000
  -> bootstrap @ 0x8C008300
  -> BIOS GD-ROM ABI
  -> REQ_MODE (0x1E) complete
  -> SET_MODE (0x1F) complete
  -> game/Katana startup continues
  -> dynamic call 0x8C0F0596 -> 0x8C0F05A0
  -> stop: target is not yet part of the conservative raw closure
```

The new failure is useful because `0x8C0F05A0` is local executable-looking data reached through a register-indirect dispatch, not an unknown SH-4 opcode or a missing BIOS service. 0.0.44 should generalize callback/vtable/function-table discovery without promoting arbitrary data to code.

## Static baseline

- 806 generated functions
- 53,925 reachable SH-4 instructions
- 53,925 known
- 0 unknown
- 9,486 CFG blocks
- RAW_SH4=0

An earlier aggressive callback-pointer experiment was deliberately rejected because it expanded the closure into data and produced thousands of false UNKNOWN words. The released 0.0.43 keeps the conservative zero-RAW baseline and preserves the next failure for clean diagnosis.
