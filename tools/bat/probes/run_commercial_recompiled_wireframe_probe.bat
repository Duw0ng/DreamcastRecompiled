@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_wireframe_probe.bat "ruta\juego.cdi"
  echo.
  echo Dibuja TODA la geometria TA no-background como wireframe, sin clasificar 2D/3D por Z.
  echo Colores: SQ=cyan, CH2=magenta, CPU=amarillo, unknown=blanco.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%
set DCR_PVR_WIREFRAME=1
set DCR_PVR_SOURCE_ONLY=0

echo.
echo [DreamcastRecomp 0.0.170] Probe TA-WIREFRAME
echo [INFO] Background OFF, depth/textura/blend ignorados por el wireframe.
echo [INFO] NO usa la heuristica Z-variable: el tablero plano tambien debe dejar contorno si llega al raster.
echo [INFO] Colores: SQ=cyan, CH2=magenta, CPU=amarillo, unknown=blanco.
echo [INFO] Revisa pvr-wire=, pvr-sqpt=, pvr-ch2pt=, pvr-trisrc= y pvr-onsrc=.
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
echo [INFO] Probe TA-WIREFRAME termino con RC=%RC%.
exit /b %RC%
