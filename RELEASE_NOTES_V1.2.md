# DCRV1.2 Stable — Español/Inglés + VMU Fix

DreamcastRecompiled V1.2 Stable Release for Windows x64.

## Highlights

- Aqua launcher with Spanish/English UI and selectable themes.
- Bundled Python/Tk host: no separate Python installation required.
- ChuChu Rocket! as the primary tester game, with a prepared native SH-4 module.
- Standalone VMU, always-on-top preference and Open / Show VMU control.
- Controller testing and configuration, optional console and heartbeat diagnostics.

## ChuChu Rocket! tester

The Windows package includes recompiled game code but **no CDI disc image**. You must supply your own compatible copy; the compiled DLL cannot start the game without the required disc data.

Reference: **MK-51049 / V1.007**, header region **JUE** (Japan / USA / Europe), volume **CHU_CHU_ROCKET_KAL**. Other revisions are not validated. Automatic selection checks title/volume, not an executable hash or exact revision.

Extract the entire Windows ZIP, start `DreamcastRecompiled.exe`, select your compatible CDI under Games, configure input and play. The prepared ChuChu path avoids rebuilding SH-4. Other games may require C++ build tools.

## Validation and limitations

- Original archive CRC check passed; the eight prepared game/host/plugin binaries match the bundled SHA-256 manifest.
- Launcher suite: **132 passed, 37 skipped, 0 failures** out of 169 discovered tests. Skipped tests require a graphical desktop.
- Updated outdated test expectations for the current runner, explicit heartbeat flags and UI wording; no runtime or game DLL changes in this publication review.
- Offline ChuChu gameplay was previously validated by the maintainer. This review did not run Windows gameplay or visually validate VMU taskbar behavior.
- Compatibility remains title-specific; online/DCNet is experimental.

Report game/revision, Windows version, CPU/GPU, controller, reproduction steps and logs. Do not attach your disc image.
