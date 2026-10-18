# Dreamcast Recompiled v1.2 — Tester Release

Classic Aqua launcher with a game library, 1080p/16:9 DX11 presentation, and an integrated controller panel. The runtime is still based on **cpp38e HOT_BLOCK_RECOVERY**. This UI release did not change the SH-4 recompiler or the previously validated DX11 presentation.

## Controllers (no additional windows required)

1. Open **Controllers** and choose **Player P1–P4**.
2. Under **Device**, choose Auto, XInput 1–4, or DirectInput/WinMM 0–15. **Refresh** checks which devices currently report input. You may deliberately select a silent/unidentified port for diagnostics.
3. Press **Start** with sticks centered and triggers released.
4. Select each control, move it to its limit, and press **Capture**. Calibration begins with stick X/Y and continues with L/R. You can then learn A/B/X/Y, Start, and D-pad mappings in the same selector. **Skip** bypasses an individual step.
5. Adjust **Dead zone** to correct drift. Select **Save player** to store calibration in launcher settings.
6. **Apply to game** updates the real `profiles/controller_profile.ini` and creates `profiles/controller_profile.ini.bak` automatically.

Both stick indicators display X/Y in one circle. UP is shown as positive Y for visualization only; mappings are converted to Maple's native convention. The tester uses the device explicitly selected for that player and does not silently substitute another. Calibration from a different device is not reused.

**Actual limitation:** the stable runtime supports only **one active `controller_profile.ini` profile**. The launcher can configure and test devices independently for P1–P4, but **Apply to game** changes the single active engine profile. This revision does not implement four simultaneous Maple assignments.

Calibration translates movements into `axis:...`, buttons into `button:...` or `pov:...`, and trigger pressure into `trigger:...` / `range:<axis>:<rest>:<maximum>`, all understood by cpp38e. Universal support for every DualSense driver variant is not available; when WinMM maps both triggers to one axis, calibrate L and R separately.

## Launch

`INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`

You do not need to rebuild the SH-4 guest to adjust the interface or controllers. VSync was not changed. The separate advanced Visual Tester remains available.

## Original validation

- 94 automated launcher, controller, profile, and calibration tests in a virtual environment.
- Python syntax compilation passed.
- Byte-for-byte comparison against V8.6.1 for files outside `launcher/`, including runtime, executables, and batch files.
- Physical controller detection and saving profiles with real hardware remained pending Windows validation for this historical release.
