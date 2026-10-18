@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem CT2 A/B experiment. No original disc, ABI1 module, or save is changed.
rem Default: v1.2 262144 SH-4-cycle host sync quantum (control).
rem --fast-sync: test existing 524288 quantum (experimental).
rem --profile: adds HOTPC and perf; NEVER compare profiled FPS to baseline.
if "%~1"=="" goto :usage
if not exist "%~1" (
  echo [CT2] Disc image not found: %~1
  exit /b 2
)
set "DCR_TEST_DISC=%~f1"
set "DCR_TEST_PROFILE=0"
set "DCR_TEST_FASTSYNC=0"
set "DCR_TEST_CALLBACK=0"
shift
:parse
if "%~1"=="" goto :run
if /I "%~1"=="--profile" (
  set "DCR_TEST_PROFILE=1"
) else if /I "%~1"=="--fast-sync" (
  set "DCR_TEST_FASTSYNC=1"
) else if /I "%~1"=="--callback-probe" (
  set "DCR_TEST_CALLBACK=1"
) else (
  echo [CT2] Unknown option: %~1
  goto :usage
)
shift
goto :parse
:run
set "DCR_TEST_FLAGS=--hotpc-off"
set "DCR_TEST_SYNC=262144"
if "!DCR_TEST_FASTSYNC!"=="1" (
  set "DCR_TEST_SYNC=524288"
  set "DCR_TEST_FLAGS=!DCR_TEST_FLAGS! --sync-524"
)
if "!DCR_TEST_PROFILE!"=="1" (
  set "DCR_TEST_FLAGS=!DCR_TEST_FLAGS! --perf --hotpc"
)
set "DCR_CT2_CALLBACK_PROBE=0"
if "!DCR_TEST_CALLBACK!"=="1" (
  set "DCR_CT2_CALLBACK_PROBE=1"
  set "DCR_TEST_FLAGS=!DCR_TEST_FLAGS! --hotpc"
)
pushd "%~dp0"
echo.
echo ===============================================================
echo  Crazy Taxi 2 - verified loop and host sync A/B experiment
echo  Host sync quantum: !DCR_TEST_SYNC! SH-4 cycles
echo  Heavy profiler: !DCR_TEST_PROFILE!  ^(0=clean FPS, 1=HOTPC/Perf^)
echo  CT2 callback probe: !DCR_TEST_CALLBACK!  ^(1=sample mutable callback^)
echo  Renderer: DX11   SPG fast-forward: ON
echo  Build: existing per-game native SH-4 module --reuse
echo ===============================================================
echo.
if "!DCR_TEST_PROFILE!"=="1" (
  echo [CT2] Profile mode slows CPU. Diagnose hotspots, NOT clean FPS.
) else if "!DCR_TEST_CALLBACK!"=="1" (
  echo [CT2] Callback probe adds HOTPC overhead. NOT a clean FPS run.
) else (
  echo [CT2] Clean FPS comparison. Test same scene/duration per mode.
)
echo [CT2] Do not use a fresh compile as an FPS measurement.
echo [CT2] The game/VMU caches stay separate and are never deleted.
echo.
call "scripts\windows\run_game.bat" "%DCR_TEST_DISC%" --reuse --gpu --spg-ff-on --hot-recovery-on --heartbeat-on --no-portable-sync --no-pause !DCR_TEST_FLAGS!
set "DCR_RESULT=!ERRORLEVEL!"
echo.
echo [CT2] Complete. Exit=!DCR_RESULT!  Quantum=!DCR_TEST_SYNC!  Profile=!DCR_TEST_PROFILE!
echo [CT2] Send the newest log in logs\ and the matching mode.
popd
exit /b !DCR_RESULT!
:usage
echo Usage:
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi"
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --fast-sync
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --profile
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --fast-sync --profile
echo   TEST_CRAZY_TAXI_2_FPS.bat "C:\Games\Crazy Taxi 2.cdi" --callback-probe
exit /b 2
