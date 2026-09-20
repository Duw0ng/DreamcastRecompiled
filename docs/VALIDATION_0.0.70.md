# Validation 0.0.70

## Scope

ChuChu Rocket! 4P Battle runtime target `0x8C01406A`. No PVR behavior changes.

## Structural discovery

The target is loaded as an address-taken callback and has a straight-line ABI adapter body that:

1. saves a callee-saved GPR with `MOV.L Rm,@-R15`,
2. performs only decoder-clean setup instructions,
3. ends in a local backward `BRA`, and
4. restores the exact same GPR with `MOV.L @R15+,Rm` in the branch delay slot.

The rule is applied only to R4-R7 callback arguments reaching a nearby call. No game-address seed is used.

## Commercial closure

- Reachable functions: 2,794
- Reachable instructions: 239,578
- Known SH-4: 239,578
- Unknown SH-4: 0
- RAW_SH4: 0
- New entries versus 0.0.69: `0x8C013F7E`, `0x8C01406A`, `0x8C0193E0`, `0x8C01946C`
- Removed entries: 0

## Regression

- CTest: 47/47 PASS
- Generated `generated_program.cpp`: compiled with Clang 17 at `-O0` for validation.
- Generated commercial runner: linked successfully against the generated runtime.

## Deferred

The 4P Battle board remains visually corrupted/mostly black. PVR gameplay-scene diagnostics are intentionally deferred to the next milestone so this release changes only CPU/code discovery.
