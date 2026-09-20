# DreamcastRecomp 0.0.129 validation

- Runtime/PVR/AICA code paths remain based on 0.0.128; this revision changes build/launcher behavior only.
- `run_commercial_recompile.bat` defaults `DCR_BUILD_JOBS` to 1.
- Generated CMake defaults `DCR_MSVC_MP_COUNT=1` and emits `/MP${DCR_MSVC_MP_COUNT}`; the launcher passes `-DDCR_MSVC_MP_COUNT=N`, so Windows gets `/MP1` by default and never bare `/MP`.
- `tools/build_progress.ps1` defaults `CompileJobs` to 1; MSBuild remains one node and BelowNormal.
- `tools/disc_signature.ps1` fingerprints the selected CDI using normalized full path, size and UTC modification ticks.
- Cache hit requires matching signature plus existing `dreamcast_program.exe` and `disc.map`; `DCR_FORCE_RECOMPILE=1` / `DCR_CLEAN_RECOMPILE=1` bypass it.
- Linux CMake Release regression: 47/47 PASS.
- Fresh generated `generated_compile_test`: RC=0.
- Fresh generated `dreamcast_program`: RC=0, R0=42.
- Windows BAT cache and MSVC resource behavior require live acceptance.
