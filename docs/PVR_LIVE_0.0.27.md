# PVR live preview — DreamcastRecomp 0.0.27

The generated Windows runner can display the 640x480 software PVR framebuffer while the recompiled Dreamcast program continues executing.

```text
--pvr-window
--pvr-frame-sync
--pvr-window-scale=N
--pvr-window-interval=N
--pvr-window-fps=N
--pvr-window-throttle-ms=N
```

The normal 2ndMix script uses frame synchronization, scale 2 and a 60 Hz **absolute-deadline** pacer. Unlike 0.0.25, it does not add a fixed 16 ms sleep after render/present work.

```bat
run_homebrew_2ndmix_live.bat
```

Press Esc or close the window to stop.

The software renderer currently decodes common KOS polygon header state, U/Vs, ARGB1555/RGB565/ARGB4444 textures, basic twiddled/non-twiddled addressing, nearest sampling, color modulation, approximate alpha blending and triangle strips. It remains a bootstrap renderer, not an exact PowerVR2 implementation.

Frame boundaries are still inferred from immediate-mode PVR list progression. Exact ISP completion, render interrupt, page flip and VBlank remain future work.

For performance details see `PVR_PERFORMANCE_0.0.27.md`.
