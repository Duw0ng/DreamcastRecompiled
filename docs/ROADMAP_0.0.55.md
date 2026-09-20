# DreamcastRecomp roadmap after 0.0.55

## Immediate Windows acceptance

1. Run ChuChu Rocket! through `run_commercial_recompiled.bat`.
2. Confirm the title and memory-card menu still render correctly.
3. On the save screen, test arrows plus Dreamcast A/B using `Z/Space/J` for A and `X/K` for B (or an XInput controller).
4. Confirm the former `0xA010A070` BIOS-font mapping fault is gone.
5. Return the next `[DreamcastRecomp ERROR]`, if any, plus heartbeats around the input event.
6. Compare perceived speed and `pvr-prof-ms` growth against 0.0.54.

## Input diagnosis

Use the new fields:

- `maple-input=current/recent`
- `maple-src=keyboard/xinput`
- `maple-changes`

If the masks change but the game does not react, continue in guest Maple/device semantics. If the masks remain zero, continue in Windows host-input capture instead.

## Save/VMU path

The memory-card screen makes VMU emulation the likely next device milestone. Do not fake a successful save globally. First observe the post-font path and the exact Maple unit/function commands requested by the game, then implement the smallest generic VMU block-storage device compatible with those requests.

## Performance

PVR raster is now the measured dominant host cost. Use the added `pvr-tri` and `pvr-tex=total/bilinear/paletted` counters to decide the next optimization:

- texture-heavy/bilinear: specialize sampler/format paths further;
- many small triangles: reduce raster setup/call overhead;
- paletted-heavy: optimize Morton/palette decode;
- PVR time drops below other subsystems: revisit SH-4 idle polling (`idle-skipped=0`) and AICA only then.

Preserve guest timing and 640x480 output while optimizing.
