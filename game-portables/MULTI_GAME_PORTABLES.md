# Lightweight game portables — Sega Tetris & Segagaga

The direct portable format starts with `DreamcastRecompiled.exe`, a matching game-specific `dcr_game_module.dll`, DreamcastRuntime and selected hardware plugins. No Python or C++ installation is needed for the prepared game module.

| Game | Standalone ZIP | Maintainer observations | Caveat |
|---|---|---|---|
| [**Sega Tetris**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sega-tetris-direct-v1.2) | 10.9 MiB / 47 files | Gameplay, audio and graphics reached in a community English-patched Japanese build | Occasional crash; experimental |
| [**Segagaga (SGGG)**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2) | 14.4 MiB / 11 files | Partial boot/execution observed | Known audio and compatibility issues; experimental |
| [**ChuChu Rocket!**](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-chuchu-rocket-direct-v1.2) | 13.6 MiB / 11 files | Offline gameplay, up to ~60 FPS reported in some tested sections | Requires compatible disc; network experimental |

Each release is binary-identical to the corresponding approved project ZIP, verified by SHA-256 and ZIP CRC. The source files for these assets were transferred as separate Git blob chunks, listed in `game-portables/binary-sources`. The GitHub Actions [release workflow](../.github/workflows/publish-direct-binaries.yml) reconstructs them, verifies their guest ID, size, cryptographic digest, CRC, and absence of disc images before publication.

**Requirements:** Windows x64. Unzip the archive fully, run `DreamcastRecompiled.exe`, and select your own compatible retail game CDI/GDI. No game discs, BIOS or Sega firmware are included. Per-game modules are not interchangeable, and the presence of a precompiled module does not mean universal compatibility.

**Support:** Please attach logs, Windows version, CPU/GPU, the game revision, and the failure details. Do not upload retail game data. This independent recompiler's MIT license only covers original project material; commercial game code retains third-party rights.
