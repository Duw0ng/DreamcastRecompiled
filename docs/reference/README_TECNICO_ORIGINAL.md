# Dreamcast Recompiled v1.2 — Edicion estable

**Base oficial: cpp38e HOT_BLOCK_RECOVERY.** Esta distribucion restaura la fuente congelada el 2026-10-07. Para iniciar ChuChu Rocket! con los parametros validados, consulta [LEEME_PRIMERO_V1.2.md](LEEME_PRIMERO_V1.2.md) y usa `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat "ruta\ccr.cdi"`. El contenido que sigue es la documentacion tecnica historica intacta de cpp38e.

---

# DreamcastRecompiled v1.2 cpp38b2

Current focus: **historical performance recovery**. cpp38b2 keeps cpp38b1's index-scratch PVR path, historical `superblock` mode and 262144-cycle host-sync cadence, and fixes the telemetry bug that still left Superblock FPU counters active. The production guest now compiles those generated metrics out. See `CPP38B2_FPU_METRICFREE_RECOVERY.md`.

> **cpp37c GDROM_CDDA_NATIVE:** `dcr_gdrom.dll` ABI2 V3 now owns Red-Book CDDA transport and decode state in addition to cpp37a/b media, TOC, command lifecycle and DMA. PLAY_TRACKS/PLAY_SECTORS/PAUSE/RELEASE/STOP/SEEK, raw 2352-byte audio reads, byte-order analysis and Yellow-Book compatibility now live in the plugin. DreamcastRuntime keeps only the thin 75 Hz scheduler/audio-mix bridge plus fallback. PCM crosses the DLL boundary in 64-frame batches. Guest ABI1 and the pinned SAFE guest remain unchanged.

> **cpp37b GDROM_COMMAND_DMA_NATIVE:** `dcr_gdrom.dll` ABI2 V2 now owns the GD command lifecycle state, multi-transfer cursor/progress and G1/GD DMA transfer path in addition to cpp37a DiscMap/TOC/bulk media I/O. DMA bulk copies use Host Services V2 memory regions and the plugin raises/acks the GD-DMA IRQ itself. CDDA playback/drive-audio lifecycle deliberately remains in DreamcastRuntime for cpp37c. Normal and portable game windows now use short names such as `DreamcastRecompiled - CHUCHU ROCKET [cpp37b]`. Guest ABI1 and the pinned SAFE guest remain unchanged.

> **cpp37a GDROM_NATIVE_PLUGIN_PHASE1:** `dcr_gdrom.dll` is now ABI2 plugin kind 4 and owns DiscMap V1/V2 parsing, track geometry, sessions, TOC generation and bulk logical sector reads. DreamcastRuntime keeps only a compatibility snapshot for CDDA/command paths that will migrate in cpp37b/c. `DCR_GDROM_TRACE=1` and the launcher v0.8 checkbox expose media routing. Guest ABI1 and the pinned SAFE ChuChu guest remain unchanged.

> **cpp36b1g SONY_HAPTICS_VISUAL_V2:** Modern Maple tester now uses a more authentic Dreamcast-shaped live pad, double-buffered/state-change painting to reduce flicker, and haptics routed through `dcr_maple.dll`. XInput, Sony DualShock 4 and DualSense/PS5 classic rumble are supported on Windows via native HID output reports. The tester's `Test rumble (PS4 / PS5)` button exercises the same backend used by Dreamcast PuruPuru. Guest ABI1 remains frozen.

> **cpp36b1f:** The modern controller setup now includes a full live Dreamcast pad view. Select Port A/B/C/D and a host device; the drawing reacts to the mapped Dreamcast buttons, stick and triggers in real time. This is a launcher-only update and does not require rebuilding the guest.

# cpp36b1e — Runtime external raw ENV compatibility

`cpp36b1e` fixes portable-safe Guest ABI1 execution with older cached `dreamcast_program.exe` runners. The locally prepared `BOOTSTRAP.BIN` is now supplied to `DreamcastRuntime.dll` through `DCR_EXTERNAL_RAW_IMAGE` / `DCR_EXTERNAL_RAW_BASE`, so `--reuse` does not need to rebuild or relink the guest runner just to understand a new CLI option. Auto-portable sync remains enabled.

