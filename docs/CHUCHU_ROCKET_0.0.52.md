# ChuChu Rocket! — 0.0.52 checkpoint

## What changed from 0.0.51

The Windows 0.0.51 run passed SEGA and reached Sonic Team, then failed at `0x8C0190A4 -> 0x8C0264D2`. The destination belongs to another row of an SDK two-level dispatch table. 0.0.52 recognizes the short call-then-tail method form inside an already-proven dense row and closes that family generically.

The dark Sonic Team background was a separate PVR omission. The logo geometry was correct, but pixels not touched by ordinary TA lists exposed DreamcastRecomp's `#101018` software clear. 0.0.52 decodes the real PVR background strip from VRAM and rasterizes it behind normal geometry.

## Local result

- former target `0x8C0264D2` passed;
- Sonic Team background is cyan from guest PVR data;
- 9,000-packet bounded run: 483 frames with no target failure;
- longer run: over 13,600 packets / 586 frames;
- ChuChu Rocket! title and `PRESS START BUTTON!` reached.

## Next acceptance

Run `run_commercial_recompiled.bat`, wait for the title and press **Enter** (mapped to Dreamcast START). Capture the first menu/input failure or renderer divergence. Black rectangles around some title textures remain a known 0.0.52 issue.
