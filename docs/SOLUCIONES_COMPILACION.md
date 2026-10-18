# Compilation fixes based on 0.0.203_rebased193

## Compile and test ChuChu Rocket! on Windows

1. Extract this ZIP into a new folder with a short path, such as `C:\DCR203fix`. Do not copy `build` or `generated` folders from an earlier release.
2. Install Visual Studio 2022 / Build Tools with **Desktop development with C++**, Windows SDK, and CMake. Run from Developer PowerShell or the x64 Native Tools prompt if CMake is not on PATH.
3. In PowerShell, navigate to the directory containing the batch file:

```powershell
.\compilar_y_probar_chuchu.bat "C:\Games\ChuChu Rocket.cdi"
```

Alternatively, drag the CDI image onto `compilar_y_probar_chuchu.bat`.

The batch file builds the tools, runs 53 tests (including actual compilation and linking of generated C++), extracts the user-supplied CDI, regenerates the game, builds Release x64, and opens the game window. The commercial build is limited to one parallel job. New tests may require several minutes on the first Windows run.

For another run with incremental compilation:

```powershell
.\run_commercial_recompiled.bat "C:\Games\ChuChu Rocket.cdi"
```

Click the PVR window to focus it. Controls: Enter = Start; arrow keys = D-pad; Z/Space/J = A; X/K = B; C/U = X; V/I = Y.

Session logs are written to `logs`. If compilation fails, capture the **first compiler error with surrounding lines**; later MSBuild errors are often consequences. Manually closing the game may return exit code 130.

## Corrected errors

- **Generated runtime:** `dc_read64_fmov_hot` called `dc_guest_read32_hot`, a helper defined only in the program source. Compiling `dc_runtime.cpp` therefore failed with an undeclared-identifier error. Its two fallback reads now call runtime-owned `dc_read32_hot`. The fix is in `src/codegen/cpp_emitter.cpp` so it survives regeneration; patching temporary generated C++ alone is insufficient.
- **Single-function output:** called `dc_guest_*` helpers without emitting their definitions. Both emitter modes now share the same helper definitions, and individual output includes `<cstring>` for `std::memcpy`.
- **Legacy build tools:** the commercial batch script formerly built tools only when `dc_disc_probe.exe` was missing. It now configures and incrementally builds `dc_disc_probe`, `dc_boot_prepare`, and `dc_raw_recomp` on every invocation, checking for all three binaries before continuing.
- **PowerShell diagnostics:** the progress script preserves native stderr, uses CMake's exit code, and restores the environment in `finally`. This particular change was reviewed statically but not executed under PowerShell during the original test.
- **Insufficient build tests:** new `generated_single_build` and `generated_program_build` tests configure, compile, link, and execute the generated runtime's actual smoke test. Earlier validation focused mainly on emitted text.

`dc_runtime.cpp` was already part of the CMake library, and D3D11 libraries were already linked. Adding them again cannot fix missing identifiers. The exact patch is provided as `COMPILATION_FIX.patch`.

## Scope

The original 0.0.203 baseline, game options, and experimental CT2 folder are retained. This fix does not apply experimental CT2-specific modes to ChuChu. No ChuChu CDI is distributed in the ZIP.

The original local validation used GCC/Linux. It does **not** establish that MSVC, Windows batch files, Direct3D, Windows audio, or a real ChuChu session was successfully tested. Hardware- and game-specific defects may remain.
