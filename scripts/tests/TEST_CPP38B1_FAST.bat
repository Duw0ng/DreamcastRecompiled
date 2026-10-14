@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem cpp38b1 recommended path: historical FPU superblock, metrics OFF,
rem 262144-cycle host sync, cpp38a index scratch + cpp38a PVR fast paths.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --fpu-superblock
exit /b %ERRORLEVEL%
