@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo  DreamcastRecomp v0.1 Official - Regression tests
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
exit /b %ERRORLEVEL%
