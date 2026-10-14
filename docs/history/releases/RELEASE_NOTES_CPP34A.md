# DreamcastRecomp v1.2 cpp34a — MODULAR_RUNTIME_ABI1

This is the one-time migration from monolithic cpp33 builds to a persistent Guest ABI.

## What changes

- Generated SH-4 is isolated in `dcr_game_module.dll`.
- Host/device runtime is isolated in `DreamcastRuntime.dll`.
- Modem/PPP/UDP/DNS is the first real replaceable device plugin: `dcr_network.dll`.
- A generic ABI reserves PVR, AICA, GD-ROM and Maple plugin kinds for later extraction without another guest-code migration.
- Per-game generated source/build cache defaults to `%LOCALAPPDATA%\DreamcastRecompiled\games`.
- Console flash and VMU default to `%LOCALAPPDATA%\DreamcastRecompiled\system`.

## Development loop

The first cpp34 launch of a cpp33/pre-ABI game cache requires one full SH-4 compile. After that:

```bat
run_game.bat "game.cdi"
run_game.bat "game.cdi" --rebuild-network
run_game.bat "game.cdi" --rebuild-runtime
```

Use `--recompile-game` only when SH-4 code generation or Guest ABI changes.

## Validation

- 54/54 core tests PASS.
- Dynamic plugin load PASS.
- Network-only rebuild: 0 generated guest objects compiled.
- Cross-release-path runtime rebuild: 0 generated guest objects compiled.
