@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_type7_probe.bat "ruta\juego.cdi"
  echo.
  echo Diagnostico PVR: conserva geometria/UV/texturas, pero fuerza a blanco la modulacion de vertices TA type 7/8.
  echo Si el tablero aparece aqui y no en el runner normal, el fallo esta en intensity/face-color.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

set DCR_PVR_TYPE7_UNLIT=1
echo.
echo [DreamcastRecomp 0.0.170] Ejecutando probe TYPE7-UNLIT...
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
echo [INFO] Probe TYPE7-UNLIT termino con RC=%RC%.
echo [INFO] En heartbeat debe aparecer pvr-i7probe=on.
exit /b %RC%
