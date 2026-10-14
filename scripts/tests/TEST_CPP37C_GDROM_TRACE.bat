@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --gdrom-trace
exit /b %ERRORLEVEL%
