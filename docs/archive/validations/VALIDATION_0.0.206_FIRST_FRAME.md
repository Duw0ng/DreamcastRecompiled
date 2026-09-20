# DreamcastRecomp 0.0.206 - First Frame Validation

Date: 2026-09-18

## Summary
This iteration extends the 0.0.205 commercial boot / G2 / modem work with a validated path to a real **first visible frame** on the commercial CDI that was uploaded as `Daytona USA.cdi`.

Important: the uploaded CDI does **not** appear to be Daytona USA internally. The disc identifies itself as:

- Product: `T19724M V1.001`
- Title: `PIZZICATO POLKA`
- Date: `2004-04-23`

Even so, it served as a valid commercial compatibility test and now reaches a real rendered frame.

## Additional progress beyond 0.0.205
- Device-clock-enabled commercial run advanced beyond the previous VBlank wait loop.
- Late SH-4 callback targets were iteratively absorbed until rendering began.
- First visible frame produced successfully.
- No `SH4-FAULT` and no `DreamcastRecomp ERROR` in the successful first-frame run.

## Late callback targets absorbed during first-frame push
- `0x8C044098`
- `0x8C019480`
- `0x8C05A75E`
- `0x8C0720E4`
- `0x8C042982`
- `0x8C04B4DC`
- `0x8C048254`
- `0x8C04411E`
- `0x8C044128`

## Successful first-frame signals
Observed during the successful run:

- `STARTRENDER` asserted
- `pvr-renders > 700`
- `pvr-frames > 700`
- `pvr-flips > 700`
- `pvr-packets > 33000`
- framebuffer dump produced and converted to PNG

A copy of the validated first frame is included as:

- `first_frame_daytona_not_daytona.png`

## Recommended command profile
For this particular CDI / test path, the validated runner mode is deterministic device clock:

```bat
run_commercial_recompiled_firstframe_probe.bat "C:\ruta\Daytona USA.cdi"
```

This batch uses:
- `--device-clock`
- optional direct-game-entry passthrough via `DCR_DIRECT_GAME_ENTRY`
- PVR/framebuffer dump to `daytona_firstframe.ppm`

## Notes
This package focuses on preserving the validated source tree and the first-frame evidence. It does not include the CDI itself.
