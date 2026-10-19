@echo off
setlocal EnableExtensions

rem CT2 callback probe: runtime-only diagnostics. Does NOT rebuild SH-4 guest DLL.
if "%~1"=="" goto :usage
if not exist "%~1" (
  echo [CT2 PROBE] Disc image not found: %~1
  exit /b 2
)
set "CT2_DISC=%~f1"
set "DCR_CT2_CALLBACK_PROBE=1"
pushd "%~dp0..\.."
echo.
echo ================================================================
echo  DreamcastRecompiled V1.2 - CT2 callback and counter probe
echo  Guest ABI unchanged / existing SH-4 native cache reused
echo  Captures callback near 0x8C15787C and scheduler counters
echo  Not a valid FPS benchmark: guest call history adds overhead
echo ================================================================
echo.
call "scripts\windows\run_game.bat" "%CT2_DISC%" --reuse --gpu --spg-ff-on --heartbeat-on --no-portable-sync --no-pause --hotpc-off
set "CT2_RESULT=%ERRORLEVEL%"
popd
echo [CT2 PROBE] Finished with code %CT2_RESULT%. Upload the new session log.
exit /b %CT2_RESULT%

:usage
echo Usage: PROBE_CT2_CALLBACK.bat "C:\Games\Crazy Taxi 2.cdi"
exit /b 2
