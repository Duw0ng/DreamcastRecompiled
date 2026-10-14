@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem cpp38b2 recommended path: Superblock + generated FPU metrics compiled OUT,
rem 262144-cycle host sync, cpp38a/cpp38b1 index scratch + PVR fast paths.
rem First cpp38b2 run may rebuild only the existing generated C++ guest module once;
rem it does not rediscover the SH-4 closure.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --fpu-superblock
exit /b %ERRORLEVEL%
