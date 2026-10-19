@echo off
setlocal EnableExtensions
if "%~1"=="" goto :usage
if not exist "%~1" (
  echo [CT2 WAIT] Disc image not found: %~1
  exit /b 2
)
set "CT2_DISC=%~f1"
set "DCR_CT2_CALLBACK_PROBE=0"
set "DCR_SH4_RETFAST=0"
pushd "%~dp0..\.."
echo ============================================================
echo  DCR V1.2 - CT2 scheduler-safe wait coalescing A/B test
echo  Run same in-game scene for 60 seconds in both launches.
echo ============================================================
echo.
echo [A/B 1] BASELINE: wait coalescing OFF
set "DCR_CT2_WAIT_COALESCE=0"
call "scripts\windows\run_game.bat" "%CT2_DISC%" --reuse --gpu --spg-ff-on --heartbeat-on --no-portable-sync --no-pause --hotpc-off
if errorlevel 1 (
  popd
  exit /b 1
)
echo.
echo [A/B 2] OPTIMIZED: wait coalescing ON
set "DCR_CT2_WAIT_COALESCE=1"
call "scripts\windows\run_game.bat" "%CT2_DISC%" --reuse --gpu --spg-ff-on --heartbeat-on --no-portable-sync --no-pause --hotpc-off
set "EXITCODE=%ERRORLEVEL%"
popd
echo [CT2 WAIT] Send both session logs for FPS, audio and scheduler analysis.
exit /b %EXITCODE%
:usage
echo Usage: TEST_CT2_WAIT_AB.bat "C:\Games\Crazy Taxi 2.cdi"
exit /b 2
