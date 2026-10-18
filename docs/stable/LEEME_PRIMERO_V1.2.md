# Dreamcast Recompiled v1.2

**Official stable baseline — cpp38e HOT_BLOCK_RECOVERY, frozen October 7, 2026.**

This release is based on the authentic archive `DreamcastRecomp_v1.2_STABLE_cpp38e_HOT_BLOCK_RECOVERY.zip`, originally checked with SHA-256:

```text
1f3722dfa1f962c95b5d9594eb4f88dbb4a25860961869ef03a9bf1f5e4e7f06
```

## Launch on Windows

From PowerShell in this directory:

```powershell
.\INICIAR_DREAMCAST_RECOMPILED_V1_2.bat "C:\path\ccr.cdi"
```

You can also drag a CDI file onto `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`.

This uses the same stable `TEST_CPP38E_FAST.bat` workflow with flags `--reuse --spg-ff-on --hotpc-off --hot-recovery-on --no-portable-sync --no-pause`. To load another title for the first time or generate its guest cache, use `run_game.bat "C:\path\game.cdi"` after consulting the main README.

## What remains unchanged

- The original cpp38e C++ recompiler, SH-4, legacy PVR, AICA, GD-ROM, Maple, and networking code.
- Existing `run_game.bat`, `TEST_CPP38E_FAST.bat`, `APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat`, and other batch files.
- The ABI1 guest `safe_guests/CHUCHU_ROCKET/dcr_game_module.dll` and original profiles.
- Recompiler/cache identifiers (`DCR_RUNTIME_BUILD_ID` and `DCR_GUEST_IMPL_REV`), which **must not be changed** merely for public branding.
- HOTPC profiling remains disabled by default in the baseline configuration.

## Presentation changes in the original snapshot

- The public folder, delivery, and launcher name became **Dreamcast Recompiled v1.2**.
- The visible UI/title strings in the Go-built launcher `portable/DreamcastRecompiled.exe` and its source `portable/launcher/main_windows.go` were revised. The original internal loading configuration remained unchanged.
- This document was added. No game logic, clock rate, rendering, or audio routines were changed for that branding step.

The portable executable was included, but the earlier confirmed stable performance measurement concerned the `TEST_CPP38E_FAST.bat` path. The portable UI was cross-built for Windows x64; **it was not run on Windows in that original validation session**.

## Before replacing an experimental copy

Do not mix cpp38az/cpp38ba files into this directory. Extract the ZIP into its own folder. Per-game data is stored in `%LOCALAPPDATA%\DreamcastRecompiled\games`; merely extracting cpp38e sources does not erase that data. Back up any mixed cache before repairing it, and avoid `--clean` if you intend to preserve existing cache entries.

Original snapshot history: `STABLE_BASE_CPP38E.md`, `CPP38E_HOT_BLOCK_RECOVERY.md`, and `RELEASE_NOTES_CPP38E.md`.
