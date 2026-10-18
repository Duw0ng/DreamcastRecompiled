# Dreamcast Recompiled v1.2 — Classic Aqua V8.6.1 Controller Tester

## Controller changes (without altering cpp38e or V8.4 DXGI)

- Dreamcast controller visualization based on a realistic front-facing illustration; no PIL/PyPI dependency is required on Windows.
- Independent **L and R** shoulder-trigger indicators and pressure bars (0–100%).
- Two complete **XY indicators** for left and right sticks, with positive Y displayed upward.
- Persistent **P1–P4** selector. The tester inspects only the selected controller port instead of accidentally displaying another connected device.
- Highlights for A/B/X/Y/Start, directional pad, and L/R follow the assignments in `profiles/controller_profile.ini`, including `button:`, `pov:`, `axis:`, and calibration `range:` entries.
- The active-profile summary displays the stored assignments on the same panel. Changes to `controller_profile.ini` are detected without restarting the launcher.
- The existing profile editor and separate Visual Tester are still available. **P1–P4 only selects the port inspected by the tester; it does not automatically change how the game's four Maple ports are routed.**
- Artwork and windows support launcher widths of 800, 960, and 1152.

## How to test

Extract the ZIP into a new folder. Run `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`, open **Controllers**, select a player, and move each stick in a circle. Check that the XY indicators trace circles and the Y axis moves vertically rather than horizontally. Test the L/R triggers and A/B/X/Y buttons. The profile panel should show the actual saved assignments.

## Scope and limitations

- In `auto` mode, the tester polls XInput for the selected port, falling back to WinMM/DirectInput for that same port. Explicit `backend=xinput`, `backend=winmm`, and `backend=ps4` selections remain respected.
- Physical WinMM button numbering depends on the device driver and the profile learned by the Visual Tester. Basic mapping is retained for backward compatibility.
- A real Dreamcast controller has **one physical analog stick**; a PC controller's right stick appears in the additional XY indicator.
- Rumble still depends on the existing Maple runtime. Options marked as pending in the launcher do not enable unimplemented features.
- Real PS4/PS5 hardware could not be verified in the original Linux tests. Synthetic controller data and a Tkinter GUI under a virtual display were used.
- Video settings and VSync were not changed.
