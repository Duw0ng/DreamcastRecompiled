@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo ========================================
echo  DreamcastRecomp 0.0.170 - KOS Memtest Suite
echo ========================================
echo.

call run_kos_memtest_databus.bat
if errorlevel 1 goto :error
call run_kos_memtest_addressbus.bat
if errorlevel 1 goto :error
call run_kos_memtest_device.bat
if errorlevel 1 goto :error

echo.
echo [OK] Las 3 rutinas centrales de memtest32 pasaron nativamente.
exit /b 0

:error
echo.
echo [ERROR] Fallo alguna rutina KallistiOS de memtest32.
exit /b 1
