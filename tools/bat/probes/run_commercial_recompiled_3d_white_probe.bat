@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_3d_white_probe.bat "ruta\juego.cdi"
  echo.
  echo Probe 3/4: los triangulos 3D se dibujan blanco solido sin textura/shading/blend.
  echo El depth test y background siguen normales para separar shading de Z.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%
set DCR_PVR_3D_WHITE=1

echo.
echo [DreamcastRecomp 0.0.170] Probe 3D-WHITE
echo [INFO] Si aparece geometria blanca, falla shading/textura/alpha/blending y no la geometria.
echo [INFO] Heartbeat esperado: pvr-3dprobe=--W-/...
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
echo [INFO] Probe 3D-WHITE termino con RC=%RC%.
exit /b %RC%
