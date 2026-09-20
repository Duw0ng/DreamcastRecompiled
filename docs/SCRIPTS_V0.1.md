# Scripts — DreamcastRecomp v0.1 Official

## Public interface

The supported Windows entry point is:

```bat
run_game.bat "C:\Games\game.cdi" [options]
```

Supported options:

| Option | Purpose |
|---|---|
| `--perf` | Low-overhead runtime profiling. |
| `--debug` | Extra diagnostics and faster heartbeat reporting. |
| `--profile` | Detailed PVR profiling; intentionally slower. |
| `--software` | Force the software PVR path where the profile supports a GPU path. |
| `--no-audio` | Keep AICA/ARM7 logic active but disable host audio playback. |
| `--clean` | Delete the v0.1 generated workspace and rebuild it. |
| `--help` | Print command help. |

Examples:

```bat
run_game.bat "D:\Dreamcast\ChuChu Rocket.cdi"
run_game.bat "D:\Dreamcast\Crazy Taxi 2.cdi" --perf
run_game.bat "D:\Dreamcast\Daytona USA.cdi" --debug
run_game.bat "D:\Dreamcast\Record of Lodoss War.cdi" --clean
```

`run_game.bat` identifies known titles from the disc metadata and selects the maintained profile automatically. Unknown CDI images use the generic commercial path.

## Other user-facing scripts

- `check_system.bat`: checks the minimum Windows toolchain.
- `build_windows.bat`: builds the core tools and runs the regression suite.
- `Configurar_Mando.bat`: creates/updates the optional controller profile.

## Historical launchers

Historical game/version launchers are kept under `tools/bat/legacy/` so the release root stays clean. Compatibility wrappers still resolve the repository root and forward to the maintained v0.1 paths where applicable. New documentation should not use the old names.

## Developer / diagnostic BATs

Developer and diagnostic BATs now live under `tools/bat/`:

- `tools/bat/tests/` — focused regression launchers.
- `tools/bat/homebrew/` — KallistiOS/homebrew validation.
- `tools/bat/probes/` — commercial/PVR/performance diagnostics.
- `tools/bat/legacy/` — historical compatibility launchers.
- `tools/bat/dev/` — miscellaneous developer helpers.

They are intentionally **not** part of the stable end-user CLI because many operate on ELF files, generated projects, or a previously built runtime rather than on a CDI. Use `run_tests.bat` from the root for the normal regression suite.
