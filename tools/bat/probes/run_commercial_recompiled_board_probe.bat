@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_board_probe.bat "ruta\juego.cdi"
  echo.
  echo Diagnostico PVR extremo: toda geometria TA no-background se dibuja blanca.
  echo Este modo desactiva texturas, depth test, blending y USER TILE CLIP como
  echo posibles causas. Si el tablero sigue sin aparecer, su geometria no esta
  echo llegando como primitivas validas y hay que seguir el framing/origen TA.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

set DCR_PVR_BOARD_GEOMETRY=1
echo.
echo [DreamcastRecomp 0.0.170] Ejecutando probe BOARD-GEOMETRY...
echo [INFO] Todas las primitivas no-background: blanco solido, textura OFF, depth test OFF, blend OFF, clip OFF.
echo [INFO] 0.0.170 acepta coordenadas TA finitas fuera de pantalla como Flycast; NaN/Inf siguen rechazados.
echo [INFO] El raster software recorta el bounding box antes de convertir a enteros.
echo [INFO] Mira pvr-spr=, pvr-stail=, pvr-objset=, pvr-tafsm= y pvr-strip=.
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
echo [INFO] Probe BOARD-GEOMETRY termino con RC=%RC%.
echo [INFO] En heartbeat debe aparecer pvr-boardprobe=on.
echo [INFO] Revisa tambien pvr-badv=total/nonfinite/extreme/probe-accepted; probe-accepted debe quedar en 0.
exit /b %RC%
