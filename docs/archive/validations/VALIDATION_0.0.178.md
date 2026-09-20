# Validation — DreamcastRecomp 0.0.178

## Triggering Crazy Taxi 2 run

The real Windows 0.0.177 run progressed beyond `PRESS START BUTTON` into the game-mode/rules menu. Selecting the default **PLAY BY NORMAL RULES** branch reached dispatcher `0x8C03E06E`, which attempted the valid P0 target `0x0C03ADEC` (canonical P1 `0x8C03ADEC`) but the target was not registered. Immediately before the failure the run had reached 167 GD-ROM requests, 9,800 sector reads, approximately 20 MB read, about 3,152 PVR frames and more than 503 million traced calls.

## Rules/mode callback-family closure

The observed target belongs to a mixed/sparse callback structure rather than an isolated function. 0.0.178 adds eight validated roots in one supplemental AOT module:

- `0x8C0360A2`: 103/103 known SH-4, 0 unknown.
- `0x8C03ADEC`: 252/252 known SH-4, 0 unknown.
- `0x8C03C5D8`: 720/720 known SH-4, 0 unknown.
- `0x8C03CD7E`: 685/685 known SH-4, 0 unknown.
- `0x8C03D7A8`: 189/189 known SH-4, 0 unknown.
- `0x8C035B30`: 335/335 known SH-4, 0 unknown.
- `0x8C03E1B2`: 45/45 known SH-4, 0 unknown.
- `0x8C034E28`: 264/264 known SH-4, 0 unknown.

`generated_ct2_rules178.cpp` registers 478 block/native entry points for these roots. The complete packaged generated source contains 41,671 unique registered target addresses. Comparison against `CT2_0.0.173_REQUIRED_TARGETS.txt` reports 36,018 required addresses and **0 lost**.

The structure is deliberately not promoted by the existing generic dense-vtable scan because valid local pointers are interleaved with null/small scalar fields. A future analyzer pass can generalize this as an anchored mixed/sparse callback-table pattern after conservative false-positive auditing.

## Regression / build validation

- Main source CTest: **50/50 PASS**.
- Generated CT2 `dreamcast_program`: build/link PASS on the development host.
- Packaged `generated_compile_test`: **RC=0** and explicitly registers the rules/mode supplemental module.
- Normal CT2 launcher remains manual. Auto A/START and raster fast-forward remain confined to the optional audit launcher.

## Runtime acceptance

A Linux Debug runtime pass with the newly expanded closure was too slow to reach the post-rules state in a useful test window. Therefore 0.0.178 does **not** claim selection-of-character or in-game success yet. The next acceptance point is the Windows manual run after choosing **PLAY BY NORMAL RULES**.
