# DreamcastRecomp v0.1 Official — consolidated release

This package consolidates the latest controller/tester hotfix codebase into the single public **v0.1 Official** identity (`0.1.0`). No v0.1.3 functionality was removed; the point-release labels were development packaging labels and are folded into v0.1 Official.

> **v0.1 learning-start hotfix:** `Iniciar aprendizaje` now updates the objective immediately before waiting for input, keeps all learning state in script scope for Windows PowerShell 5.1 event handlers, and fixes button-31 scanning by using a 64-bit shift mask. The same mask fix is applied to `DreamcastControllerConfig.ps1`. The start button visibly changes to `Aprendizaje activo...` while learning is running.

# v0.1 — PS4 L2/R2 DirectInput calibration
> **v0.1 learning-UI hotfix:** the Controller Visual Tester now shows live RAW input feedback while guided learning is active. The current step is displayed prominently, wrong/non-matching controls are still reported instead of being silently ignored, and trigger capture shows the candidate axis/delta before release.


Native PS4/WinMM no longer guesses that U/V are independent analog triggers. The default mapping uses L2/R2 button bits with zero cross-talk. To recover analog pressure, run `run_controller_test.bat`, open **Aprendizaje**, complete the L2/R2 steps, and save the profile. The learned `range:<axis>:<rest>:<full>` form works with separate axes or with the single combined axis commonly exposed by DirectInput.

> **v0.1 anti-flicker hotfix:** the visual controller tester now uses WinForms double buffering and event-driven repainting instead of redrawing the full controller every 33 ms. Analog HID jitter is quantized to visible resolution, reducing unnecessary CPU/GDI work.

> **v0.1 tester hotfix:** `DreamcastControllerTest.ps1` is shipped as pure ASCII for compatibility with Windows PowerShell 5.1. D-pad glyphs are generated at runtime, avoiding UTF-8-without-BOM parser corruption on legacy PowerShell.


## v0.1 controller tester launcher path hotfix

- Fixes the PowerShell preflight in `run_controller_test.bat`: the tester path is now read from the `TESTER_PS1` environment variable instead of `$args[0]`.
- Adds an explicit `if not exist` check before invoking the PowerShell parser.
- Uses `Test-Path -LiteralPath` before `Parser::ParseFile`, avoiding failures with full Windows paths containing spaces or legacy `powershell.exe -Command` argument handling.


## v0.1 controller tester parser hotfix

- Fixed the PowerShell parser error in `tools/controller_test/DreamcastControllerTest.ps1` caused by a missing closing parenthesis in the learning-step status string.
- Replaced the fragile nested interpolation with `-f` formatting.
- `run_controller_test.bat` now validates the `.ps1` with the official PowerShell parser before launching it.

# DreamcastRecomp v0.1 Official — visual controller diagnostics

- Adds `run_controller_test.bat`, a live Windows controller tester for XInput and native DirectInput/WinMM pads.
- Visualizes Dreamcast-mapped D-pad/buttons, both analog sticks and L/R trigger values.
- Shows raw physical axes/button mask/POV beside the mapped guest values.
- Reads the same `profiles/controller_profile.ini`, backend selection, deadzone and binding syntax used by normal game runs.
- Adds guided input learning for unusual controllers; learned values are only written after explicit Save.
- Intended especially for validating DualShock 4 USB/Bluetooth without DS4Windows.

# DreamcastRecomp v0.1 Official

DreamcastRecomp v0.1 is the first release presented as a stable public milestone instead of an internal `0.0.x` development checkpoint.

## v0.1.1 controller patch

v0.1.1 expands the default Windows input bridge beyond XInput. Native DualShock 4 devices exposed through DirectInput/WinMM are detected automatically, including their POV/D-pad, standard PS4 face-button layout, both sticks, Options/START, shoulder buttons and L2/R2 fallback. The default profile uses backend-neutral `logical:*` mappings, while raw per-button/axis mappings remain available through `Configurar_Mando.bat`.

## Release identity

- Public version: **v0.1 Official**
- Internal SemVer: **0.1.0**
- Development lineage: **0.0.210**
- Canonical Windows launcher: **`run_game.bat`**

## User-facing changes

- One launch syntax for commercial CDI testing.
- Automatic maintained profiles for Daytona USA, Crazy Taxi 2, ChuChu Rocket!, and Record of Lodoss War.
- Optional `--perf`, `--debug`, `--profile`, `--software`, `--no-audio`, and `--clean` modes.
- Drag-and-drop CDI support through the same launcher.
- `check_system.bat` preflight helper.
- Historical game launchers retained as compatibility aliases instead of being the documented path.
- Version identity normalized in CMake, generated runners, window titles, heartbeats, and session log names.

## Architectural statement

The long-term goal remains maximum native recompilation. v0.1 is deliberately **hybrid**: SH-4 program code is statically analyzed/recompiled to host C++, while Dreamcast hardware behavior and still-unconverted execution surfaces are provided by runtime models/HLE or emulation where required. This hybrid layer is what makes it possible to validate newly recompiled SH-4 code against realistic PVR, AICA, Maple, Holly, GD-ROM and timing behavior while native coverage grows.

DreamcastRecomp therefore does **not** claim that v0.1 is already a 100% native replacement for every Dreamcast subsystem.


## Validation

See [`VALIDATION_V0.1.md`](VALIDATION_V0.1.md) for the final build/test scope and platform caveats.


## Clean release layout

The public ZIP keeps the repository root intentionally small. Normal users should mainly need `README.md`, `run_game.bat`, `check_system.bat`, `Configurar_Mando.bat`, `build_windows.bat` and `run_tests.bat`. Historical validation material is under `docs/archive/`, internal launchers are under `tools/bat/`, and screenshot evidence is under `test-img/`.
