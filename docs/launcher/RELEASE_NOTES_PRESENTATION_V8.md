# Dreamcast Recompiled v1.2 — V8: 16:9 / 1080p output (experimental)

Baseline: cpp38e HOT_BLOCK_RECOVERY. The ABI1 guest, recompilation module, and device emulation remain unchanged. VSync is unchanged.

## Available Windows output modes

- **Output resolution:** native, 960p, 1080p, 1440p, or desktop.
- **Aspect ratio:** 4:3 preserves proportions with letterboxing/pillarboxing when required; 16:9 stretches the existing image across a widescreen output area (with bars if the window has a different aspect). **This is not genuine widescreen and does not increase field of view.**
- **DX11:** a full-screen triangle presentation pass uses bilinear GPU sampling without changing the original 640×480 render output. Native mode retains cpp38e's `CopyResource` path.
- **Software/GDI:** `StretchDIBits` outputs either to a 4:3 rectangle or the full window.
- **Per-game settings:** output settings inherit from global configuration or the game profile; the native `dcr_game_module.dll` and its cache are unaffected.

## Not included

- Real 1080p PVR rendering or higher-resolution geometry/textures.
- Wider horizontal camera view or per-game widescreen patches.
- VSync, fullscreen support, refresh-rate switching, or alternate output filters.

The integration is optional. Defaults add no new environment variables and preserve the original cpp38e presentation. This experimental presentation branch required Windows testing and FPS/audio/loading measurements before being considered stable. It did not replace the frozen official v1.2 release.

## How to test

In the V8 launcher, choose **Settings → Video → 1080p / 16:9**. Start with `960p / 4:3`, then `1080p / 4:3`, and finally `1080p / 16:9`. If a regression occurs, restore `native / 4:3` and attach the session log.

Note: a 1080p output window may be larger than the display on lower-resolution monitors. The window is resizable.
