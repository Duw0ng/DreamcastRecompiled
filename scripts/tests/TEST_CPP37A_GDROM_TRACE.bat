@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\juego.cdi"
  exit /b 1
)
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --gdrom-trace
exit /b %errorlevel%
