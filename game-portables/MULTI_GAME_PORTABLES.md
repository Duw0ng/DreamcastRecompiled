# Lightweight game portables — Sega Tetris & Segagaga

The direct portable format starts with `DreamcastRecompiled.exe`, a matching game-specific `dcr_game_module.dll`, DreamcastRuntime and selected hardware plugins. No Python or C++ installation is needed for the prepared game module.

| Game | Standalone ZIP | Maintainer observations | Caveat |
|---|---|---|---|
| [**Sega Tetris**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sega-tetris-direct-v1.2) | 10.9 MiB / 47 files | Gameplay, audio and graphics reached in a community English-patched Japanese build | Occasional crash; experimental |
| [**Segagaga (SGGG) v1.2 Rev 2**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2) | ~15.1 MB | Boot and menu navigation audio validated | AICA Rev 2 eliminates confirmed repeated menu SFX; other compatibility limitations may remain |
| [**ChuChu Rocket!**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-chuchu-rocket-direct-v1.2) | 13.6 MiB / 11 files | Offline gameplay, up to ~60 FPS reported in some tested sections | Requires compatible disc; network experimental |

The current releases were upgraded to **v1.2 Rev 2 on 17 October 2026**, reusing their familiar direct portable asset names. The original SHA-256-verified v1.2 files remain archived under release tags ending in `-initial-archive`. The original Git blob chunk manifests in `game-portables/binary-sources` describe those initial historical sources, not the upgraded Rev 2 ZIPs. The [Rev 2 publication workflow](../.github/workflows/publish-rev2-direct-portables.yml) validates exact user-tested artifacts, backs up the originals and checks public asset hashes after uploading.

**Requirements:** Windows x64. Unzip the archive fully, run `DreamcastRecompiled.exe`, and select your own compatible retail game CDI/GDI. No game discs, BIOS or Sega firmware are included. Per-game modules are not interchangeable, and the presence of a precompiled module does not mean universal compatibility. All current four direct portables include the shared raw CDI plain/scrambled boot-stage fix and SGGG-confirmed AICA Rev 2 one-shot audio fix.

**Support:** Please attach logs, Windows version, CPU/GPU, the game revision, and the failure details. Do not upload retail game data. This independent recompiler's MIT license only covers original project material; commercial game code retains third-party rights.
