# DreamcastRecomp 0.0.151 — validation

## Scope

0.0.151 is a focused TA parser correctness build on top of 0.0.150. It does not replace the proven D3D11/TA bulk performance architecture.

## Flycast parity audited

- TA vertex format/stride comes from PCW Object Control only.
- 32-byte vertex formats: 0, 1, 2, 3, 4, 7, 8, 9, 10.
- 64-byte vertex formats: 5, 6, 11, 12, 13, 14.
- 64-byte polygon/vertex B halves remain locked continuations and are never decoded as independent PCWs.
- Modifier-volume data stays on its dedicated 64-byte continuation path; its stencil/shadow rendering effect remains a later compatibility item.

## New diagnostics

- `pvr-objisp=<count>` counts legacy replay conflicts where `PCW.Texture=0` but duplicated `ISP_TSP.Texture=1`.
- First legacy-OR conflict: `[PVR TA-OBJCTRL] ...`.
- First impossible vertex additionally emits `[PVR TA-STATE] pcw/isp/tsp/tcw decoder vbytes hbytes pending ...`.

## Regression coverage

The generated-runtime staging self-test now covers:

1. PCW untextured Intensity+Offset with ISP_TSP Texture=1: decoder remains Type 2 and the header remains 32 B instead of being falsely promoted to a 64 B textured header.
2. Split 64-byte Type-5 vertex: second 32 bytes are consumed as the locked B half even when their first dword looks like another parameter type.
3. Split 64-byte intensity+offset polygon header: face-color B half is consumed as continuation, not USER TILE CLIP/object-list/control data.

## Host validation

- CMake Release build: PASS.
- CTest: 50/50 PASS.

## Windows acceptance test

Run:

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

Reproduce the same Mouse Mania/heavy-board path and send the automatically saved session log. The key new fields are `pvr-objisp=`, `[PVR TA-OBJCTRL]`, `[PVR TA-STATE]`, `pvr-badv=`, `pvr-ta-stage=`, `pvr-long=`, `pvr-cpu-est-ms=`, `pvr-gcpu-ms=` and `fps=`.
