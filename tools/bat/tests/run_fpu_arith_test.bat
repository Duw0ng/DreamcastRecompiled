@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - FPU Arithmetic
echo ========================================
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\fpu_arith rmdir /s /q generated\fpu_arith
if exist generated\fpu_arith-build rmdir /s /q generated\fpu_arith-build

build\Release\dc_recomp.exe samples\sh4_fpu_arith.elf --function _main --output generated\fpu_arith
if errorlevel 1 exit /b 1
cmake -S generated\fpu_arith -B generated\fpu_arith-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\fpu_arith-build --config Release
if errorlevel 1 exit /b 1

generated\fpu_arith-build\Release\dreamcast_program.exe > generated\fpu_arith\native_output.txt
set RC=%ERRORLEVEL%
type generated\fpu_arith\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=42" generated\fpu_arith\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\fpu_arith\native_output.txt >nul || exit /b 1

echo.
echo [OK] FADD/FSUB/FMUL/FDIV/FCMP/FMAC + PR=1 double ejecutados nativamente: R0=42.
exit /b 0
