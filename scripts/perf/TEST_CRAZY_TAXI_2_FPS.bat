@echo off
setlocal EnableExtensions
rem DreamcastRecompiled v1.2 Rev 2 - Crazy Taxi 2 FPS comparison.
rem A comparable BASELINE and an explicitly instrumented PROFILE share one
rem original SH-4 guest cache and never request implicit recompilation.
if "%~1"=="" goto :usage
if not exist "%~1" (
  echo [CT2] Disc image not found: %~1
  exit /b 2
)
set "DCR_TEST_DISC=%~f1"
set "DCR_TEST_MODE=BASELINE"
set "DCR_TEST_FLAGS=--hotpc-off"
if /I "%~2"=="--profile" (
  set "DCR_TEST_MODE=PROFILE"
  set "DCR_TEST_FLAGS=--perf --hotpc"
) else if not "%~2"=="" (
  echo [CT2] Unknown second argument: %~2
  goto :usage
)
pushd "%~dp0"
echo.
echo ===============================================================
echo  DreamcastRecompiled v1.2 Rev 2 - Crazy Taxi 2 FPS
echo  Mode: %DCR_TEST_MODE%
echo  Boot: current per-game profile / existing ABI1 SH-4 guest
echo  Renderer: DX11     SPG fast-forward: ON
echo ===============================================================
echo.
if /I "%DCR_TEST_MODE%"=="PROFILE" (
  echo [CT2] PERF and HOTPC sampling adds CPU overhead. Do NOT use
  echo [CT2] this run to compare FPS. It identifies hotspots only.
) else (
  echo [CT2] BASELINE run: no heavy performance sampling.
)
echo [CT2] This script will not rebuild your game's native SH-4 module.
echo [CT2] If --reuse fails, prepare the title using the main launcher.
echo.
call "scripts\windows\run_game.bat" "%DCR_TEST_DISC%" --reuse --gpu --spg-ff-on --hot-recovery-on --heartbeat-on --no-portable-sync --no-pause %DCR_TEST_FLAGS%
set "DCR_RESULT=%ERRORLEVEL%"
echo.
echo [CT2] Session ended. Exit code: %DCR_RESULT%
echo [CT2] Attach your new session log from logs\ for analysis.
popd
exit /b %DCR_RESULT%
:usage
echo Usage:
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi"
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --profile
exit /b 2
