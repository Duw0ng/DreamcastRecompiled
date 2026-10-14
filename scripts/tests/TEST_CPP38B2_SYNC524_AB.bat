@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem Controlled A/B: Superblock remains active; only host sync returns to 524288.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --fpu-superblock --sync-524
exit /b %ERRORLEVEL%
