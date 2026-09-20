# Validation — DreamcastRecomp 0.0.180

## Crazy Taxi 2 fresh static closure

- Synthetic entries: 3,538
- Reachable functions: 3,538
- Call-graph edges: 9,806
- Reachable instructions: 297,656
- Known SH-4: 297,656
- Unknown SH-4: 0
- RAW_SH4: 0
- CFG blocks: 56,039
- DCIR ops: 303,879
- Inline BSRF thunks: 23
- Stored callback targets: 30
- Callback-object targets: 246
- Anchored dense callbacks: 229
- Argument callbacks: 65
- Promoted CFG fragments: 192
- Dense dispatch targets: 163
- Scaled-index targets: 155
- Rejected dirty fragments: 0

## New BRAF regression

The CT2 dispatch around `0x8C08138A` is `SHLL2 R9` + `BRAF R9` followed by six clean `BRA/NOP` trampolines. The regenerated core registers all six entries:

- 0x8C081392
- 0x8C081396
- 0x8C08139A
- 0x8C08139E
- 0x8C0813A2
- 0x8C0813A6

`generated_compile_test` explicitly asserts these entries.

## Target regression

- 0.0.179 packaged target union: 42,132
- 0.0.180 packaged target union: 42,685
- Added: 553
- Lost: 0

## Tests

- Core CTest suite: 51/51 PASS
- Generated CT2 build/link: PASS
- Generated CT2 compile-test: RC=0
- Generated core RAW_SH4 markers: 0
- Generated core unknown-SH4 markers: 0

## Runtime status

The previous invalid-memory failure at `guest_pc=0x8C08779C` was reproduced internally and eliminated by the shared-delay-slot CFG fix. Subsequent execution progressed to the later `BRAF` dispatch at `0x8C08138A`, whose six trampoline entries are now statically discovered and registered in 0.0.180. Container execution is substantially slower than the user's Windows test, so the final post-BRAF route still requires the manual CT2 test; no claim of first in-game frame is made here.
