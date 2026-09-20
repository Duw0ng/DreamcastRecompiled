@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - System/Control
echo ========================================
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\system_control rmdir /s /q generated\system_control
if exist generated\system_control-build rmdir /s /q generated\system_control-build

build\Release\dc_recomp.exe samples\sh4_system_control.elf --function _main --output generated\system_control
if errorlevel 1 exit /b 1
cmake -S generated\system_control -B generated\system_control-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\system_control-build --config Release
if errorlevel 1 exit /b 1

generated\system_control-build\Release\dreamcast_program.exe > generated\system_control\native_output.txt
set RC=%ERRORLEVEL%
type generated\system_control\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=42" generated\system_control\native_output.txt >nul || exit /b 1
findstr /C:"GBR=0x2A" generated\system_control\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\system_control\native_output.txt >nul || exit /b 1

echo.
echo [OK] System/control, PREF normal, MOVCA.L y stack-reg transfers: PASS.
exit /b 0
