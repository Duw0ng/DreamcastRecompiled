@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_3d_isolate_probe.bat "ruta\juego.cdi"
  echo.
  echo Probe 4/4 decisivo: SOLO geometria 3D, blanca, depth ALWAYS y sin background.
  echo Tambien evita shading/textura/blending para comprobar si XYZ llega al raster.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%
set DCR_PVR_NO_BACKGROUND=1
set DCR_PVR_3D_DEPTH_ALWAYS=1
set DCR_PVR_3D_WHITE=1
set DCR_PVR_3D_ONLY=1

echo.
echo [DreamcastRecomp 0.0.170] Probe 3D-ISOLATE
echo [INFO] Solo quedan triangulos con Z variable: blanco, sin textura/blend, sin background, depth ALWAYS.
echo [INFO] Si aqui tampoco aparece el tablero/cubo/ratas, mirar FTRV/FPU/XYZ y framing TA.
echo [INFO] Heartbeat esperado: pvr-3dprobe=BDWO/...
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
echo [INFO] Probe 3D-ISOLATE termino con RC=%RC%.
exit /b %RC%
