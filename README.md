# DreamcastRecompiled

**V1.2 Stable Release — Native SH-4 recompilation for Windows x64**

[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE) ![Windows](https://img.shields.io/badge/Windows-x64-blue) ![Compatibility](https://img.shields.io/badge/Compatibility-in%20development-orange)

**[Portable game catalog](game-portables/README.md) · [Releases](https://github.com/Duw0ng/DreamcastRecompiled/releases) · [Español](docs/README.es.md) · [ChuChu Rocket! tester guide](docs/CHUCHU_ROCKET_TESTER_V1.2.md) · [Report issues](https://github.com/Duw0ng/DreamcastRecompiled/issues)**

> **Release status:** V1.2 is the stable baseline for validated paths, **not** a claim of universal Dreamcast compatibility. The public V1.2 GitHub release and downloadable asset have not yet been published.

## What is DreamcastRecompiled?

DreamcastRecompiled explores running Sega Dreamcast programs on modern PCs by discovering SH-4 code, recompiling supported paths into native C++, and executing them with a host-side Dreamcast hardware runtime.

It is a **recompiler-first hybrid runtime**: native SH-4 code, dynamic target recovery/interpreter fallback, hardware modeling and HLE coexist. This is not 100% native execution or a drop-in replacement for a mature Dreamcast emulator.

```text
User-provided Dreamcast disc image
  -> disc identification / boot preparation
  -> SH-4 discovery, native C++ / reusable game module
  -> PC host runtime
  -> PVR / AICA / Maple + VMU / GD-ROM / Holly
  -> game output
```

## V1.2 highlights

- **Aqua graphical launcher:** library, settings, controllers, diagnostics, and multiple interface themes.
- **Español / English:** configurable bilingual UI.
- **Portable Windows x64 launcher:** bundled Python 3.12 / Tk dependencies; no separately installed Python needed for the GUI.
- **Native SH-4 game-module reuse:** supported precompiled game code can run without recompiling every launch; incremental generation and interpreter fallback remain available.
- **Standalone VMU / AquaPet:** independent VMU window, foreground recovery and always-on-top handling. Includes the `ctypes.wintypes` packaging fix tested by the project maintainer.
- **Controller testing:** XInput and native DirectInput/WinMM support, mapping and calibration tools.
- **Host hardware runtime:** PVR rendering, AICA audio, GD-ROM access, Maple and VMU storage, diagnostics, and selected modular plugins.
- **Networking experiments:** DCNet / modem paths are under development, not guaranteed online compatibility.

## ChuChu Rocket! — primary tester game

**Direct portable download:** [ChuChu Rocket! V1.2 — 11-file, 13.6 MiB standalone ZIP](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-chuchu-rocket-direct-v1.2). This lightweight EXE + game-module package starts directly and is distinct from the full Aqua launcher distribution.


**The V1.2 Windows tester build contains an SH-4 native game module compiled from ChuChu Rocket! code.** On the validated disc path the launcher checks the disc title and the bundle integrity and runs the native module without rebuilding SH-4 locally. This lets testers exercise the recompilation pipeline and measure performance.

**The compiled module alone is NOT a playable game.** Testers must supply their **own compatible original CDI disc image**. The game still loads boot data and disc assets through the GD-ROM path; no game image is included.

| Disc field | Validated identification |
|---|---|
| Title | ChuChu Rocket! |
| Product / revision | `MK-51049`, `V1.007` |
| Region flags | `JUE` — Japan / USA / Europe |
| Volume ID | `CHU_CHU_ROCKET_KAL` |
| Disc boot executable | `1ST_READ.BIN` |
| Recorded disc date | `2000-02-17` |

These values come from the disc probe of the project's reference copy; a different revision or rebuilt CDI may not be compatible. The automatic selection checks title/volume; it does not enforce an executable hash or exact revision. Use the reference revision below. See **[exact tester steps and limitations](docs/CHUCHU_ROCKET_TESTER_V1.2.md)**.

> **Tester distribution model:** The V1.2 archive includes precompiled SH-4 code for the validated ChuChu Rocket! build but **does not contain a retail CDI or game assets**. A user-supplied compatible game image is required to start. The project MIT license covers original project code, not third-party commercial program rights; no ownership or rights over ChuChu Rocket! are claimed. Distribution of compiled game-derived code remains subject to applicable rights and law.

## Getting started on Windows

1. Obtain an authorized Windows x64 release/tester archive and extract it completely.
2. Launch `DreamcastRecompiled.exe` (or `ABRIR_LAUNCHER.bat`). Keep `launcher/` and `_internal/` beside the executable.
3. In **Games / Juegos**, select your own supported CDI disc image.
4. For the validated ChuChu Rocket! native path, select the compatible `MK-51049 V1.007` disc. Its prepared SH-4 module avoids a new native compilation; the **CDI is still required**.
5. Configure controller/audio/video options and launch. Capture a session log if the game fails.

No Python installation is required for the packaged Aqua launcher. **Other games may require Visual Studio C++ Build Tools, CMake, and Windows SDK** for native code generation. Do not assume all games use the ChuChu fast path.

**Standalone VMU:** open it from Aqua or with `ABRIR_LAUNCHER.bat --vmu`. Use **Open / Show VMU** if it is hidden behind another window.

**Advanced developer runner:** `scripts\windows\run_game.bat "C:\Games\example.cdi"`.

**Additional portable testers:** [Sega Tetris](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sega-tetris-direct-v1.2) (gameplay observed; occasional crash) and [Segagaga](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-sggg-direct-v1.2) (experimental; known audio issues). See the [multi-game release guide](game-portables/MULTI_GAME_PORTABLES.md).

## Compatibility snapshot

| Title | Tested progress | Notes |
|---|---|---|
| **ChuChu Rocket!** | ✅ Offline gameplay | Main SH-4 native test; online experimental; matching CDI needed. |
| **Sega Tetris** (Japan, community-patched build) | ✅ Gameplay observed | Graphics/audio worked in a test; intermittent crash reported. |
| **Mr. Driller** | 🟡 Gameplay observed | Significant menu graphics issues, plus audio defects. |
| **Segagaga (SGGG)** | 🟡 Partial | Audio and compatibility issues remain. |
| **Daytona USA 2001** | 🟡 Partial | Menus / demo path observed, not proven fully playable. |
| **Crazy Taxi 2** | 🟡 Partial | Startup/menus reached; gameplay issues. |

✅ = gameplay observed in a reported test; 🟡 = partial / unstable. These are not guarantees for all regions, configurations, or future versions.

## Known limitations and next steps

Native recompilation coverage is incomplete; some titles crash, fail to boot, or show graphics and audio defects. Input mapping and VMU behavior can differ across Windows hardware. Online/DCNet is experimental. Playable Linux, Vulkan, and OpenGL implementations are not part of this V1.2 release.

Our priorities are broader SH-4 coverage, improved PVR rendering and AICA audio, performance stability, controller reliability, and repeatable title-by-title compatibility tests.

## Bug reports

Please [open an issue](https://github.com/Duw0ng/DreamcastRecompiled/issues) with the game title, region/revision, steps to reproduce, Windows version, CPU/GPU, controller, and a complete runtime/launcher log. **Do not attach commercial `.cdi`, firmware, extracted executables or game-derived DLLs.**

## Source checkout versus Windows release

The repository contains project source and launcher assets. The Windows release archive additionally includes the Python/Tk host and the prepared ChuChu Rocket! module. Download the packaged release for compiler-free testing; GitHub source archives do not contain those bundled binaries.

## Source and credits

Original project code is licensed under [MIT](LICENSE). Third-party code, documentation and assets retain their respective licenses. Relevant independent projects and research include [dcrecomp](https://github.com/sp00nznet/dcrecomp), [Flycast](https://github.com/flyinghead/flycast), [KallistiOS](https://github.com/KallistiOS/KallistiOS), and [Dreamcast technical documentation](https://mc.pp.se/dc/). Their mention does not imply affiliation or endorsement.

DreamcastRecompiled is independent of SEGA. Dreamcast and commercial titles belong to their respective owners. The MIT license for this repository does not license Sega firmware, commercial games, or derivatives of proprietary executable code.

**Recompiling the Dreamcast, one working title at a time.**
