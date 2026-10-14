# DreamcastRecompiled v1.2 cpp36b1g — SONY_HAPTICS_VISUAL_V2

- Native DualShock 4 rumble through Sony HID.
- Native DualSense/PS5 rumble through Sony HID; both USB and Bluetooth report formats are implemented.
- PuruPuru and the tester share the same haptics backend.
- `Test rumble (PS4 / PS5)` works for selected WinMM/DirectInput Sony pads even when Windows exposes a generic joystick label.
- Controller preview redrawn closer to the original Dreamcast controller.
- Visual flicker reduced using memory-DC double buffering and change-triggered invalidation.
- Guest ABI1 is unchanged; no SH-4 guest rebuild is required.
