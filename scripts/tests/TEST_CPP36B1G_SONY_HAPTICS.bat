@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\ccr.cdi"
  exit /b 1
)
echo ============================================================
echo  cpp36b1g SONY HAPTICS + DREAMCAST VISUAL V2
echo  Guest ABI1 SAFE: REUSE / NO SH-4 REBUILD
echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --maple-trace
exit /b %ERRORLEVEL%
