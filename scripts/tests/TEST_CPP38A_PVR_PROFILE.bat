@echo off
setlocal
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\game.cdi"
  exit /b 2
)
rem Diagnostic run. --perf adds profiling overhead; use TEST_CPP38A_PVR_FAST for FPS comparison.
call "%~dp0..\windows\run_game.bat" "%~1" --reuse --perf
exit /b %ERRORLEVEL%
