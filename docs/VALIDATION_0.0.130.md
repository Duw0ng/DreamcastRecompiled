# DreamcastRecomp 0.0.130 validation

- Runtime/PVR/AICA behavior remains based on 0.0.129; this revision reverts only commercial build orchestration.
- `run_commercial_recompile.bat` configures the generated CMake project without any `DCR_BUILD_JOBS` or `DCR_MSVC_MP_COUNT` setting.
- Generated MSVC CMake contains no active `/MP` compiler option.
- `tools/build_progress.ps1` enforces `CL_MPCount=1`, `CMAKE_BUILD_PARALLEL_LEVEL=1`, `MSBUILDDISABLENODEREUSE=1`, and builds with `--parallel 1`.
- Linux CMake Release regression: 47/47 PASS.
- Fresh generated C++ project configured and built with `--parallel 1`.
- Fresh `generated_compile_test`: RC=0.
