# DreamcastRecomp 0.0.128 — validation

## Scope
- Normal commercial launcher: one visible gameplay/PVR window (`--host-window` removed).
- Diagnostic perf/profile launchers retain `--host-window`.
- Generated MSVC Release target enables `/MP`, bounded by `CL_MPCount`.
- `build_progress.ps1` defaults `CompileJobs=2`, keeps CMake/MSBuild parallel level at 1, disables node reuse, and requests BelowNormal priority.
- No guest/runtime PVR/AICA/SH-4 logic changes from 0.0.127.

## Local validation
- Main Linux Release build: PASS.
- CTest: **47/47 PASS**.
- Fresh `dc_recomp_cpu_control_output` configure/build: PASS.
- `generated_compile_test`: RC=0.
- Fresh generated `dreamcast_program`: RC=0, R0=42.
- Emitted CMake verified to contain Release `/MP`; runtime cap remains `CL_MPCount=CompileJobs`.
- Normal launcher verified with zero `--host-window` occurrences; perf/profile retain it.
- Windows-specific `/MP` resource behavior requires live MSVC acceptance; default cap is intentionally 2.
