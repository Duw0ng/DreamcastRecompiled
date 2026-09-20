# DreamcastRecomp v0.1 Official

**Static Dreamcast recompilation for PC, with a pragmatic hybrid runtime while native coverage grows.**

> v0.1 is the first official public milestone. It consolidates the long `0.0.x` development line into a single user-facing release and a single recommended Windows launcher.

## What DreamcastRecomp is

DreamcastRecomp analyzes Dreamcast SH-4 programs, discovers reachable code, translates supported SH-4 instructions into an intermediate representation, and emits native C++ that is compiled for the host PC.

The project is **not yet 100% native end-to-end**. Its target is to move as much execution as practical into native recompiled host code, but v0.1 intentionally uses a hybrid architecture while that work continues:

```text
Dreamcast CDI / ELF
       |
       v
SH-4 analysis + reachability / closure
       |
       v
DCIR / generated native C++  ---------> host compiler ---------> native executable
       |                                                        |
       +---------------- Dreamcast runtime ---------------------+
                        PVR / AICA / Maple / Holly / GD-ROM
                        timing / BIOS-HLE / device models
                        selective emulation/HLE where needed
```

That hybrid layer is important during development: it lets newly recompiled SH-4 code run against realistic Dreamcast device behavior instead of requiring every subsystem to become native at the same time.

## v0.1 at a glance

- **SH-4:** static decoding, control-flow discovery, native C++ emission, dynamic/indirect target closure and multiple commercial-title compatibility passes.
- **PVR/TA:** software rendering plus D3D11 acceleration/fallback paths, texture formats, sprites, blending, depth, clipping, Store Queue/CH2 ingestion and extensive diagnostics.
- **AICA/ARM7:** ARM7 execution, native slot mixing, PCM/ADPCM paths, timers/interrupt work, WinMM output and CDDA support used by tested titles.
- **Maple:** keyboard/XInput controller bridge, DMA/device handling and persistent VMU work used by tested software.
- **Holly / timers / GD-ROM:** interrupt/event plumbing, SH-4 TMU support, CDI parsing, disc maps and commercial boot HLE used by the current compatibility path.
- **Commercial boot:** CDI probing, IP.BIN / boot extraction, bootstrap preparation and symbol-free SH-4 closure.
- **Diagnostics:** heartbeats, session logs, function maps, perf profiling, PVR probes and historical validation reports.

The repository still contains experimental and diagnostic code. A green item means the tested path is working for the listed checkpoint, **not** that the entire Dreamcast subsystem is cycle-perfect or complete for every game.

## Quick start — Windows

### 1. Requirements

Install **Visual Studio 2022** or **Build Tools 2022** with:

- Desktop development with C++
- CMake tools for Windows
- Windows 10/11 SDK

Optional first check:

```bat
check_system.bat
```

### 2. Run a game

Use your own legal Dreamcast CDI image. Commercial game data is not included in this repository.

```bat
run_game.bat "C:\Games\game.cdi"
```

You can also drag a `.cdi` directly onto `run_game.bat`.

### 3. Common modes

```bat
run_game.bat "C:\Games\game.cdi" --perf
run_game.bat "C:\Games\game.cdi" --debug
run_game.bat "C:\Games\game.cdi" --profile
run_game.bat "C:\Games\game.cdi" --software
run_game.bat "C:\Games\game.cdi" --no-audio
run_game.bat "C:\Games\game.cdi" --clean
```

| Flag | Use |
|---|---|
| `--perf` | Low-overhead performance profile. |
| `--debug` | Extra diagnostics and 500 ms heartbeat. |
| `--profile` | Detailed PVR profiling; slower by design. |
| `--software` | Force software PVR instead of the GPU path where applicable. |
| `--no-audio` | Disable host audio output while keeping ARM7/AICA execution active. |
| `--clean` | Rebuild the v0.1 generated workspace from scratch. |
| `--help` | Show the launcher help. |

Historical title/probe BATs are grouped under `tools/bat/` so the root stays focused on normal use. New users should use `run_game.bat`. See [`docs/SCRIPTS_V0.1.md`](docs/SCRIPTS_V0.1.md).

## Build only

```bat
build_windows.bat
```

This configures the x64 CMake project, builds the core tools in Release mode and runs the regression tests.

## Controller setup

The default `auto` input path now supports **keyboard, XInput and native DualShock 4 / DirectInput**. A PS4 controller can therefore be connected directly by USB or Bluetooth without DS4Windows. On native PS4 input, the D-pad is read from the Windows POV hat instead of being mistaken for XInput button numbers.

