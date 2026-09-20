# Validation — 0.0.67

Scope: second 4P Battle stage-start callback discovery only.

- Accepted Windows checkpoint: 0.0.66 passes `0x8C0F96D8` and reaches a later stage-start call at `0x8C0E965E -> 0x8C0FB168`.
- No title-specific seed for `0x8C0FB168`.
- Existing shared-tail literal callback recognizer now allows a single forward BRA up to 0x28 bytes from the literal load (previously 0x20).
- The callback register must remain unchanged before the BRA, in the BRA delay slot, and through the short invoke block.
- Current ChuChu Rocket! commercial closure: 2,784 functions, 239,352 known SH-4 instructions, 0 unknown, RAW_SH4=0.
- New closure entries versus 0.0.66: `0x8C0FB168`, `0x8C0FB38C`, `0x8C0FA66C`, `0x8C0FAB08`; no previous functions are lost.
- PVR, VMU, Maple, BIOS-font synthesis, AICA and timing behavior are unchanged.
