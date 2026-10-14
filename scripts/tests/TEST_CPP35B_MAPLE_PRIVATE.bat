@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: TEST_CPP35B_MAPLE_PRIVATE.bat "ruta\juego.cdi"
  exit /b 1
)
echo ============================================================
echo  DreamcastRecomp cpp35b1 - Maple + CACHE ABI FREEZE
echo ============================================================
echo [SAFE] --reuse activo: este test NO puede iniciar una recompilacion SH-4 completa.
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %errorlevel%