> **cpp36b1d AUTO_PORTABLE_SYNC_FIX:** robust SAFE guest detection now marks the existing pinned ChuChu ABI1 DLL portable-safe even with padded title/volume metadata. `--reuse` creates/refreshes the portable automatically; `--sync-portable` does the same and exits without launching. No SH-4 rebuild.

> **cpp36b1a AUTO_PORTABLE_SAFE_SEED:** includes the previously built ChuChu Rocket online portable `dcr_game_module.dll` as a SHA-256 pinned Guest ABI1 safe seed. If an existing ChuChu ABI1 cache is still present but is marked legacy/non-portable, `run_game.bat --reuse` adopts this seed automatically, writes the portable-safe marker, refreshes runtime/plugins and auto-syncs the portable folder/ZIP **without recompiling SH-4**.

> **cpp36b1 AUTO_PORTABLE_GUEST:** new commercial guest compilations are portable-safe by default. The normal `run_game.bat` now auto-synchronizes a ready portable folder/ZIP from the same ABI1 guest, so no separate portable builder/recompile is required after the one-time migration of legacy embedded-raw caches.

> **cpp36b PURUPURU_MODERN_INPUT:** Maple V3 now adds a real PuruPuru expansion, native XInput rumble, and independent host-device selection for Dreamcast ports A-D. Keyboard + Mouse is one combined virtual controller; XInput/DirectInput devices are addressable individually. Guest ABI1 remains frozen. See `CPP36B_PURUPURU_MODERN_INPUT.md`.

> **cpp36a MAPLE_DEVICE_TREE:** `dcr_maple` now exposes a port-aware V3 device tree with controller ports A-D, two expansion slots per port, KOS/HLE multi-controller enumeration, and optional `DCR_MAPLE_TRACE=1`. Guest ABI1 remains frozen, so existing `dcr_game_module.dll` caches are reused. See `CPP36A_MAPLE_DEVICE_TREE.md`.

> **cpp35b1 CACHE_ABI_FREEZE:** Guest ABI v1 is now formally frozen. A valid cpp34/cpp35 `dcr_game_module` is reused without rewriting its headers/CMake metadata; runtime, Network and Maple rebuild from a separate CMake tree. Use `TEST_CPP35B1_NO_GUEST_REBUILD.bat` to guarantee that a test aborts rather than silently recompiling SH-4.

> **cpp35b MAPLE_PRIVATE_STATE:** Plugin Runtime V2 now moves Maple controller/VMU/DMA ownership into `dcr_maple.dll` without changing Guest ABI1. The cpp34 built-in path remains as fallback. See `CPP35B_MAPLE_PRIVATE_STATE.md` and `VALIDATION_CPP35B.md`.

> **cpp35a Plugin Runtime V2 checkpoint:** Guest ABI1 remains frozen. `DreamcastRuntime.dll` can now host ABI1 and ABI2 plugins side-by-side; `dcr_maple.dll` is the first ABI2 device boundary, while `dcr_network.dll` remains the proven cpp34s ABI1 implementation. cpp35a deliberately keeps a compatibility bridge to the known-good Maple implementation before moving all Maple private state in the next extraction step.

# DreamcastRecomp v1.2 cpp35a PLUGIN_RUNTIME_V2_MAPLE_BRIDGE

> **Modular development build:** `v1.2 cpp34c FLASH_BASE0_FIX_ABI1` keeps the crash-free v1.2/CPP32A guest translation and introduces a persistent Guest ABI v1. The SH-4 closure is built as `dcr_game_module.dll`, host/device logic lives in `DreamcastRuntime.dll`, and network is the first independently replaceable plugin: `dcr_network.dll`.

# DreamcastRecomp v1.2 cpp34c FLASH_BASE0_FIX_ABI1

**Static Dreamcast recompilation for PC, with a pragmatic hybrid runtime while native coverage grows.**

> v0.1 is the first official public milestone. It consolidates the long `0.0.x` development line into a single user-facing release and a single recommended Windows launcher.

## cpp34 modular runtime

cpp34 changes the build boundary, not the translated SH-4 semantics. The first run for a game creates a persistent **Guest ABI v1** module. After that, runtime and plugin changes can be rebuilt independently.

