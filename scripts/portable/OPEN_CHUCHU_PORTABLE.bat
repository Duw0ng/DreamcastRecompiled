@echo off
setlocal
set "P=%LOCALAPPDATA%\DreamcastRecompiled\games\CHUCHU_ROCKET\portable\DreamcastRecompiled.exe"
if not exist "%P%" (
  echo [ERROR] Aun no existe la portable auto-sincronizada.
  echo [INFO] Ejecuta primero run_game.bat "ccr.cdi" --reuse
  exit /b 2
)
start "" "%P%"
