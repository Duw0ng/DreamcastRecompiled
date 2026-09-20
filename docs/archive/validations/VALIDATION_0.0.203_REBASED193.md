# DreamcastRecomp 0.0.203_rebased193 validation

## Purpose

0.0.203 is a fail-closed SH-4 closure hotfix after the real ChuChu Rocket 0.0.202 run expanded to 12,288 raw entries / 13,048 final functions and decoded 304,178 unknown SH-4 instructions. That shape is data promoted as code, not a legitimately larger game closure.

## Containment changes

- Global address-taken scan remains available, but promotion now requires >=4 repeated clustered references plus an unmistakable strong ABI or compact callable entry shape.
- Global promotion budget is reduced from 512 to 128 maximum; candidates remain visible in `sh4_address_taken_evidence.csv` even when not promoted.
- The 0.0.201 ABI-argument bounded-table rule is restricted to exact 4-byte entries, `2 <= N <= 8`, and every table target must independently have a strong callable shape and decoder-clean CFG.
- The older memory-loaded bounded-table path is restored to its pre-0.0.201 dense-callable validation.
- Any speculative function whose reachable CFG contains UNKNOWN SH-4 is forbidden from contributing further closure edges.
- Every new closure candidate passes a final decoder-clean fragment gate before being admitted.
- Commercial defaults return to 8192 final functions / 6144 raw closure entries, leaving 2048 entries of protected ProgramAnalysis headroom.
- The raw recompiler prints `[SH4 closure] pass N entries=X/Y` progress on every pass so a long analysis no longer looks frozen.

## CT2 regression

Using the retained CT2 raw image locally:

- closure converges in 17 passes;
- synthetic/final functions: 3,553;
- reachable instructions: 299,380;
- known SH-4: 299,380;
- unknown SH-4: 0;
- RAW_SH4: 0;
- global address-taken promoted: 96;
- `0x8C03650C` remains present and proven (16 references / 215 instructions / 11 calls);
- `0x8C06A850`, `0x8C06A8F0`, and `0x8C06A9F4` all remain in the final function map.

The raw analysis takes about 18 seconds in the local Linux validation environment. This timing is not a Windows/Xeon performance claim.

## Core regression

- CTest: 51/51 PASS.
- CT2 `dc_runtime.cpp`: compile PASS.
- CT2 `generated_runner.cpp`: compile PASS.
- CT2 `generated_program_part_17.cpp`: compile PASS.
- CT2 `generated_sub_8C03650C.cpp`: compile PASS.
- CT2 `generated_sub_8C06A_table.cpp`: compile PASS.

## ChuChu acceptance

The actual ChuChu retail image is not redistributed in the release package, so the 0.0.203 acceptance test must be run against the user's local CDI. The expected healthy shape is a few thousand clean functions with `Unknown SH-4 = 0` and `RAW_SH4 = 0`, not a closure that races toward 6,144/8,192 entries.
