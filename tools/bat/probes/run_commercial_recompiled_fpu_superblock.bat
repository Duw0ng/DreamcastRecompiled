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

set DCR_FPU_MODE=superblock
echo.
echo [DreamcastRecomp 0.0.170] FPU A/B: SUPERBLOCK STATIC 0.0.170
echo [INFO] Reutiliza el mismo dreamcast_program.exe; NO recompila el juego.
echo [INFO] Confirma en heartbeat: fpu-mode=superblock
echo [INFO] Compara fps=, fpu-cache=, fpu-super=, pvr-ta-stage-ms= y pvr-cpu-est-ms=.
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
echo.
echo [INFO] FPU superblock termino con RC=%RC%.
echo [INFO] Log: %CD%\logs\DreamcastRecomp_0.0.170_session_YYYYMMDD-HHMMSS.log
if "%RC%"=="130" echo [INFO] RC=130 significa que cerraste manualmente una ventana.
exit /b %RC%
