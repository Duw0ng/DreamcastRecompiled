@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\juego.cdi"
  exit /b 2
)
echo ============================================================
echo  cpp36b PURUPURU + MODERN MAPLE - NO GUEST REBUILD
 echo  Maple trace enabled for validation
 echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --rebuild-maple --maple-trace
exit /b %ERRORLEVEL%
