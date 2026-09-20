@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - RTE ordering
echo ========================================
echo.
echo SSR.T=1, SR.T=0. El delay slot MOVT debe observar el SR restaurado.
echo Resultado esperado: R0=1, SR=0x1, PC=0xFFFFFFFF.
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1
if exist generated\rte_probe rmdir /s /q generated\rte_probe
if exist generated\rte_probe-build rmdir /s /q generated\rte_probe-build

build\Release\dc_recomp.exe samples\sh4_system_control.elf --function _rte_probe --output generated\rte_probe
if errorlevel 1 exit /b 1
cmake -S generated\rte_probe -B generated\rte_probe-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\rte_probe-build --config Release
if errorlevel 1 exit /b 1

generated\rte_probe-build\Release\dreamcast_program.exe --ssr=1 --spc=0xFFFFFFFF > generated\rte_probe\native_output.txt
set RC=%ERRORLEVEL%
type generated\rte_probe\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=1" generated\rte_probe\native_output.txt >nul || exit /b 1
findstr /C:"SR=0x1" generated\rte_probe\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\rte_probe\native_output.txt >nul || exit /b 1

echo.
echo [OK] RTE restaura SR antes del delay slot y retorna via SPC correctamente.
exit /b 0