```text
DreamcastRecompiled.exe
        |
        +---- dcr_game_module.dll      <- generated SH-4; expensive; persistent
        |
        +---- DreamcastRuntime.dll     <- host runtime / device fallback layer
                  |
                  +---- plugins/dcr_network.dll   <- modem / PPP / UDP / DNS
                  +---- future dcr_pvr.dll
                  +---- future dcr_aica.dll
                  +---- plugins/dcr_gdrom.dll   <- DiscMap / TOC / sessions / bulk sectors
                  +---- future dcr_maple.dll
```

The guest cache is stored by default under `%LOCALAPPDATA%\DreamcastRecompiled\games`, so extracting a newer DreamcastRecompiled build no longer throws away the expensive game compilation. The ABI is deliberately frozen: new device state should live inside a plugin instead of changing the guest-facing `DCRuntime` layout.

Current cpp34c migration status: the **active network path is externalized** to `dcr_network.dll`; the frozen cpp33c network implementation remains only as a compatibility fallback inside the runtime. PVR/AICA/GD-ROM/Maple still use the proven built-in implementation but already have reserved plugin kinds in ABI v1. They can be moved one at a time without recompiling the game module.

Console-wide persistent state is also independent from the ZIP now:

```text
%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_flash.bin
%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_vmu_a1.bin
```

That means changing DreamcastRecompiled folders no longer creates a fresh Dreamcast ID or VMU. `DCR_SYSTEM_DATA_ROOT` can override the location; `DCR_IMPORT_SYSTEM_DIR` can point once to an older cpp33 folder to import its existing flash/VMU.

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
run_game.bat "C:\Games\game.cdi" --renderer=dx11
run_game.bat "C:\Games\game.cdi" --renderer=software
run_game.bat "C:\Games\game.cdi" --no-audio
run_game.bat "C:\Games\game.cdi" --clean
run_game.bat "C:\Games\game.cdi" --rebuild-network
run_game.bat "C:\Games\game.cdi" --rebuild-runtime
```

| Flag | Use |
|---|---|
| `--perf` | Low-overhead performance profile. |
| `--debug` | Extra diagnostics and 500 ms heartbeat. |
| `--profile` | Detailed PVR profiling; slower by design. |
| `--renderer=dx11` | Force the current Direct3D 11 GPU PVR backend. |
| `--renderer=software` | Force the software PVR path. |
| `--gpu` | Alias for `--renderer=dx11`. |
| `--software` | Legacy alias for `--renderer=software`. |
| `--no-audio` | Disable host audio output while keeping ARM7/AICA execution active. |
| `--reuse` / `--no-recompile` | Require an existing Guest ABI v1 module; never regenerate SH-4. |
| `--recompile-game` | Force SH-4 regeneration/recompile. Normally needed only for ABI/codegen changes. |
| `--rebuild-runtime` | Rebuild `DreamcastRuntime.dll` + plugins without recompiling SH-4. |
| `--rebuild-network` | Rebuild only `dcr_network.dll`; fastest network development loop. |
| `--clean` | Remove the detected title's generated/build cache. |
| `--help` | Show the launcher help. |



### Per-game compatibility profiles (cpp23)

The canonical launcher now creates an isolated workspace for every detected disc title:

```text
games\<TITLE>\game.cfg
games\<TITLE>\generated\
games\<TITLE>\build\
```

The CFG is generated automatically from `dc_disc_probe` metadata the first time a title is seen. Known games receive compatibility defaults for boot staging, renderer, seeds and entry path; unknown games receive a `GENERIC` profile that can be edited without changing source code.

This also prevents cross-game cache contamination: `--reuse` reuses only the currently detected title, and `--clean` cleans only that title.

Boot staging is now explicit in the known profiles:

- ChuChu Rocket: `plain`
- Crazy Taxi 2: `scrambled`
- Daytona USA / `PIZZICATO POLKA`: `plain`
- Record of Lodoss War: `plain`
- Segagaga / SGGG: `scrambled`

See `docs/CPP23_GAME_PROFILE_REGRESSION_FIX.md` for the regression audit and profile format.

### Renderer selection

`run_game.bat` now treats the renderer as a named backend instead of a simple GPU on/off switch:

```bat
run_game.bat "C:\Games\game.cdi" --renderer=dx11
run_game.bat "C:\Games\game.cdi" --renderer=software
run_game.bat "C:\Games\game.cdi" --renderer=auto
```

`auto` preserves the compatibility defaults used by the current title profiles: Generic/CT2 select DX11, while Daytona/Lodoss keep their established software path unless DX11 is explicitly requested. `d3d11`, `direct3d11` and `directx11` are accepted aliases for `dx11`.

The launcher namespace still recognizes the reserved renderer names, but the active graphics roadmap after cpp20 is **DX11 native-path validation first, then Vulkan**. DX9/DX10/DX12 are not active implementation targets.

### Native GPU PVR path (cpp20)

The DX11 backend now performs substantially more work directly on the host GPU: supported Dreamcast textures can be decoded by compute shader, the TA vertex arena uploads without a per-vertex conversion copy, VB/IB streaming uses a persistent three-buffer ring, compatible render-to-texture results remain GPU-resident with lazy VRAM synchronization, and PVR state bundles use a pipeline cache. The PVR frontend now targets a `PVRRendererBackend` interface so Vulkan can be added later without duplicating Dreamcast TA/PVR semantics.

Use `pvr-gnative=` in the heartbeat to measure compute texture decode, avoided CPU decode, pipeline-cache hits/misses, GPU RTT commits/lazy readbacks and true fallback materializations. See `docs/PVR_NATIVE_GPU_CPP20.md` for the field order and Windows validation checklist.

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

## Compatibility snapshot — v0.1.1

These are project validation checkpoints, not a promise of complete compatibility across every region/revision of a game.

| Title | Boot | Menus | Graphics | Audio | Controls | Gameplay | Notes |
|---|---:|---:|---:|---:|---:|---:|---|
| **ChuChu Rocket!** | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | Primary long-running 2D/commercial regression target; real gameplay has been reached in development. |
| **Crazy Taxi 2** | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | v0.1.1 restores the commercial bootstrap/allocator/closure path and is validated into real 3D gameplay; long-session performance remains under active tuning. |
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
run_game.bat    Canonical v0.1.1 Windows game launcher
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
DreamcastRecomp_v0.1.1_session_YYYYMMDD-HHMMSS.log
```

