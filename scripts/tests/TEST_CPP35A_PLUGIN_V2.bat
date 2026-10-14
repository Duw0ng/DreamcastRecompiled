@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: TEST_CPP35A_PLUGIN_V2.bat "ruta\juego.cdi"
  exit /b 1
)
echo ============================================================
echo  DreamcastRecomp cpp35a - Plugin Runtime V2 / Maple
 echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~1" --rebuild-maple
exit /b %errorlevel%
