# cpp36b1 — AUTO_PORTABLE_GUEST

## Goal

A commercial guest is compiled only once and the resulting `dcr_game_module.dll` is usable both by the normal development runner and by the portable launcher.

The old split pipeline generated two different guests:

- normal build: `.raw_boot`/`BOOTSTRAP.BIN` embedded in `dcr_game_module.dll`;
- portable builder: a second SH-4 generation with `--no-embed-raw-image`.

cpp36b1 removes that split. New commercial recompilations always use `--no-embed-raw-image`. Retail bytes are prepared locally from the selected CDI/GDI and supplied to the precompiled runner at execution time with `--external-raw-image`.

## Normal flow after migration

```text
run_game.bat game.cdi --reuse
        |
        +-- reuse dcr_game_module.dll (no SH-4 rebuild)
        +-- update runtime/plugins if needed
        +-- auto-sync ready portable folder
        +-- auto-sync portable_latest.zip
        +-- execute normal test
```

No separate portable builder is required.

## One-time migration of old caches

Normal guests produced before cpp36b1 contain `.raw_boot` in the DLL. They cannot be made redistribution-safe by copying the DLL. Use once:

```bat
run_game.bat "game.cdi" --portableize
```

This is the final format transition. The newly generated guest is raw-free and can then be reused by later runtime/plugin versions as long as Guest ABI1 remains compatible.

## Portable output

For the detected game cache:

```text
%LOCALAPPDATA%\DreamcastRecompiled\games\<GAME>\portable\
%LOCALAPPDATA%\DreamcastRecompiled\games\<GAME>\<GAME>_portable_latest.zip
```

The portable contains the modern launcher, `dreamcast_program.exe`, `dcr_game_module.dll`, `DreamcastRuntime.dll`, Network/Maple plugins, `dc_disc_probe.exe`, and `dc_boot_prepare.exe`.

It does **not** contain CDI/GDI, IP.BIN, BOOT.DISC.BIN, BOOT.BIN, or BOOTSTRAP.BIN.
