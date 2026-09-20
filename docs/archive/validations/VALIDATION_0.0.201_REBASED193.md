# DreamcastRecomp 0.0.201_rebased193 validation

## Compatibility / closure

Validated against the raw BOOTSTRAP image reconstructed from the user's existing generated CT2 package:

- Closure passes: 17
- Reachable functions: 3548
- Reachable instructions: 298494
- Known SH-4: 298494
- Unknown SH-4: 0
- RAW_SH4: 0
- Scaled-index targets: 163

The bounded argument-selected table at 0x0C111478 resolves to:

- 0x8C06A850
- 0x8C06A8F0
- 0x8C06A9F4

All three entries decode as clean CFGs. Their generated supplemental package registers 30 basic-block entry points.

## Performance change

The CT2 commercial target defines `DCR_ASSUME_MMU_OFF=1`, matching every validated CT2 heartbeat to date (`sh4-mmu=off/0/0`). Generated 32-bit main-SDRAM reads and writes use inline `memcpy` word access after the same physical-range/mirror calculation as the runtime hot helper. Non-main-RAM addresses fall back to the original helpers, and Store Queue writes retain the exact local SQ path.

This removes cross-translation-unit helper calls from the common RAM word path, including the geometry hotspot around 0x8C080CE6 / 0x8C080D3A and stack/control stores.

No SH-4 scheduler, SPG, IRQ, host-sync, TMU, AICA, FTRV/FIPR precision, or guest clock change is used to obtain performance.

## Tests

- Core build: success
- Core CTest: 51/51
- CT2 commercial static library: success
- `generated_sub_8C06A_table.cpp`: compiled successfully
- `generated_program_part_17.cpp`: compiled successfully
- `generated_compile_test`: RC=0
- CT2 `dreamcast_program`: linked successfully
