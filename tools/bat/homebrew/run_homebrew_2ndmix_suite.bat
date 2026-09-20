@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\2ndmix.elf
if not "%~1"=="" set ELF=%~1
call run_homebrew_2ndmix_crc.bat "%ELF%"
if errorlevel 1 exit /b %ERRORLEVEL%
call run_homebrew_2ndmix_pcx.bat "%ELF%"
if errorlevel 1 exit /b %ERRORLEVEL%
call run_homebrew_2ndmix_graphics_probe.bat "%ELF%"
if errorlevel 1 exit /b %ERRORLEVEL%
echo.
echo ================================================
echo  [OK] Suite real 2ndMix DreamcastRecomp 0.0.170
echo ================================================
exit /b 0
