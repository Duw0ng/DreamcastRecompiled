@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\juego.cdi"
  echo.
  echo Este test usa --reuse: si no encuentra un Guest ABI1 valido, SALE CON ERROR.
  echo Nunca inicia una recompilacion SH-4 desde cero por su cuenta.
  exit /b 2
)
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
