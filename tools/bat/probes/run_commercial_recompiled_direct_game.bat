@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_direct_game.bat "ruta\juego.cdi"
  echo.
  echo Modo experimental 0.0.205: conserva la extraccion y closure comercial,
  echo pero salta el bootstrap IP.BIN en runtime y entra directamente a 0x8C010000.
  echo Util para imagenes con bootstrap/selfboot modificado que vuelve a transformar
  echo un 1ST_READ.BIN que ya es ejecutable.
  exit /b 1
)
set "DCR_DIRECT_GAME_ENTRY=0x8C010000"
call run_commercial_recompiled.bat "%~1"
exit /b %ERRORLEVEL%
