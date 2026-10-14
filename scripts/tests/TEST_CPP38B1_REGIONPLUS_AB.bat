@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem Controlled A/B: only the runtime FPU mode changes. No guest recompilation.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --fpu-regionplus
exit /b %ERRORLEVEL%
