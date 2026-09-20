# DreamcastRecomp 0.0.43 — Validation

## Core regression

- CTest: **47/47 PASS**.
- KallistiOS corpus supplied for the project: **155/155 ELF loaded**, **155/155 ISA-clean**, **155/155 `_main` graphs with RAW_SH4=0**.
- Symbolized KallistiOS functions scanned: **202,431**.
- Known SH-4 instructions: **17,953,662**.
- Unknown SH-4 instructions: **0**.

## Commercial static closure — ChuChu Rocket! CDI

The CDI is used only as a local test input and is not redistributed.

- ISO payload base: `0x4B000`
- FAD base: `150`
- Boot: `1ST_READ.BIN`, 1,532,256 bytes at `0x8C010000`
- Combined bootstrap base: `0x8C008000`
- Entry: `0x8C008300`
- Generated functions: **806**
- Reachable instructions: **53,925**
- Known SH-4: **53,925**
- Unknown SH-4: **0**
- CFG blocks: **9,486**
- RAW_SH4: **0**

## Final native commercial run

Command used locally:

```text
dreamcast_program --commercial-boot --disc-map=disc.map --commercial-continuation-limit=50000
```

The previous 0.0.42.1 boundary at BIOS GD-ROM is passed. 0.0.43 successfully processes:

```text
[GDROM] request #1 id=2 cmd=0x1E ...
[GDROM] request #2 id=3 cmd=0x1F ...
[GDROM HLE] requests=2 | execs=2 | sector-reads=0 | bytes=0
```

The next failure is a genuine raw-closure/runtime-dispatch gap:

```text
[CALL] 0x8C0F0596 -> 0x8C0F05A0
[DreamcastRecomp ERROR] No recompiled/native target registered for Dreamcast address 0x8C0F05A0 (guest_pc=0x8C0F0596)
```

This checkpoint intentionally does not add a title-specific seed for that address. The next version should recover the target generically from the data-driven Katana dispatch structure that feeds the `JSR @Rn`.

## Implemented but not yet reached by this final retail checkpoint

0.0.43 also contains first-pass SH-4 DMAC channel-2, Holly event/mask routing, external SH-4 interrupt entry state, TA/texture transfer routing and asynchronous interrupt-unwind propagation. These paths remain subject to the next retail and KallistiOS execution tests; their presence is not claimed as complete hardware emulation.
