@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem Controlled A/B after the one-time cpp38b2 metric-free guest migration.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --fpu-regionplus
exit /b %ERRORLEVEL%
