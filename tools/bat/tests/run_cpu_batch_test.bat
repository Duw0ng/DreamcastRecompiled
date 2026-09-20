@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - CPU Batch
echo ========================================
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\cpu_batch rmdir /s /q generated\cpu_batch
if exist generated\cpu_batch-build rmdir /s /q generated\cpu_batch-build

build\Release\dc_recomp.exe samples\sh4_cpu_batch.elf --function _main --output generated\cpu_batch
if errorlevel 1 exit /b 1
cmake -S generated\cpu_batch -B generated\cpu_batch-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\cpu_batch-build --config Release
if errorlevel 1 exit /b 1

generated\cpu_batch-build\Release\dreamcast_program.exe > generated\cpu_batch\native_output.txt
set RC=%ERRORLEVEL%
type generated\cpu_batch\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=42" generated\cpu_batch\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\cpu_batch\native_output.txt >nul || exit /b 1

echo.
echo [OK] Batch CPU 0.0.170 ejecutado nativamente: R0=42.
exit /b 0
