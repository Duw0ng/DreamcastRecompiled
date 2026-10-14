@echo off
setlocal
cd /d "%~dp0..\.."
echo ============================================================
echo  DreamcastRecomp v1.2 cpp33c PLANETWEB_DUAL_ISP - Regression tests
echo ============================================================
where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake no esta disponible en PATH.
  exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 exit /b 2
cmake --build build --config Release --parallel 1
if errorlevel 1 exit /b 3
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b %ERRORLEVEL%
rem interp-fallback-v1: standalone runtime tests (SH-4 interpreter, IP.BIN staging).
cmake -S runtime -B build_runtime_tests -A x64
if errorlevel 1 exit /b 4
cmake --build build_runtime_tests --config Release --target DreamcastRuntime dcr_sh4_interp_tests dcr_portable_ipbin_stage_tests --parallel 1
if errorlevel 1 exit /b 5
ctest --test-dir build_runtime_tests -C Release --output-on-failure
exit /b %ERRORLEVEL%
