# ChuChu Rocket! acceptance — 0.0.56

## Windows checkpoint

0.0.55 reached the memory-card selection screen with working controller input. Confirming a slot entered the retail BIOS-font path and faulted at `0xA012BAC0` (`guest_pc=0x8C0227D0`).

## Fix

The 0.0.55 synthetic font aperture ended at physical `0x0011FFFF`. The new access is physical `0x0012BAC0`, so 0.0.56 extends the existing read-only synthetic backing through `0x001FFFFF`. The HLE still returns `0xA0100020`; no proprietary BIOS bytes are included.

## Next observation

Heartbeats now print the last low-level Maple command/destination/unit/function and count all non-controller frames. If ChuChu probes a VMU after the font read succeeds, the next Windows log should reveal the exact command sequence needed for a generic VMU block-storage implementation.
