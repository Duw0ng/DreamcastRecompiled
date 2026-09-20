@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_singlethread.bat "ruta\juego.cdi"
  echo.
  echo Genera/compila el ejecutable comercial y lo arranca desde el IP.BIN real.
  echo 0.0.170 mantiene el fix Store Queue/FMOV64 y acelera el software PVR para gameplay real-time.
  echo Incluye probes separados para background, depth y shading 3D.
  echo Teclas: flechas=D-pad, Z/Space/J=A, X/K=B, C/U=X, V/I=Y, Enter=Start.
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

echo.
echo [DreamcastRecomp 0.0.170] Ejecutando bootstrap comercial recompilado...
echo [INFO] Revisa fmov64=, pvr-objsrc=, pvr-sqpt=, pvr-sq2w1=, pvr-trisrc= y pvr-onsrc=.
echo [INFO] Traza CALL/RET completa desactivada: el IP.BIN hace decenas de miles de llamadas al decodificar graficos.
echo [INFO] Si ocurre un error, se mostraran automaticamente las ultimas 32 llamadas.
echo [INFO] 0.0.170 modo comparativo single-thread: desactiva el raster PVR multihilo.
echo [INFO] El runner normal NO activa --pvr-profile para evitar medir cada triangulo durante el juego.
echo [INFO] Revisa fps= y confirma pvr-mt=off en el heartbeat.
echo [INFO] Para 3D revisa fpu3d=, pvr-3d=, pvr-3dz= y pvr-3dzm=.
echo [INFO] pvr-3dprobe=---- indica ejecucion normal sin bypass diagnostico.
_cb\Release\dreamcast_program.exe ^
  --commercial-boot ^
  --disc-map=generated\commercial_recompiled\disc.map ^
  --host-window ^
  --pvr-window ^
  --pvr-frame-sync ^
  --fast-dispatch ^
  --direct-dispatch ^
  --sh4-tick-batch=256 ^
  --maple-host-input ^
  --aica-arm7 ^
  --aica-play ^
  --device-clock-host ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo.
echo [INFO] El runner comercial termino con RC=%RC%.
if "%RC%"=="130" echo [INFO] RC=130 significa que cerraste manualmente una ventana durante la ejecucion comercial.
echo [INFO] Si falla, el runner imprime automaticamente las ultimas 32 llamadas antes de [DreamcastRecomp ERROR].
exit /b %RC%
