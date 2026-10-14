# Crazy Taxi 2 — SH-4 adjacent ABI entry hotfix

## Runtime failure

`DreamcastRecomp ERROR: No recompiled/native target registered for Dreamcast address 0x0C16BFB6`

Canonical P1 target: `0x8C16BFB6`  
Nearest registered native entry: `0x8C16BFAA`  
Distance: `0xC` bytes

The target begins with a normal SH-4 GPR ABI prologue:

- `MOV.L R14,@-R15`
- `MOV R4,R14`
- `STS.L PR,@-R15`

The closure already had logic for functions immediately following an `RTS` delay slot, but that exact-boundary path only tested the FPU-only ABI predicate even though the surrounding comment described GPR and FPU support.

## Fix

At an exact post-`RTS` boundary, test `looks_like_strong_abi_prologue(..., false)` first and promote a proven GPR ABI entry through the conservative structural admission path. The existing FPU-only path remains as fallback. No title-specific address is seeded.

## Synthetic reproduction

A raw fixture was built with one 12-byte function beginning at `0x8C16BFAA`, followed immediately by a second GPR ABI function at `0x8C16BFB6`.

Before the patch:

- Synthetic entries: 1
- Reachable functions: 1

After the patch:

- Synthetic entries: 2
- Reachable functions: 2
- Function map contains `0x8C16BFB6,sub_8C16BFB6`
- RAW_SH4: 0

The normal project test suite was also run after the change; all regular tests passed, and the generated-program build test passed when run separately after the first aggregate command hit the execution-time limit.
