@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_no_background_probe.bat "ruta\juego.cdi"
  echo.
  echo Probe 1/4: suprime solamente el background plane del PVR.
  echo Si aparece el 3D, el problema esta en background/depth composition.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%
set DCR_PVR_NO_BACKGROUND=1

echo.
echo [DreamcastRecomp 0.0.170] Probe NO-BACKGROUND
echo [INFO] Todo queda normal salvo el background plane PVR.
echo [INFO] Heartbeat esperado: pvr-3dprobe=B---/...
_cb\Release\dreamcast_program.exe ^
  --commercial-boot ^
  --disc-map=generated\commercial_recompiled\disc.map ^
  --host-window ^
  --pvr-window ^
  --pvr-frame-sync ^
  --maple-host-input ^
  --aica-arm7 ^
  --device-clock-host ^
  --pvr-profile ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo.
echo [INFO] Probe NO-BACKGROUND termino con RC=%RC%.
exit /b %RC%
