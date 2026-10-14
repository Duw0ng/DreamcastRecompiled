# SGGG checkpoint — cpp11 recovery

This checkpoint preserves the SGGG work that had previously lived only in a temporary generated workspace.

## Source changes

- Commercial executable restaging is derived from the generated `.raw_boot` section address/size instead of the Crazy Taxi 2 hard-coded size.
- Dreamcast BIOS FlashROM vector implements `FLASHROM_INFO` and `FLASHROM_READ` over a deterministic 128 KiB HLE flash image.
- Flash vector remains installed at `0x8C0000B8 -> 0x8C001004` during commercial boot setup.
- The existing opt-in `--probe-controller-start-burst` low-level GETCOND diagnostic is moved to a late SGGG-safe window: PVR frames 1800..5000, short START pulses every 90 GETCOND responses.
- `profiles/sggg_known_seeds.txt` records the six SH-4 entries recovered through cpp9/cpp10 tracing.

## Last known SGGG milestone before this source recovery

The cpp10 generated runtime sustained 2,566 consecutive PVR frames with no missing SH-4 target and reached:

`Hitmaker -> CRI/Sofdec -> Thunderstone -> Mission Statement -> Management Philosophy -> 2025 HANEDA TOKYO -> anime cinematic`

The target for the next diagnostic run is:

`cinematic -> title screen -> PRESS START -> real menu`

The START injection is diagnostic only and is not enabled during normal/manual execution.
