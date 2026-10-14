# Validation — cpp32a CPP23 callback-closure rebase

Base: `DreamcastRecomp_v0.1.1_cpp23_GAME_PROFILES_REGRESSION_FIX`.

## Build

- CMake configure: PASS
- Full project build (GNU 14.2 / Ninja / Release): PASS
- `dc_raw_recomp` with nested-state closure: PASS

## Regression suite

- CTest: **54/54 PASS**
- `generated_single_build`: PASS
- `generated_program_build`: PASS
- `controller_visual_tester_package`: PASS

## Scope audit

The implementation is a cpp23 copy plus the conservative two-level Katana state-table callback recovery and version/documentation markers. It does not import the discarded cpp32a runtime/PVR/codegen snapshot.

## Commercial validation still required

`ccr.cdi` was not available in the current container/Library/Drive lookup, so the two observed runtime paths must be verified on the Windows test machine after a clean ChuChu rebuild:

- `0x8C0190A4 -> 0x8C04FFC0`
- `0x8C0190A4 -> 0x8C034FA0`

Expected: no missing native target, `RAW_SH4=0`, bounded `Nested state callbacks`, and no meaningful regression from cpp23 stable performance.
