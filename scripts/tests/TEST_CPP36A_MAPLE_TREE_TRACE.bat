@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\juego.cdi"
  exit /b 2
)
echo ============================================================
echo  cpp36a MAPLE DEVICE TREE - TRACE / NO GUEST REBUILD
echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --rebuild-maple --maple-trace
exit /b %ERRORLEVEL%
