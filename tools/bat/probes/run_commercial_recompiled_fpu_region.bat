@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set EXE=_cb\Release\dreamcast_program.exe
if not exist "%EXE%" (
  echo [ERROR] No existe el runtime comercial compilado.
  echo [INFO] Ejecuta una sola vez: run_commercial_recompiled.bat "ruta\juego.cdi"
  echo [INFO] Luego este BAT relanza EL MISMO ejecutable sin recompilar.
  exit /b 1
)

set DCR_FPU_MODE=region
set DCR_FPU_METRICS=1
echo.
echo [DreamcastRecomp 0.0.170] FPU A/B: REGION 0.0.165-style
echo [INFO] Cache FR/XF local probado; telemetria completa activada para comparar con el log 0.0.165.
echo [INFO] Comparte FMA3 del ejecutable 0.0.170; no recompila.
echo [INFO] Confirma en heartbeat: fpu-mode=region165, fpu-metrics=on.
"%EXE%" ^
  --commercial-boot ^
  --disc-map=generated\commercial_recompiled\disc.map ^
  --pvr-window ^
  --pvr-frame-sync ^
  --pvr-gpu ^
  --pvr-mt ^
  --fast-dispatch ^
  --direct-dispatch ^
  --sh4-tick-batch=256 ^
  --maple-host-input ^
  --aica-arm7 ^
  --aica-play ^
  --device-clock-host ^
  --device-clock-host-max-catchup=4096 ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo [INFO] FPU region165 termino con RC=%RC%.
exit /b %RC%
