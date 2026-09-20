@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
call run_kos_aica_sfx_hle.bat %*
if errorlevel 1 exit /b %ERRORLEVEL%
call run_kos_sfx_full_probe.bat %*
if errorlevel 1 exit /b %ERRORLEVEL%
echo.
echo [OK] Audio suite 0.0.170 completada.
exit /b 0
