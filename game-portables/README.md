# DreamcastRecompiled — Game Portables

**Windows x64 · [Games-Portables: all game downloads in one release](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/Games-Portables)**

Each portable is a separate ZIP, with a game-specific precompiled SH-4 module. **Download one game at a time; do not mix game DLLs.** Previous individual releases remain available as legacy links.

| Game | Download from Games-Portables | Status | Known issues / notes |
|---|---|---|---|
| **ChuChu Rocket!** | [ZIP](https://github.com/Duw0ng/DreamcastRecompiled/releases/download/Games-Portables/CHUCHU_ROCKET_portable_latest.zip) | **Tester — offline gameplay validated** | Approximately **60 FPS** observed in some tested sections, not universally guaranteed; **DCNet/online experimental**. Requires compatible CDI MK-51049 / V1.007 / CHU_CHU_ROCKET_KAL. |
| **Sega Tetris** | [ZIP](https://github.com/Duw0ng/DreamcastRecompiled/releases/download/Games-Portables/SEGA_TETRIS_portable_latest.zip) | **Experimental — gameplay reached** | Graphics and audio worked during tests of a community-patched Japanese revision; **intermittent crashes**. Other revisions unverified. |
| **Segagaga (SGGG)** | [ZIP](https://github.com/Duw0ng/DreamcastRecompiled/releases/download/Games-Portables/SGGG_portable_latest.zip) | **Experimental — partial execution** | Known **audio problems**, remaining **compatibility defects**; full gameplay unverified. |
| **Mr. Driller** | [ZIP](https://github.com/Duw0ng/DreamcastRecompiled/releases/download/Games-Portables/MR_DRILLER_portable_latest.zip) | **Experimental — gameplay reached** | Significant **graphics/rendering errors in menus** plus **audio defects** in previous tests; latest ZIP integrity checked, but not re-tested on Windows. |

All games require a compatible user-provided disc image. The portable packages contain no CDI/GDI game images or Dreamcast firmware. Experimental status does not guarantee complete gameplay or stable performance. The full application release **v1.2 Stable** remains separate from this collection.

## Install

1. Download the game-specific Windows x64 ZIP from its release and extract it completely.
2. Open `DreamcastRecompiled.exe` (or `ABRIR_LAUNCHER.bat`).
3. Select a compatible game disc image under **Games / Juegos**, then start.
4. For the standalone VMU, use Aqua's VMU menu or `ABRIR_LAUNCHER.bat --vmu`.

The validated ChuChu path runs an already-prepared game DLL without repeatedly recompiling SH-4. Network/DCNet is experimental, other revisions are unverified, and v1.2 is not a general Dreamcast compatibility guarantee.

## Publishing another portable

1. Add the new native Windows bundle at `games/GAME_ID/` in a source release ZIP. It needs `bin/dreamcast_program.exe`, `bin/dcr_game_module.dll`, `bin/DreamcastRuntime.dll`, `portable_manifest.txt` with `GAME=GAME_ID`, and `checksums.sha256` for its EXE/DLL files. Use a compatible DCR runtime and ABI.
2. Copy [template/game.template.json](template/game.template.json) to `games/GAME_ID.json` and add it to [catalog.json](catalog.json). Record the correct revision, disc volume, supported media and test status.
3. Copy [template/release.template.json](template/release.template.json) to `releases/new-game-vX.Y.json`. Point to an **already published** GitHub Release asset with its *exact SHA-256* and choose a unique portable release tag.
4. Validate: `python game-portables/tools/portable_games.py validate`. Local test: `python game-portables/tools/portable_games.py pack --spec game-portables/releases/new-game-vX.Y.json --source existing-source.zip --output dist`.
5. Commit to `main`. [Publish game portables](../.github/workflows/publish-portable-games.yml) handles unpublished specs, checks the pinned original ZIP, validates the game DLLs, repacks one game, creates SHA-256, and makes a game-specific GitHub Release. It refuses to overwrite existing tags.

Individual ZIPs include `DreamcastRecompiled.exe`, `_internal/`, `launcher/`, one `games/GAME_ID/`, any related `safe_guests/GAME_ID/`, controller/tools/scripts, legal notices and `PORTABLE_README.txt`. Unrelated game modules and build/test source are omitted.

## Rights and bug reports

DreamcastRecompiled is unaffiliated with Sega. MIT covers original DCR source only, **not** proprietary or game-derived code; maintainers must ensure they have the necessary distribution rights. Do not submit games, game executables, proprietary firmware or disc images as issues. Include CPU/GPU, Windows version, disc revision, controller and logs when reporting defects.

## Previous individual portable releases
- [Sega Tetris V1.2 tester](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sega-tetris-direct-v1.2) — tested gameplay, but intermittent crash reported.
- [SGGG V1.2 experimental](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2) — partial execution, known audio issues.

Read the [multi-game packaging guide](MULTI_GAME_PORTABLES.md). Each title uses its own native module; no commercial CDI is included.
