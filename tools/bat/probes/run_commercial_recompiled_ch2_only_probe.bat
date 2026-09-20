@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_ch2_only_probe.bat "ruta\juego.cdi"
  echo.
  echo Aisla superficies cuyo Global Parameter entro por CH2 DMA y las dibuja en wireframe.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%
set DCR_PVR_WIREFRAME=1
set DCR_PVR_SOURCE_ONLY=CH2

echo.
echo [DreamcastRecomp 0.0.170] Probe CH2-ONLY-WIREFRAME
echo [INFO] Solo superficies TA cuyo header llego por CH2 DMA. Background OFF.
echo [INFO] Si el tablero aparece aqui pero no en SQ-ONLY, la separacion de rutas queda demostrada.
echo [INFO] Heartbeat esperado: pvr-wire=on/... y pvr-srcprobe=2/...
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
echo [INFO] Probe CH2-ONLY-WIREFRAME termino con RC=%RC%.
exit /b %RC%
