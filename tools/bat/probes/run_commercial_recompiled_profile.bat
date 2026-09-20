@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
echo [COMPAT] run_commercial_recompiled_profile.bat es un alias historico.
echo [COMPAT] Interfaz recomendada: run_game.bat "juego.cdi" [--perf] [--debug]
call run_game.bat %* --profile
exit /b %ERRORLEVEL%
