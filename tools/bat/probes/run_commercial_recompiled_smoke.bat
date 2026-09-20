@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_smoke.bat "ruta\juego.cdi"
  echo.
  echo 0.0.170 recompila el juego y ejecuta el mismo runner comercial normal,
  echo pero agrega un burst FINITO de 6 pulsos START cuando el juego comienza
  echo a consultar el control KOS. Al terminar, vuelve al input normal de host.
  echo Esto NO modifica run_commercial_recompiled.bat ni los defaults PVR/TA.
  exit /b 1
)

set DCR_GPR_CACHE=0
set DCR_EMIT_HOT_TRACE=0
call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

echo.
echo [DreamcastRecomp 0.0.170 SMOKE] Ejecutando bootstrap comercial con START burst...
echo [INFO] Se inyectan 6 pulsos START solo en este BAT y luego se devuelve el control al teclado/XInput.
echo [INFO] Si el titulo usa la ruta KOS de estado de control veras: [DreamcastRecomp smoke] START pulse N/6.
echo [INFO] Si falla SH-4, el error incluye target, guest-pc, PR, SP, SR, R0, R4 y R8 mas las ultimas 32 llamadas.
set DCR_FPU_MODE=superblock
set DCR_HOT_TRACE=0
_cb\Release\dreamcast_program.exe ^
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
  --probe-controller-start-burst ^
  --aica-arm7 ^
  --aica-play ^
  --device-clock-host ^
  --device-clock-host-max-catchup=4096 ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo.
echo [INFO] El smoke comercial termino con RC=%RC%.
echo [INFO] Log: %CD%\logs\DreamcastRecomp_0.0.170_session_YYYYMMDD-HHMMSS.log
if "%RC%"=="130" echo [INFO] RC=130 significa que cerraste manualmente una ventana durante la ejecucion comercial.
exit /b %RC%
