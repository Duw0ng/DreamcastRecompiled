# DreamcastRecomp roadmap — after 0.0.50

## 0.0.50 checkpoint

The first retail title now reaches a correctly colored completed splash through real TA list-completion interrupts and repeated guest `STARTRENDER` writes. The commercial PVR path is no longer stuck at `isp-start=0`. The long Windows run additionally exposed and drove fixes for GD-ROM `PLAY_SECTORS (0x15)` plus the ARM7/UI starvation that made PVR presentation appear frozen.

## 0.0.51 target

Primary target: advance beyond the SEGA splash to the next stable retail screen and classify only the first real dependency encountered there.

Priority order:

1. post-splash PVR/texture/blend correctness;
2. framebuffer read-base/page-flip timing if the next screen exposes scanout errors;
3. Maple input when an interactive menu appears;
4. the next GD-ROM command after the now-modeled CDDA state family;
5. real CDDA sample streaming only if the title requires audible disc audio;
6. ARM7 acceleration/recompilation so `host-aica-drop` can eventually return to zero without starving the guest.

Secondary cleanup: replace the one remaining initial render heuristic once the exact pre-Katana transition is understood.

Acceptance target: a Windows run that preserves nonzero `list-end-irq`, `isp-start` and `render-irq`, shows the clean completed SEGA splash, and reaches a new post-splash screen or a later reproducible guest/hardware boundary.
