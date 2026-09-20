# DreamcastRecomp v0.1 Official — Validation

Validation date: 2026-09-20

## Release identity

- Public release: **DreamcastRecomp v0.1 Official**
- Semantic/internal version: **0.1.1**
- Development lineage: **0.0.210**
- Canonical Windows launcher: `run_game.bat`

## Source/build validation

The v0.1 source tree was configured and built from scratch in the Linux validation container with CMake in Release mode. The complete source build reached 100%, including `dc_disc_probe`, `dc_boot_prepare`, `dc_raw_recomp`, core libraries, unit tests and generated-program test targets.

Runtime identity checks:

```text
dc_disc_probe 0.1.1
dc_raw_recomp 0.1.1
```

`CMakeLists.txt` also declares project version `0.1.1`.

## Automated regression suite

Final CTest result after the v0.1.1 controller changes:

```text
100% tests passed, 0 tests failed out of 53
Total Test time (real) = 18.71 sec
```

This includes SH-4 decoding/analysis, CFG/DCIR, generated code, memory/address-bus behavior, CPU control, FPU operations, system control, PVR decoding/guards and generated-program build tests.

### v0.1.1 controller regression coverage

`cpp_emitter_tests` now checks that generated runtimes contain the backend-neutral `logical:*` mappings, DirectInput/WinMM joystick polling, PS4 backend selection, POV/D-pad handling, native DS4 Cross mapping, and WinMM device-slot scanning. The generated single-program and generated multi-program build tests also pass after these changes.

The runtime default input path now shares the same controller-profile pipeline even when no profile file exists. In `auto` mode it attempts XInput first, then DirectInput/WinMM. Native DS4 mappings cover Cross/Circle/Square/Triangle, Options, L1/R1, POV D-pad, both sticks and L2/R2 fallback.

## Launcher validation

The public Windows interface has been normalized around:

```bat
run_game.bat "juego.cdi" [--perf] [--debug] [--profile] [--software] [--no-audio] [--clean]
```

Maintained historical game launchers now forward to `run_game.bat` so older instructions continue to work. Development-only probes/tests remain separate because many accept ELF files, generated projects or specialized diagnostic inputs rather than CDI images.

The launcher preserves the maintained per-title profiles for ChuChu Rocket!, Crazy Taxi 2, Daytona USA 2001 and Record of Lodoss War, including their seed files and timing/render/audio defaults.

The Linux validation container does not provide Windows `cmd.exe`, Visual Studio/MSVC, WinMM, a physical DualShock 4, or Direct3D 11. Therefore the `.bat` control flow and Windows-only controller branch were statically reviewed here, while final physical DS4/Windows acceptance remains a platform test. The cross-platform code generator and generated-program regression suite pass completely.

## Release-package layout validation

The official ZIP was reorganized after the code/regression validation so the repository root is user-facing instead of development-history-facing:

- root reduced from **185 loose files to 12**;
- compatibility screenshots moved to `test-img/`;
- historical 0.0.x validation reports and raw validation artifacts moved under `docs/archive/`;
- historical patches and seed snapshots grouped under `patches/archive/` and `profiles/archive/`;
- 95 internal Windows BAT files grouped under `tools/bat/{tests,homebrew,probes,legacy}`;
- a simple root `run_tests.bat` was added for the normal CTest regression path.

Every relocated BAT was statically checked to resolve the repository root before using its historical relative paths. After the layout cleanup, the Linux source build again reached 100%. Tests 1–52 passed in the post-layout run; test 53 (`generated_program_build`) was then rerun separately after the outer command timeout and passed. Taken together, the post-layout package remains **53/53 PASS**.

## Commercial-image scope

No copyrighted commercial game image is included in this package. The automated v0.1 revalidation covers the source/runtime/code-generation regression suite. Compatibility status in the README carries forward the project's documented live tests from the 0.0.x development lineage; it is not a claim that every title was rerun from its CDI inside this Linux validation container.

## Result

**DreamcastRecomp v0.1 Official is source/build/regression validated at 53/53 tests with active version identity 0.1.1. Native PS4/DirectInput behavior is implemented and requires the final physical Windows controller acceptance test.**