Recommended behavior:

- Xbox/XInput/DS4Windows/Steam Input: detected through XInput first.
- DualShock 4 without DS4Windows: falls back automatically to DirectInput/WinMM.
- Native DS4 mapping: Cross=A, Circle=B, Square=X, Triangle=Y, Options=START, L1=C, R1=Z, D-pad=POV, plus both sticks and L2/R2.
- If Windows exposes several legacy gamepads, DreamcastRecomp scans additional WinMM slots when the selected one is unavailable.

To inspect or remap a controller:

```bat
Configurar_Mando.bat
```

The configurator now includes **Auto**, **PS4 nativo** and **XInput** presets. If `profiles/controller_profile.ini` exists, `run_game.bat` loads it automatically. Profiles can also use backend-neutral `logical:*` bindings so one profile works with both XInput and native PS4 input.

### Visual controller tester

Run:

```bat
run_controller_test.bat
```

The v0.1 tester shows **the values after Dreamcast mapping**, not only raw Windows input. It includes a live D-pad/button view, two analog-stick plots, 0-255 trigger bars, a raw XInput/DirectInput diagnostics tab, and a guided **Aprendizaje** mode that can identify unusual axes/buttons and save them to `profiles/controller_profile.ini`. This is especially useful for checking a DualShock 4 without DS4Windows. See `docs/CONTROLLER_TESTER.md`. Native PS4 L2/R2 no longer guess U/V axes: the safe default is independent digital trigger bits, while **Aprendizaje** can calibrate real analog trigger ranges without DS4Windows.

Default keyboard mapping:

| Dreamcast | Keyboard |
|---|---|
| D-pad | Arrow keys |
| Analog | WASD |
| A | Z / J / Space |
| B | X / K |
| X | C / U |
| Y | V / I |
| START | Enter |

## Compatibility snapshot — v0.1

These are project validation checkpoints, not a promise of complete compatibility across every region/revision of a game.

| Title | Boot | Menus | Graphics | Audio | Controls | Gameplay | Notes |
|---|---:|---:|---:|---:|---:|---:|---|
| **ChuChu Rocket!** | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | Primary long-running 2D/commercial regression target; real gameplay has been reached in development. |
| **Crazy Taxi 2** | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟡 | Title/menu and substantial runtime paths work; gameplay remains an active compatibility/performance target. |
| **Daytona USA 2001** | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟡 | Validated through `PRESS START` and moving 3D attract/demo paths; real race gameplay remains under active validation. |
| **Record of Lodoss War** | 🟢 | 🟡 | 🟡 | 🟡 | 🟢 | 🟡 | v0.1 carries the 0.0.210 secondary-executable/START compatibility work; broader gameplay still WIP. |

Legend: 🟢 validated working path · 🟡 partial / work in progress · 🔴 known non-working path · ⚪ not validated.

## Native vs emulated / HLE status

DreamcastRecomp should be understood as a **recompiler-first hybrid runtime** today.

| Area | v0.1 approach | Status |
|---|---|---|
| SH-4 game code | Static analysis + generated native C++ | Core project path; coverage depends on discovered closure. |
| SH-4 unresolved/dynamic behavior | Runtime dispatch/compatibility machinery | Still required for robust commercial execution. |
| PVR / TA | Host runtime model; software + D3D11 paths | Substantial implementation, not cycle-perfect. |
| ARM7 / AICA | ARM7 execution + host-side AICA device/mixer model | Functional subset used by validated audio paths. |
| Maple / VMU | Runtime device model + host input/persistence | Functional subset used by tested software. |
| Holly / TMU / interrupts | Runtime hardware model | Partial but actively used by commercial paths. |
| GD-ROM / CDI | Native host parser + HLE/runtime disc model | Functional for current tested CDI workflows. |
| BIOS services | HLE / synthetic apertures where needed | No Sega BIOS is redistributed. Optional user-provided ROM support exists for specific paths. |

The long-term direction is to reduce scaffolding and emulated/HLE dependencies where native recompilation or cleaner host-native models make sense, without sacrificing correctness just to claim a higher “native percentage.”

## Repository layout

```text
include/        Public/core C++ headers
src/            Analyzer, recompiler, runtime/codegen and disc tooling
tests/          Core regression tests
docs/           Current docs; historical material is under docs/archive/
profiles/       Maintained title/controller profiles
corpus/         KallistiOS compatibility reports / corpus metadata
tools/          Build/profiling helpers and internal BAT launchers
test-img/       Screenshot evidence used during compatibility testing
run_game.bat    Canonical v0.1 Windows game launcher
build_windows.bat / run_tests.bat / check_system.bat
                Public build, test and environment helpers
```

