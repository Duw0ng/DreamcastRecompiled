# DreamcastRecompiled v1.2 Rev 2 — Full Aqua AICA Audio Test

**Experimental Windows x64 test distribution, NOT the v1.2 Stable release.**

## What's new

- Bundles the **AICA Rev 2** runtime used in the tested SGGG game-portable.
- AICA ARM7 2 MiB RAM mirrors and register-bus address decoding are corrected.
- When a **non-looping AICA one-shot voice** reaches its end, its hardware `KYONB` latch is cleared. A subsequent global `KYONEX` for an unrelated sound no longer incorrectly restarts that completed sample.
- Preserves the original **full Aqua launcher** (English/Spanish controls, VMU, game selector, diagnostics) and the precompiled ChuChu Rocket! SH-4 game module.
- Keeps the universal boot-stage correction for raw CDI tracks and `IP.BIN` unchanged.
- Includes the latest matching runtime source and regression tests so user-initiated runtime rebuilds retain the fix.

## Games to test

Open `DreamcastRecompiled.exe` and use your own compatible disc images. Compare sound effects in ChuChu Rocket!, SGGG, Sega Tetris, Mr. Driller and any other games in your library, noting menu navigation, gameplay, music, repeated sounds and freezes.

**Confirmed so far:** SGGG's repeated menu-navigation sounds were eliminated in the user-tested Rev 2 game-specific portable. This is not proof of perfect audio for all titles.

## Next target: Crazy Taxi 2 (FPS)

This package adds `TEST_CRAZY_TAXI_2_FPS.bat`, a performance-test entry point. It is a **measurement baseline and diagnostic**, not an unverified FPS patch.

1. Prepare Crazy Taxi 2 once through the full launcher if it does not already have a valid game-specific native SH-4 cache.
2. Run `TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi"` and drive to a reproducible scene. This disables heavy profiling but leaves SPG fast-forward and DX11 on, so the FPS number is useful.
3. Run `TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --profile` with the same scene. This enables **SH-4 HOTPC and PVR/CPU samples**; profiling reduces FPS and should only be used to identify bottlenecks.
4. Supply both session logs and specify the exact scene (menu, taxi selection, loading, city driving).

The launcher runs `scripts\windows\run_game.bat` with `--reuse`; it **never recompiles the game's native SH-4 module automatically**. A missing or invalid guest cache is reported as an error. Don't delete or merge per-game caches when switching between builds.

## How to test safely

Extract this entire ZIP to its own folder. Avoid overwriting an existing v1.2 installation. The published official Stable v1.2 ZIP and the four game-specific Rev 2 downloads remain unchanged.

No game disc image, commercial game content, or SEGA firmware is included. Changes to third-party applications and user saves are not required.