When reporting a failure, include:

- exact game/region/revision if known;
- launcher command used;
- the complete session log;
- the last visible screen/state;
- whether `--perf`, `--debug`, `--profile` or a non-default `--renderer` was active;
- CPU/GPU and Windows version for performance/rendering bugs.

Do **not** upload copyrighted CDI/BIOS files to public bug reports.

## Credits and technical references

DreamcastRecomp is an independent project. Its implementation and debugging benefited heavily from public Dreamcast documentation, open-source hardware models, recompilation prior art and homebrew ecosystems, especially:

- **[dcrecomp](https://github.com/sp00nznet/dcrecomp)** by **sp00nznet** — important prior art and inspiration for expanding DreamcastRecomp's SH-4 analysis/recompilation system and pushing SH-4 coverage toward a more complete implementation. dcrecomp is a separate MIT-licensed project and remains the work of its own authors.
- **[Flycast](https://github.com/flyinghead/flycast)** — major reference for Dreamcast hardware behavior and parity audits.
- **[KallistiOS](https://github.com/KallistiOS/KallistiOS)** — Dreamcast homebrew OS, headers, examples and the compatibility corpus used throughout development.
- **Marcus Comstedt's Dreamcast technical documentation** — low-level hardware documentation: https://mc.pp.se/dc/
- **Dreamcast Wiki / community documentation** — hardware and software-development reference material: https://dreamcast.wiki/

These references and inspirations do not imply code ownership, endorsement, affiliation, or relicensing. Third-party projects retain their own copyright and license terms. See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## License

DreamcastRecomp's original project code is released under the **MIT License**. See [`LICENSE`](LICENSE).

```text
Copyright (c) 2026 Duw0ng
```

The MIT license applies to code and documentation authored for this repository unless a file explicitly states otherwise. Third-party projects and commercial software remain under their own copyright and license terms. The project license does not grant rights to Sega firmware, Dreamcast BIOS/flash data, commercial games, game assets, extracted executables or other copyrighted third-party material.

## Legal

DreamcastRecomp does not include Sega firmware, commercial games, or copyrighted game assets. Users are responsible for supplying any software/firmware they are legally entitled to use and for complying with applicable law.

Dreamcast is a trademark of SEGA. DreamcastRecomp is an independent project and is not affiliated with, authorized by, sponsored by, or endorsed by SEGA.

## Release

**DreamcastRecomp v1.2 Stable**  
Internal version: `0.1.1`  
Development lineage: `0.0.210` universal runtime + recovered CT2 compatibility work from `0.0.171–0.0.203`

See [`RELEASE_NOTES_V0.1.1.md`](RELEASE_NOTES_V0.1.1.md), [`docs/release/VALIDATION_V0.1.1_CT2.md`](docs/release/VALIDATION_V0.1.1_CT2.md) and [`CHANGELOG.md`](CHANGELOG.md) for details.

### cpp22 fast runtime hotpatch / cached runs

If a commercial game has already been generated and compiled, cpp22 can patch only the host runtime instead of rebuilding all recompiled SH-4 shards:

```bat
apply_cpp22_texture_cache_hotfix.bat
run_game.bat "C:\path\to\game.cdi" --reuse --renderer=dx11
```

Keep `generated\official_v0.1.1` and `_cb_v0.1.1`; `--reuse` intentionally refuses a different CDI than the one recorded in the cached `disc.map`.


## cpp34c retail flash base-block fix

The cpp34b network test exposed a pre-modem flash lookup loop: ChuChu Rocket repeatedly read partition 2's bitmap and physical block `0x1C040` without ever touching the modem. cpp34c seeds the CRC-valid logical block `0x0000` that retail Dreamcast flash images place in that first user block, and migrates existing cpp34b persistent flash images in place. Guest ABI remains v1, so use `--rebuild-runtime`; do not recompile the game module.

## cpp34b full ISP compatibility

cpp34b keeps Guest ABI v1 from cpp34a and expands the persistent Dreamcast flash profile to cover all documented dial-up layouts used by retail software:

- PlanetWeb settings `0x80-0x84` (including the previously missing `0x81`).
- PlanetWeb PPP/email/proxy blocks `0xC0-0xC5`.
- DreamKey PPP/DNS blocks `0xC6-0xC8`.
- Generic Katana ISP blocks `0xE0`, `0xE2`, `0xE4-0xE9`, `0xEB`.
- Automatic migration preserves the 64-bit console ID while rebuilding only user partition 2.
- Flash hot-loop diagnostics print the BIOS caller at 10k/100k/1M INFO calls and 40k/400k/4M READ calls.

The game module ABI is unchanged: after the initial cpp34 migration, network-only rebuilds still compile only `dcr_network.dll`.


## cpp34q — Late-AOT overlay para targets SH-4 descubiertos en runtime

Los targets indirectos que solo aparecen al avanzar por rutas nuevas (por ejemplo, online real) ya no obligan a regenerar el guest completo. `PATCH_CPP34Q_CHUCHU_LATE_TARGET.bat` toma el `BOOTSTRAP.BIN` ya cacheado, recompila únicamente el entrypoint faltante y lo instala como `dcr_guest_patch.dll` ABI1 en el directorio de plugins del juego.

Para el target descubierto después de la respuesta real del servidor de ChuChu:

```bat
TEST_CPP34Q_LATE_AOT_ONLINE.bat "C:\ruta\ccr.cdi"
```

Los targets adicionales quedan acumulados en `late_aot\late_target_seeds.txt` dentro del cache de ese juego.


## cpp34r — Network Fast Path + Submit Trace

cpp34r keeps the cpp34q Late-AOT overlay and the cpp34p online stack, but removes hot-path overhead from modem polling: TLS plugin/state caches, 1 ms host socket polling, 50 ms full diagnostic mirroring, bounded serial catch-up, and connected-tick early returns. Pressing START while connected arms a short `[SUBMIT TRACE]` window with guest PC and network/save counters. Use `TEST_CPP34R_NETWORK_FASTPATH_SUBMIT.bat`; it reuses the existing ABI1 guest and existing `0x8C036102` overlay whenever present.

## cpp36b1c — MSVC Maple Input Fix

- Fixes MSVC 14.51 `std::clamp` ambiguity between Win32 `LONG` mouse coordinates and `int` limits.
- Guards Windows preprocessor macros to avoid redefinition warnings.
- Guest ABI1 and the adopted portable-safe ChuChu guest remain unchanged.
- This update rebuilds only Runtime/plugins; it does not recompile SH-4.