Generated commercial data is written under `generated/` and local build folders. Do not commit or redistribute copyrighted game images or extracted commercial binaries.

## Testing philosophy

The project has been developed by repeatedly moving from synthetic CPU tests to KallistiOS homebrew and then to real commercial execution. Important regressions are kept as tests or validation documents instead of relying only on “it booted once.”

For the normal regression suite use:

```bat
run_tests.bat
```

Focused developer launchers are grouped under `tools\bat\tests`, `tools\bat\homebrew` and `tools\bat\probes`. They are intentionally separate from the stable `run_game.bat` CLI because many take ELF files, generated runtimes or specialized probe inputs rather than a CDI.

## Logs and bug reports

Generated runners create session logs named like:

```text
DreamcastRecomp_v0.1_session_YYYYMMDD-HHMMSS.log
```

When reporting a failure, include:

- exact game/region/revision if known;
- launcher command used;
- the complete session log;
- the last visible screen/state;
- whether `--perf`, `--debug`, `--profile` or `--software` was active;
- CPU/GPU and Windows version for performance/rendering bugs.

Do **not** upload copyrighted CDI/BIOS files to public bug reports.

## Credits and technical references

DreamcastRecomp is an independent project. Its implementation and debugging benefited heavily from public Dreamcast documentation, open-source hardware models, recompilation prior art and homebrew ecosystems, especially:

- **[dcrecomp](https://github.com/sp00nznet/dcrecomp)** by **sp00nznet** — an important source of inspiration and prior art for expanding DreamcastRecomp's SH-4 analysis/recompilation system and pushing its SH-4 coverage toward a more complete implementation. dcrecomp is a separate MIT-licensed project and remains the work of its own authors.
- **[Flycast](https://github.com/flyinghead/flycast)** — major reference for Dreamcast hardware behavior and parity audits.
- **[KallistiOS](https://github.com/KallistiOS/KallistiOS)** — Dreamcast homebrew OS, headers, examples and the compatibility corpus used throughout development.
- **Marcus Comstedt's Dreamcast technical documentation** — low-level hardware documentation: https://mc.pp.se/dc/
- **Dreamcast Wiki / community documentation** — hardware and software-development reference material: https://dreamcast.wiki/

These references and inspirations do not imply code ownership, endorsement, affiliation, or relicensing. Third-party projects retain their own copyright and license terms. See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for a concise attribution/licensing summary. Current release validation is in `docs/release/`; historical milestone evidence is grouped under `docs/archive/`.

## License

DreamcastRecomp's original project code is released under the **MIT License**. See [`LICENSE`](LICENSE).

```text
Copyright (c) 2026 Duw0ng
```

The MIT license applies to code and documentation authored for this repository unless a file explicitly states otherwise. It allows use, modification, redistribution and commercial use under the conditions of the MIT license, including preservation of the copyright and license notice.

**Important scope notes:**

- Third-party projects, source code, documentation and assets referenced by DreamcastRecomp keep their original licenses and copyright ownership. DreamcastRecomp's MIT license does not relicense Flycast, KallistiOS, dcrecomp or any other external project.
- References to external implementations are for technical research, compatibility work and prior-art acknowledgement. Any reused third-party code must continue to satisfy that code's original license and attribution requirements.
- The license does **not** grant rights to Sega firmware, Dreamcast BIOS/flash data, commercial games, game assets, extracted executables or other copyrighted third-party material.
- Recompiled or generated output derived from commercial software may still contain or represent copyrighted material belonging to the original rights holders; users are responsible for ensuring they have the legal right to use the input and resulting output.

See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for external-project attribution notes.

## Legal

DreamcastRecomp does not include Sega firmware, commercial games, or copyrighted game assets. Users are responsible for supplying any software/firmware they are legally entitled to use and for complying with applicable law.

Dreamcast is a trademark of SEGA. DreamcastRecomp is an independent project and is not affiliated with, authorized by, sponsored by, or endorsed by SEGA.

## Release

**DreamcastRecomp v0.1 Official**  
Internal version: `0.1.0`  
Development lineage: `0.0.210`

See [`RELEASE_NOTES_V0.1.md`](RELEASE_NOTES_V0.1.md), [`docs/release/VALIDATION_V0.1.md`](docs/release/VALIDATION_V0.1.md) and [`CHANGELOG.md`](CHANGELOG.md) for details.
