@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: TEST_CPP38E_FAST.bat "juego.cdi" [opciones]
  exit /b 1
)
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --spg-ff-on --hotpc-off --hot-recovery-on --no-portable-sync --no-pause %2 %3 %4 %5 %6 %7 %8 %9
exit /b %errorlevel%
