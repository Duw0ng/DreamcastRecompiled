@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo [INFO] blockssa166 fue descartado tras los logs de Mouse Mania: era mas lento que region.
echo [INFO] Este alias ejecuta Region+ 0.0.170 para no romper accesos directos antiguos.
call run_commercial_recompiled_fpu_regionplus.bat
exit /b %ERRORLEVEL%
