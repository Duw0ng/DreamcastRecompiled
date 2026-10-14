# Dreamcast Recompiled v1.2 — Classic Aqua Icons Fix

Base: **cpp38e HOT_BLOCK_RECOVERY**, unchanged (runtime, guest SH-4, PVR, AICA, GD-ROM and binaries).

## Interface

- Restores the **soft blue classic aquatic background** from the very first Aqua launcher, replacing the pronounced repeated-wave texture introduced by V2/V3. The old source picture included icons and a static 1998 clock; this edition reconstructs the unobstructed background using the original image's clean regions. It is intentionally not the full old picture, which would duplicate buttons and prevent a live clock.
- Recuts the existing eight main icon PNGs and nine advanced icons with corrected transparency and edges, preserving the original Dreamcast-style icon artwork.
- Removes the opaque blue strip from between the arms of the controllers, stray blue fringe on gear/VMU/monitor, background plate around the network globe, and remaining isolated matte pixels.
- Provides 1152, 960 and 800 pixel resource sets. Runtime uses Tkinter PhotoImage and needs no image libraries.
- Keeps V3 retained-mode hover (no background repaint), live clock, working advanced hide/show, library, controller configuration, VMU, settings and navigation.

## Regenerating icon assets (developers only)

Original V3 cutouts are retained in `launcher/assets/original_cutouts/`. To rebuild with the same deterministic source image, developers can install Pillow, OpenCV and SciPy and run:

```shell
python tools/asset_rebuild/rebuild_classic_assets.py
```

The rebuild utility does not run during game launch. End users need only Python 3 + Tkinter.

## What is validated

- Launcher GUI actions and window behavior under Linux Xvfb.
- Asset quality assertions, including transparency between the controller grips.
- Complete source and binary integrity comparison against V3 for every file outside the launcher.

The Windows launcher has not been run in the user's Windows PC in this environment.
