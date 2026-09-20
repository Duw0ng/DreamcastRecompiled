# DreamcastRecomp 0.0.168 validation

## Scope
0.0.168 is a compatibility-only development milestone derived from 0.0.167. It does not intentionally change PVR/TA rendering, AICA/CDDA, scheduler, host synchronization, texture behavior, or the FPU Region+ implementation.

## Compatibility changes
- File-backed indirect target resolution follows PC-relative literals through `MOV.L` pointer cells/vtable slots and register copies.
- Common integer address construction (ADD/SUB, logical ops, shifts/extensions/swaps) participates in the bounded constant proof.
- Raw closure can prove literal values reaching `JSR/JMP @Rn` through a bounded local CFG and register copies.
- Main-RAM code dispatch canonicalizes either P1 or P2 aliases.
- Missing-target diagnostics print physical/P1/P2 aliases, nearest registered target and target bytes.

## Regression validation
- Host Release build succeeds.
- CTest: 50/50 PASS.
- Function-analysis regression resolves `literal -> pointer cell -> register copy -> P2 JSR`.
- Synthetic raw commercial closure reaches the P2 target without unresolved calls or RAW_SH4.
- Generated commercial C++ compile/run validation is required before packaging.

## Live target
Crazy Taxi 2 should advance beyond the previous bootstrap failure at requested target `0xAC00E1A0`. If another target is missing, the new diagnostic fields are the primary evidence for the next generic compatibility pass.

Stable rollback remains 0.0.131; immediate development rollback is 0.0.167.
