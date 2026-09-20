# Validation — DreamcastRecomp 0.0.177

## Main regression

- Release source build: PASS.
- CTest: **50/50 PASS**.
- New regression covers a `JMP @Rn` whose delay slot is simultaneously a separate CFG block entry; target sampling and delay-slot execution are preserved in the delayed path.

## Crazy Taxi 2

Validated against the recovered real `Crazy-Taxi-2-pal-dcp` CDI.

- `0x8C03D5CE`: 154/154 known SH-4, 0 unknown, integrated with 30 block entries.
- `0x8C030634`: 115/115 known SH-4, 0 unknown.
- Callback siblings integrated: `0x8C0301B4`, `0x8C0302A2`, `0x8C03052C`, `0x8C0303D0`, `0x8C030760`, `0x8C030850`, `0x8C030A26`, `0x8C030A94`; every root probe reported 0 unknown SH-4.
- `0x8C0300D8`: 25/25 known SH-4, 0 unknown.
- The runtime failure at `0x8C091AF6 -> 0x00000000` was proven to be generated-code semantics, not a NULL game callback: original opcode `0x402B` is `JMP @R0`, and the old split-CFG lowering omitted the dynamic-target snapshot. 0.0.177 fixes this generically.
- After all above fixes, the real CT2 audit reached at least 169 GD-ROM requests, 1,024 rendered frames and GETCOND ~1508 with no additional missing-target before the host timeout.
- Visual checkpoints captured VMU/memory-card flow, Presented by SEGA, Hitmaker and ADX.

## Packaged generated build

- `dreamcast_program`: build/link PASS.
- `generated_compile_test`: RC=0.
- Explicit registration guards include `0x8C03D5CE`, `0x8C030634`, the sibling callback roots, and `0x8C0300D8`.

The normal `run_crazy_taxi_2.bat` remains manual; audit-only auto-input/fast-forward is separate.
