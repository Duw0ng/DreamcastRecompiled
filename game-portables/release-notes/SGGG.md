# DreamcastRecompiled — Segagaga (SGGG) Direct Portable v1.2 Rev 2

**Windows x64 · game-specific native SH-4 module · original standalone launcher.**

## Validated changes (17 October 2026)

- **Boot fixed:** raw CDI tracks can select `scrambled` even when the older portable frontend requested `plain`; the 32 KiB `IP.BIN` is preserved.
- **AICA audio fixed for menu navigation:** when a non-looping one-shot voice completes, its `KYONB` latch is cleared. Subsequent global `KYONEX` commands for unrelated effects do not replay already-finished menu sounds.
- **Maintainer gameplay confirmation:** the repeated sound on menu option transitions disappeared. This improved SGGG's playability, but is not a claim that all game audio is perfect.
- **Game name and UI:** original launcher, input and VMU controls kept; inherited visible ChuChu Rocket! text replaced with SGGG-specific naming. No need to recompile SH-4 per launch.
- **Packaging:** Windows x64 CI builds, AICA and bootstrap tests, verified SHA-256 and title-specific portable guest ABI.

Official downloadable package: [SGGG Direct Portable v1.2 Rev 2](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2)

The original direct-portable v1.2 binary is preserved in [the historical archive](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2-initial-archive). The general v1.2 Stable release is unchanged.

## Usage and limitations

Extract the full ZIP on Windows x64, start `DreamcastRecompiled.exe`, and select a compatible user-supplied SGGG CDI/GDI image. The ZIP does not include a game disc or Sega firmware. Other untested disc revisions and compatibility paths may still exhibit bugs.
