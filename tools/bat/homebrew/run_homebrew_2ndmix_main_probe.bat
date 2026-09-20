@echo off
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
rem Compatibility wrapper: 0.0.27 crossed the old PVR_RESET boundary, so the
rem former CPU-only main probe is now the minimal PVR graphics probe.
call run_homebrew_2ndmix_graphics_probe.bat %*
exit /b %ERRORLEVEL%
