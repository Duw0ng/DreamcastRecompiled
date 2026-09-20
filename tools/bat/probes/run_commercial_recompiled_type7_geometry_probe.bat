@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_type7_geometry_probe.bat "ruta\juego.cdi"
  echo.
  echo Diagnostico PVR: conserva XYZ, depth, listas, blend y USER TILE CLIP,
  echo pero para vertices TA type 7/8 fuerza color blanco y desactiva texturas.
  echo Si el tablero aparece como geometria solida, la geometria/clip esta bien y
  echo el siguiente sospechoso es textura/TCW/mipmap/sampling. Si sigue ausente,
  echo hay que concentrarse en geometria, clipping, depth o el stream TA.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

set DCR_PVR_TYPE7_GEOMETRY=1
echo.
echo [DreamcastRecomp 0.0.170] Ejecutando probe TYPE7-GEOMETRY...
echo [INFO] Type 7/8: textura OFF + modulacion blanca; geometria/depth/clip permanecen activos.
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
echo [INFO] Probe TYPE7-GEOMETRY termino con RC=%RC%.
echo [INFO] En heartbeat debe aparecer pvr-i7geom=on.
echo [INFO] Revisa tambien pvr-clip=... y pvr-badv=....
exit /b %RC%
