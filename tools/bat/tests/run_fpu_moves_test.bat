@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - FPU Moves
echo ========================================
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\fpu_moves rmdir /s /q generated\fpu_moves
if exist generated\fpu_moves-build rmdir /s /q generated\fpu_moves-build

build\Release\dc_recomp.exe samples\sh4_fpu_moves.elf --function _main --output generated\fpu_moves
if errorlevel 1 exit /b 1
cmake -S generated\fpu_moves -B generated\fpu_moves-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\fpu_moves-build --config Release
if errorlevel 1 exit /b 1

generated\fpu_moves-build\Release\dreamcast_program.exe > generated\fpu_moves\native_output.txt
set RC=%ERRORLEVEL%
type generated\fpu_moves\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=42" generated\fpu_moves\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\fpu_moves\native_output.txt >nul || exit /b 1

echo.
echo [OK] FMOV RAM/FR ejecutado nativamente: R0=42.
exit /b 0
