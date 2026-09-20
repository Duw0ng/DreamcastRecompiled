# ChuChu Rocket! commercial status — 0.0.49

0.0.49 is the first checkpoint built from a user-confirmed recognizable commercial frame rather than counters alone.

## What the 0.0.48 Windows run proved

The retail SEGA splash is being generated from the recompiled Dreamcast path. The geometry is recognizable and the game remains active through thousands of TA packets. The two concrete stops exposed by that run were BIOS GD-ROM command `0x16` and ARM7 `LDM/STM ^`.

## Fixes in 0.0.49

1. BIOS GD-ROM command `0x16` (`PAUSE`) completes instead of terminating the runner.
2. ARM7 user-bank `LDM/STM ^` forms are implemented, including exception-return state restoration.
3. TA color modes no longer collapse valid non-packed vertices to debug magenta.
4. 64-byte polygon/vertex/sprite records stay aligned across Store Queue / DMA packet boundaries.
5. The commercial BAT enables frame-synced PVR presentation to avoid exposing partially registered scenes as if they were completed video frames.

## Local visual diagnostic

With the new TA parser, the active splash framebuffer renders the background near white and the SEGA logo blue, with zero exact `#FF00FF` pixels in the captured frame. A region that appears dark in a mid-registration diagnostic is not treated as a final-render bug until the frame-synced Windows path reproduces it.

## Next acceptance

Run:

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

The useful evidence is now visual plus the end-of-run log. In particular: whether the completed splash is clean, what appears after it, whether GD-ROM advances beyond request `cmd=0x16`, and the next explicit `DreamcastRecomp ERROR` if one occurs.
