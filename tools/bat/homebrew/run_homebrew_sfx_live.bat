@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_homebrew_sfx_live.bat "ruta\sfx.elf"
  echo.
  echo Recompila el ejemplo KallistiOS sound/sfx y lo ejecuta con AICA ARM7 real + Maple.
  echo Este runner NO abre PVR. Abre una ventana de estado independiente para confirmar que sigue vivo.
  echo La demo del corpus usa un stream.drv KOS historico que espera
  echo los defaults Timer/SCILV previos a _main, por eso se aplica --probe-kos-aica-defaults.
  exit /b 1
)
set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat --no-pause
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\sfx_live rmdir /s /q generated\sfx_live
if exist generated\sfx_live-build rmdir /s /q generated\sfx_live-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 4096 --output generated\sfx_live > generated\sfx_live_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\sfx_live_codegen.txt >nul || (
  echo [ERROR] El grafo generado contiene RAW_SH4.
  type generated\sfx_live_codegen.txt
  exit /b 3
)

cmake -S generated\sfx_live -B generated\sfx_live-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\sfx_live-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp 0.0.170] KOS standard SFX live runner
echo [INFO] A/B/X/Y: J-Space / K / U / I. Flechas: mismo canal. Q/E: volumen. Enter: salir.
echo [INFO] Usa stream.drv ARM7 real y los WAV reales del ROMFS. No usa AICA KOS HLE.
echo [INFO] --probe-kos-aica-defaults solo reproduce el estado AICA de arranque KOS anterior a _main.
echo [INFO] Se abre una ventana de estado HOST independiente de PVR.
echo [INFO] La consola muestra un heartbeat cada 1 s con guest PC, ARM7, AICA y Maple.
echo [FIX] Se conservan los arreglos 0.0.40.3 de host-clock y fast-forward del polling ARM7.
echo [AUDIO] 0.0.170 desacopla WinMM del SH-4 con un hilo de audio dedicado y ring PCM acotado.
echo [AUDIO] Chunks ~11 ms, prefill ~34 ms, cola de dispositivo ~46 ms y ring maximo ~185 ms.
echo [AUDIO] Si el productor se atrasa se inserta silencio en vivo en vez de reiniciar/acumular pitidos viejos.
echo [DIAG] Heartbeat: audio-ring, audio-starves, audio-overruns, producer-gap-ms y winmm-gap-ms.
echo [DIAG] idle-skipped debe crecer rapido; underrun-restarts idealmente debe quedar en 0.
echo [FIX] El texto KOS se acelera mientras se usa la fuente BIOS sintetica; heartbeat muestra bfont-fast.
echo [INFO] Tambien se guarda generated\sfx_live\sfx_live_capture.wav al salir si hubo audio.
echo [INFO] Desde 0.0.141, si Windows produce una excepcion host se imprime codigo, direccion y guest PC.
echo.

generated\sfx_live-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-play ^
  --aica-wav=generated\sfx_live\sfx_live_capture.wav ^
  --device-clock-host ^
  --maple-host-input ^
  --host-window ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%

echo.
if "%RC%"=="0" (
  echo [OK] SFX live finalizado.
  if exist generated\sfx_live\sfx_live_capture.wav echo [INFO] Captura: generated\sfx_live\sfx_live_capture.wav
) else (
  echo [ERROR] SFX live termino con RC=%RC%.
)
exit /b %RC%
