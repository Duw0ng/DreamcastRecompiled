# v1.2 cpp36b — PURUPURU_MODERN_INPUT

- Adds native Dreamcast PuruPuru device handling to `dcr_maple`.
- Port A defaults to VMU A1 + PuruPuru simultaneously.
- Translates PuruPuru `SETCOND` effects to native XInput low/high motors with auto-stop timing.
- Adds independent A/B/C/D host-device selection via `DCR_MAPLE_PORT_<A-D>_DEVICE`.
- Adds explicit `xinput:N`, `winmm:N`, `keyboard_mouse`, `none`, and compatibility `auto` input specs.
- Combines keyboard + mouse as one virtual Dreamcast controller.
- Keeps every physical controller independently selectable.
- Keeps Maple Trace and the cpp35b1 immutable Guest ABI cache model.
- Guest ABI fingerprint unchanged.
