@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompiled_audio_probe.bat "ruta\juego.cdi"
  echo.
  echo Ejecuta el mixer AICA/CDDA heredado de 0.0.141 con reloj host y guarda dos capturas cada 15 segundos:
  echo   generated\commercial_recompiled\aica_live.wav  ^(mezcla final^)
  echo   generated\commercial_recompiled\cdda_live.wav  ^(CDDA antes del mixer^)
  exit /b 1
)

call run_commercial_recompile.bat "%~1"
if errorlevel 1 exit /b %ERRORLEVEL%

echo.
echo [DreamcastRecomp 0.0.170] AICA + CDDA live + dual WAV probe...
echo [INFO] Los WAV se actualizan automaticamente cada 15 segundos; no hace falta cerrar el runner.
_cb\Release\dreamcast_program.exe ^
  --commercial-boot ^
  --disc-map=generated\commercial_recompiled\disc.map ^
  --host-window ^
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
  --aica-wav=generated\commercial_recompiled\aica_live.wav ^
  --cdda-wav=generated\commercial_recompiled\cdda_live.wav ^
  --device-clock-host ^
  --device-clock-host-max-catchup=4096 ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%
echo.
echo [INFO] Audio probe termino con RC=%RC%.
echo [INFO] cdda-src=Tn/FAD/FRAME/ENC/ENC_N/RAW_DELTA/YB_DELTA/xTRANS/rREPAIRS/ORDER/... muestra la fuente CDDA.
echo [INFO] En T4 del CDI de prueba se espera yb hasta FAD 49601, una transicion, y raw desde 49602.
echo [INFO] ORDER=le o be es una decision fuerte; ?le aun analiza; le? es fallback conservador tras material ambiguo.
echo [INFO] Si vuelve la estatica, cdda_live.wav permite separarla del mixer AICA/WinMM.
exit /b %RC%
