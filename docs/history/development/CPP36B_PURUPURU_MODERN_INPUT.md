# cpp36b — PuruPuru + Modern Per-Port Input

cpp36b builds on the cpp36a Maple Device Tree without changing Guest ABI1.

## Maple additions

- PuruPuru / vibration pack device on either expansion slot.
- Port A defaults to VMU A1 in slot 1 and PuruPuru in slot 2.
- PuruPuru implements DEVINFO/ALLINFO, GETCOND, GETMINFO, BREAD/BWRITE timing data, SETCOND vibration and reset/kill.
- XInput vibration is driven natively from Dreamcast PuruPuru SETCOND requests, including automatic stop timing.
- DirectInput/WinMM controllers remain valid input devices; native force feedback is not claimed for that backend in cpp36b.

## Modern input selection

The Maple plugin accepts independent host selections for Dreamcast ports A-D:

```text
DCR_MAPLE_PORT_A_DEVICE=auto
DCR_MAPLE_PORT_B_DEVICE=xinput:1
DCR_MAPLE_PORT_C_DEVICE=winmm:0
DCR_MAPLE_PORT_D_DEVICE=keyboard_mouse
```

Supported values:

```text
auto
none
keyboard_mouse
xinput:N
winmm:N
```

Keyboard and mouse are intentionally combined into one virtual Dreamcast controller. Physical gamepads remain independently addressable by index.

Expansion slots are also configurable:

```text
DCR_MAPLE_PORT_A_SLOT1=vmu
DCR_MAPLE_PORT_A_SLOT2=purupuru
DCR_MAPLE_PORT_B_SLOT1=empty
DCR_MAPLE_PORT_B_SLOT2=purupuru
```

cpp36b retains one persistent VMU image, A1. Additional per-port VMUs are reserved for VMU V2.

## ABI / rebuild policy

Guest ABI1 remains frozen. The canonical `DCRuntime` fingerprint is:

```text
00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f
```

Runtime/Maple changes continue to reuse an existing `dcr_game_module.dll`.
