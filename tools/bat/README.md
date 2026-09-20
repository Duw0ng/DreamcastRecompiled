# Internal Windows scripts

The public v0.1 entry points remain in the repository root:

- `run_game.bat` — run a CDI.
- `build_windows.bat` — compile the project and run regression tests.
- `run_tests.bat` — explicit regression-test entry point.
- `check_system.bat` — check the Windows toolchain.
- `Configurar_Mando.bat` — create/update the controller profile.

Everything below this directory is for development, diagnostics, compatibility or historical reproduction. The scripts resolve the repository root automatically after being moved here.

- `tests/` — focused CPU/IR/codegen/regression launchers.
- `homebrew/` — KallistiOS and homebrew validation launchers.
- `probes/` — commercial diagnostic/performance/PVR probes.
- `legacy/` — historical title/version launchers kept for reproducibility.
- `dev/` — miscellaneous developer helpers.

New user documentation should point to `run_game.bat`, not to these internal launchers.
