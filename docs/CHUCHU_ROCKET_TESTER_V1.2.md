# ChuChu Rocket! — V1.2 native tester guide

DreamcastRecompiled V1.2 uses **ChuChu Rocket!** as its primary reference title for game-specific SH-4 native recompilation. The validated private Windows tester archive includes `games/CHUCHU_ROCKET/bin/dcr_game_module.dll` and the relevant host runtime/plugins.

## The compiled DLL is not a replacement for the disc

The game module contains host-native code derived from the original SH-4 executable, but **it is not a complete standalone copy of the game**. The launcher still requires the user to select their own compatible CDI image: it probes disc identity, prepares the bootstrap, and uses GD-ROM access to retrieve additional resources. No retail disc image is bundled.

## Tested disc identification

| Field | Value |
|---|---|
| Game | ChuChu Rocket! |
| Product ID | `MK-51049` |
| Game / IP version | `V1.007` |
| Region field | `JUE` — Japan, USA, Europe |
| Volume ID | `CHU_CHU_ROCKET_KAL` |
| Boot filename | `1ST_READ.BIN` |
| Disc date field | `20000217` |

Reference-disc SHA-256 from a historical local probe:

```text
CDI  6e95281b2feaa98e8b1327b33d8e60147c329cd314f633bba88eb25229031d3b
```

This hash identifies the disc used in the earlier validation record, **not** every compatible region/revision. `JUE` is the disc header's region field, not evidence that every released version has the same executable.

## Launch procedure (authorized tester package)

1. Extract the complete Windows x64 archive; keep `games/CHUCHU_ROCKET/`, `_internal/`, and `launcher/` alongside the launcher.
2. Start `DreamcastRecompiled.exe`.
3. Open **Games / Juegos** and select your own compatible ChuChu Rocket! CDI file. File name does not determine the game identity.
4. Start the game. The validated precompiled path checks the disc title and bundle manifest/checksums, prepares temporary boot files, and runs the native guest module. It should not regenerate the SH-4 module every time.
5. If a title/region check fails, do not force the ChuChu DLL onto a different game. Use the normal recompilation workflow for other titles.

Other titles may need Microsoft C++ Build Tools, CMake, and Windows SDK. Online/DCNet support remains experimental; this tester milestone validates **offline** gameplay first.

## What to report

Please report whether the game boots, reaches menus and gameplay, input works, sound/graphics are correct, FPS stability, and any crashes. Include launcher/session logs, Windows version, CPU, GPU, and controller, but **never upload your `.cdi`, game binary, or game-derived DLL**.

## Copyright and public distribution

Compiled native output can encode substantial copyrighted game program logic even without source disc data. The presence of an MIT-licensed recompilation tool does not grant redistribution rights to the resulting commercial game code. The private tester package is **not** automatically safe to upload as an unrestricted public GitHub asset; permission/rights review is needed. A public package can instead ship the recompiler/runtime and require users to compile their own original image locally.
