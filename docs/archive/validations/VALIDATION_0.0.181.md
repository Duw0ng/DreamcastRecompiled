# Validation — DreamcastRecomp 0.0.181

## Runtime boundary addressed

The supplied 0.0.180 log stopped at the indirect call `0x8C05F644 -> 0x0C060564`, canonical P1 address `0x8C060564`. The target is in main RAM and was not registered in 0.0.180.

## Static CT2 closure

- Functions: 3,539
- Reachable/known SH-4 instructions: 297,692 / 297,692
- Unknown SH-4: 0
- CFG blocks: 56,047
- Generated RAW_SH4 markers: 0
- New function `sub_8C060564`: 36 instructions, 36 known, 0 unknown, 8 blocks, 7 literals, 2 calls

## Registered entries for the new function

- 0x8C060564
- 0x8C060576
- 0x8C0605B8
- 0x8C0605C0
- 0x8C0605C6
- 0x8C0605CC
- 0x8C0605D0
- 0x8C0605DA

## Tests

- Core CTest suite: 51/51 PASS
- Generated CT2 build/link: PASS
- Generated CT2 compile-test: RC=0
- Linux runner smoke: starts commercial bootstrap; without a disc map it stops at the expected first data-read boundary rather than a recompilation error

## Windows acceptance target

Copy the user's Crazy Taxi 2 CDI as `experimental_ct2_0.0.181/ct2.cdi`, run `build_windows.bat`, then `run_crazy_taxi_2.bat`. The primary acceptance condition is that execution passes the former `0x8C05F644 -> 0x0C060564` missing-target stop. Any later stop should be captured with the generated 0.0.181 session log.
