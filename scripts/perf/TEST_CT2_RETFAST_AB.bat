@echo off
setlocal EnableExtensions
if "%~1"=="" goto :usage
if not exist "%~1" (
  echo [DCR CT2 RETFAST] Disc image not found: %~1
  exit /b 2
)
set "CT2_DISC=%~f1"
set "DCR_CT2_CALLBACK_PROBE=0"
pushd "%~dp0..\.."
echo.
echo ================================================================
echo    DCR V1.2 - CT2 return-only fastpath A/B benchmark
echo ================================================================
echo  Part A: original native dispatcher (DCR_SH4_RETFAST=0)
echo  Close game after running the SAME scene for 60 seconds.
echo  Save its session log; each run creates its own timestamped log.
echo ================================================================
echo.
set "DCR_SH4_RETFAST=0"
call "scripts\windows\run_game.bat" "%CT2_DISC%" --reuse --gpu --spg-ff-on --heartbeat-on --no-portable-sync --no-pause --hotpc-off
if errorlevel 1 (
  echo [CT2 RETFAST] Baseline run exited with an error.
  popd
  exit /b 1
)
echo.
echo ================================================================
echo  Part B: experimental RTS/NOP native fastpath (DCR_SH4_RETFAST=1)
echo  Again, close game after running SAME scene for 60 seconds.
echo ================================================================
echo.
set "DCR_SH4_RETFAST=1"
call "scripts\windows\run_game.bat" "%CT2_DISC%" --reuse --gpu --spg-ff-on --heartbeat-on --no-portable-sync --no-pause --hotpc-off
set "CT2_EXIT=%ERRORLEVEL%"
popd
echo [CT2 RETFAST] Done. Upload BOTH session logs for accurate comparison.
exit /b %CT2_EXIT%
:usage
echo Usage: TEST_CT2_RETFAST_AB.bat "C:\Games\Crazy Taxi 2.cdi"
exit /b 2
