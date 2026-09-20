@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\raytris.elf
if not "%~1"=="" set ELF=%~1
call run_homebrew_raytris_pixeldata.bat "%ELF%"
if errorlevel 1 exit /b %ERRORLEVEL%
call run_homebrew_raytris_color.bat "%ELF%"
if errorlevel 1 exit /b %ERRORLEVEL%
echo.
echo [INFO] El full-_main de Raytris queda como probe diagnostico separado en 0.0.27;
echo        los cambios de jump-table/hardware hacen que su frontera siga evolucionando.
echo ================================================
echo  [OK] Suite determinista Raytris 0.0.170 completada
echo ================================================
exit /b 0
