# ChuChu Rocket! commercial status — 0.0.51

## Boundary inherited from 0.0.50

The Windows acceptance run passed the SEGA splash and then stayed black indefinitely. The process was not dead: SH-4, ARM7, VBlank, Holly IRQ and Maple continued to advance, while TA/PVR production froze exactly at 5,185 packets / 144 guest render starts and frames.

## Root cause

The post-splash code enters a Katana timed wait through `0x8C0DB4E6`. Its time source ultimately reads SH-4 TMU0 `TCNT0` at `0xFFD8000C` and computes elapsed ticks from the down-counter. The guest had configured:

```text
TSTR = 0x01
TCOR0 = 0xFFFFFFFF
TCNT0 = 0xFFFFFFFF
TCR0 = 0x0002
```

In 0.0.50 the entire P4 internal-MMIO block was sparse shadow storage, so `TCNT0` never changed. The wait therefore could never reach its required elapsed-tick threshold even though the rest of the machine remained alive.

## 0.0.51 fix

The generated runtime now owns three TMU channels and models `TSTR`, `TCOR`, `TCNT` and `TCR`. Running channels decrement from guest SH-4 cycles using the internal TPSC prescalers, reload `TCOR` on underflow and set `TCR.UNF`. Full timer-interrupt delivery and external TMU clock inputs are not yet claimed.

Heartbeat telemetry includes:

```text
tmu0=<run|stop>/<TCNT>/<TCR>/<total ticks>
```

The same ChuChu route now shows `TCNT0` continuously decreasing instead of remaining `0xFFFFFFFF`, and the wait returns naturally.

## Runtime acceptance

The former freeze is passed deterministically. A bounded ARM7-enabled run reached the intentional 6,500-packet probe limit with:

```text
TA packets:             6500
logical frames:          256
renders:                 256
page flips:              255
TA list-end IRQs:         772
guest ISP/STARTRENDER:    257
GD-ROM requests:           31
```

The only terminal condition was the deliberate diagnostic packet limit. A separate longer run exceeded 8,600 packets / 449 frames with no guest error before the external host timeout.

## CDDA follow-through

The longer route reaches the previously observed GD-ROM `0x15` PLAY_SECTORS command. 0.0.51 advances CDDA position using guest time and exposes playing/paused/terminated state plus drive polling counters. The current ChuChu request terminates instead of leaving CDDA permanently in PLAY. This was not the cause of the original 5,185-packet hang, but it removes the next drive-state trap on the same route.

## Static closure

No static discovery changes were required:

```text
reachable functions:        2194
known SH-4 instructions:  165555
unknown SH-4:                  0
RAW_SH4:                       0
```

The progress is therefore a hardware/runtime correction rather than a per-title code seed.
