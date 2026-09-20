@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_156_baseline.bat "ruta\juego.cdi"
  echo.
  echo Genera/compila el ejecutable comercial y lo arranca desde el IP.BIN real.
  echo 0.0.170 conserva la baseline 0.0.91 y usa raster PVR D3D11 + presentacion DXGI directa, con fallback software multihilo.
  echo Incluye probes separados para background, depth y shading 3D.
  echo Teclas: flechas=D-pad, Z/Space/J=A, X/K=B, C/U=X, V/I=Y, Enter=Start. Solo se leen con la ventana PVR enfocada.
  exit /b 1
)

set DCR_GPR_CACHE=0
set DCR_EMIT_HOT_TRACE=1
call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

echo.
echo [DreamcastRecomp 0.0.170] Ejecutando A/B compatible con la ruta 0.0.156...
echo [INFO] Revisa fmov64=, pvr-objsrc=, pvr-sqpt=, pvr-sq2w1=, pvr-trisrc= y pvr-onsrc=.
echo [INFO] Traza CALL/RET completa desactivada: el IP.BIN hace decenas de miles de llamadas al decodificar graficos.
echo [INFO] Si ocurre un error, se mostraran automaticamente las ultimas 32 llamadas.
echo [INFO] A/B 0.0.156: GPR cache OFF + hot-trace codegen ON/runtime ON, conservando fixes 0.0.160/161.
echo [INFO] FPU AOT: superblock estatico por defecto. Tras compilar una vez usa los BAT fpu_region/fpu_superblock para A/B sin recompilar.
echo [INFO] El runner normal NO activa --pvr-profile para evitar medir cada triangulo durante el juego.
echo [INFO] Revisa fps=, dispatch-inline=, pvr-gpu=, aica-fmt=, cdda=, cdda-pcm=, cdda-src= y audio-starves=.
echo [INFO] Para 3D revisa fpu3d=, pvr-3d=, pvr-3dz= y pvr-3dzm=.
echo [INFO] pvr-3dprobe=---- indica ejecucion normal sin bypass diagnostico.
echo [INFO] El log completo de esta sesion se guarda en: %CD%\logs\
set DCR_FPU_MODE=superblock
set DCR_HOT_TRACE=1
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
  --aica-arm7 ^
  --aica-play ^
  --device-clock-host ^
  --device-clock-host-max-catchup=4096 ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo.
echo [INFO] El runner comercial termino con RC=%RC%.
echo [INFO] Log completo: %CD%\logs\DreamcastRecomp_0.0.170_session_YYYYMMDD-HHMMSS.log
if "%RC%"=="130" echo [INFO] RC=130 significa que cerraste manualmente una ventana durante la ejecucion comercial.
echo [INFO] Si falla, el runner imprime automaticamente las ultimas 32 llamadas antes de [DreamcastRecomp ERROR].
exit /b %RC%
