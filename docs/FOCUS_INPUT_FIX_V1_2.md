# Dreamcast Recompiled v1.2 — Input Focus Fix (Tester Release)

## What was fixed

- The Maple ABI2/V3 plugin no longer processes keyboard, mouse, XInput, or WinMM input unless the Dreamcast game window is the active Windows window.
- The runtime's compatibility path includes the same protection.
- Focus handling applies to Dreamcast ports A–D, corresponding to player selection P1–P4.
- Controllers stay connected, but button, stick, and trigger values are neutral (zero) while the game runs in the background.
- The previous mouse position is discarded on focus loss, preventing accumulated jumps when returning to the game.
- Focus validation checks the **actual process ID and window class** of `DreamcastRecompPVRWindow` instead of trusting an uninitialized HWND in the plugin.

## How to test

1. Extract the distribution into a new folder.
2. Open the launcher, assign a device in **Controllers**, and start ChuChu Rocket!.
3. With the game window active, verify that keyboard, mouse, and controller input work.
4. Switch to another application with Alt+Tab, or click the desktop. Inputs must not affect an inactive game.
5. Return to the game. Input should work again without a delayed mouse-position jump.
6. Repeat with ports B–D if multiple input devices are connected.

Session logs may include:

```text
[MAPLE FOCUS] inactive | input=neutral A-D
[MAPLE FOCUS] active | input=enabled
```

**The SH-4 guest, ABI1 cache, DX11 rendering, audio, and VSync were not modified.** The first launch updates the runtime/Maple DLLs without rebuilding the game's native SH-4 module.

## Release verification

- Linux Release builds of DreamcastRuntime and the updated plugins.
- 53 passing CTest cases (excluding two lengthy full-generation tests).
- The `maple_input_focus_tests` regression checks active focus, background state, foreign windows, and minimization.
- ZIP integrity was checked. Visual validation and testing with physical input hardware on Windows remained pending for this original tester release.
