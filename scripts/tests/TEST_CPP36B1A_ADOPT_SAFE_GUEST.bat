@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\ccr.cdi"
  exit /b 1
)
echo ============================================================
echo  cpp36b1a - ADOPT CHUCHU PORTABLE SAFE GUEST

echo  Este test NO permite recompilar el guest: usa --reuse.
echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
