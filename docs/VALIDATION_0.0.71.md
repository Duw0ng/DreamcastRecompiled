# Validation 0.0.71

## Scope

ChuChu Rocket! 4P Battle runtime target `0x8C0E8574`. No PVR behavior changes.

## Structural discovery

The gameplay setup function loads `0x8C0E8574` into callee-saved R8 at `0x8C019E4C`, traverses a mode selector containing conditional and unconditional direct branches, and reaches `JSR @R8` at `0x8C019F32`. The old R8-R14 detector was linear and rejected the literal as soon as it encountered a `BRA` from a different selector arm.

0.0.71 adds a bounded local CFG proof for callee-saved callback registers. A candidate is promoted only when a reachable local path preserves the exact register and terminates at `JSR/JMP @Rn`. Any explicit write to that register kills the path. Direct branches must remain within the bounded search region.

No game-address seed is used.

## Commercial closure

- Reachable functions: 2,814
- Reachable instructions: 241,737
- Known SH-4: 241,737
- Unknown SH-4: 0
- RAW_SH4: 0
- Added entries versus 0.0.70: 20 (including `0x8C0E8574` and `0x8C0E9056`)
- Removed entries: 0

## Deferred

The 4P Battle framebuffer remains mostly black with only a small lower-left HUD region visible. PVR gameplay-scene diagnostics are the next dedicated milestone.
