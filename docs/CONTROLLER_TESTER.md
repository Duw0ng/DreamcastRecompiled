# Controller Visual Tester — v0.1

Run `run_controller_test.bat` from the repository root.

The tester uses the same public controller rules as the runtime:

- `auto` tries XInput first and then native DirectInput/WinMM;
- native DualShock 4 does not require DS4Windows;
- the configured `profiles/controller_profile.ini` is loaded automatically;
- the same `logical:*`, `button:*`, `axis:*`, `pov:*` and trigger bindings are interpreted;
- the profile deadzone is applied before the Dreamcast analog values shown on screen.

## Tabs

### Mando visual
Shows the state after mapping to Dreamcast. The two stick dots are the values the Dreamcast guest receives, not only the raw Windows axes. Trigger bars show 0–255.

### Raw / diagnóstico
Shows backend, device, raw button mask, POV, physical axes, mapped Dreamcast values and the active bindings. This is useful for distinguishing a Windows/HID issue from a mapping issue.

### Aprendizaje
Walks through left/right sticks and representative buttons/D-pad inputs, detects the physical axis/button that moved, and can save the learned values to `profiles/controller_profile.ini`.

The learning mode never writes the profile until **Guardar como perfil** is pressed.

## L2 / R2 on native PS4 controllers

DirectInput/WinMM layouts are not uniform. DreamcastRecomp therefore does not assume U/V are L2/R2. Without calibration, native PS4 triggers use their independent button bits (0 or 255), preventing cross-talk.

For analog pressure, use **Aprendizaje** and complete the L2 and R2 steps by pressing each trigger fully and releasing it. The tester records the actual neutral and full values and writes a binding such as `range:z:0:32768`. Separate axes and a shared centered axis are both supported.
