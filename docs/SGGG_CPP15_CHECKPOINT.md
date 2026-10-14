# SGGG cpp15 checkpoint

Checkpoint reconstructed from the persistent cpp11 source checkpoint after a container reset.

## Source changes preserved

- Retail `.raw_boot` restaging now leaves the 32 KiB IP.BIN at `0x8C008000` intact and stages/scrambles only the main executable beginning at `0x8C010000`.
- Commercial IP.BIN PC-relative literals remain bound to the static recompiled instruction image while the original bootstrap low-RAM page is later reused.
- Low-RAM AOT overwrite detection captures 32-byte signatures and rebinds overwritten registered code to a matching recompiled relocation template. This fixes the SGGG VBR/IRQ case where `0x8C00F400` becomes the RTE stub corresponding to template `0x8C1BA4E8`.
- Stored-callback scan window increased from `0x0A` to `0x20` bytes so longer register-preserving callback setup sequences can be discovered structurally.
- Runtime-observed valid SGGG entry `0x8C181BF8` is preserved in `profiles/sggg_known_seeds.txt`.
- cpp11 Flash BIOS INFO/READ HLE and late Maple GETCOND START probe remain present.

## Validation completed before this checkpoint

- Full project test suite: 54/54 PASS after reconstruction.
- SGGG CDI reconstructed from 4 persisted parts with SHA-256:
  `4177f8ebc71fc9ae42b59827c9f455a70c5442f53e68b353fad91f89a8034de0`
- Main executable: `1_SGGG.BIN`, 3,106,103 bytes, load address `0x8C010000`.
- Reconstructed closure with callback scan widened to `0x20`:
  - 6,383 functions
  - 676,497 SH-4 instructions
  - RAW_SH4 = 0
- The visual runtime previously progressed to roughly 884 frames after fixing low-RAM VBR rebinding and `0x8C181BF8`.

## Current open blocker

`0x8C212C58` is a valid stored callback referenced from code around `0x8C1F2DE4`. It has a strong leaf-function prologue but does not save PR at entry. The current callback target validator still rejects it as an independent function. Work after this checkpoint should improve leaf-callback validation conservatively rather than add `0x8C212C58` as a title-specific seed unless needed for a diagnostic comparison.
