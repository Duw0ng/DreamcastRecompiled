# RC8 · ChuChu Rocket! native portable inside Aqua

## What changes

The official Python/Tk Aqua launcher now prefers the precompiled native bundle
in `games/CHUCHU_ROCKET/` when the user selects the correct **CDI/ISO**. The
bundle was imported from the user's `CHUCHU_ROCKET_portable_latest.zip`.
`safe_guests/` has not been modified.

The uploaded portable's legacy `DreamcastRecompiled.exe` is intentionally NOT
used: it is a second, old-style graphical launcher which expects a development
runner. Aqua is the sole public launcher; its code starts the bundled
`bin/dreamcast_program.exe` directly after local disc preparation.

## Normal execution

1. Start `ABRIR_LAUNCHER.bat` or `DreamcastRecompiled.exe`. The existing
   precompiled Aqua `.exe` is sufficient: the dispatch lives in the external
   `launcher/launch.py`, so rebuilding the launcher is **not required**.
2. Choose **Juegos** and point Aqua at your own `CHUCHU ROCKET.cdi` (its name
   does not matter). Click **Jugar**.
3. The background subprocess validates the native package manifest (ABI1,
   `cpp38e-v8.6-sh4-dynamic-ram-aot-v1`) and SHA-256 checksums.
4. It runs `dc_disc_probe.exe` from the bundle; it checks the **title/volume
   inside the generated disc.map**. A different game will not load the ChuChu
   DLL, even if its filename suggests otherwise.
5. For ChuChu, `dc_boot_prepare.exe` creates local temporary boot files from
   the user's own CDI. The native guest starts with
   `DCR_EXTERNAL_RAW_IMAGE`/`DCR_EXTERNAL_RAW_BASE`, `--disc-map` and the
   same relevant cpp38e video/audio/clock flags as the established runner.
6. The temporary files live under LOCALAPPDATA until the native process exits
   and are then removed. The game remains free to stream additional GD-ROM
   assets straight from the user's CDI through the GD-ROM plugin.
7. Other games automatically fall back to the existing incremental BAT runner.
   If **Preparar SH-4** or full rebuild mode is explicitly requested, Aqua
   uses the original compiler path for the selected game.

## Important boundaries

- No compiler, Python installation or CMake is needed **for ChuChu Rocket!**
  if the Windows offline Aqua build and precompiled game bundle are intact.
- Other titles may still require MSVC/CMake in the current build.
- CDI and ISO are the tested/probed interfaces of `dc_disc_probe.exe` in this
  source. GDI remains a separate compatibility task despite the generic
  portable README mentioning it.
- The uploaded guest DLL is native code generated from the game; distributing
  it involves a copyright question independent of the fact that no disc image
  or extracted resources are included.
- No Windows gameplay test was possible in the Linux build environment. The
  probe, staging and runtime start paths need to be tested on Windows.
- Diagnostic log: `%LOCALAPPDATA%/DreamcastRecompiled/launcher_v1_2/launcher_console.log`.
  For troubleshooting, enable the launcher diagnostic console.

## Package layout

`DreamcastRecompiled.exe` + `_internal/` = persistent Python/Tk Aqua host.
`games/CHUCHU_ROCKET/bin/` = runtime + guest DLL + GD-ROM/Maple/network plugins.
`safe_guests/` = original fallback seed modules (unchanged).
`scripts/windows/run_game.bat` = legacy incremental runner used for other games.
