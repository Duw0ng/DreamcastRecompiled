@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\2ndmix.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia el 2ndmix.elf real a samples\2ndmix.elf o pasalo como primer argumento.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat --no-pause
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\2ndmix_live rmdir /s /q generated\2ndmix_live
if exist generated\2ndmix_live-build rmdir /s /q generated\2ndmix_live-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\2ndmix_live > generated\2ndmix_live_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\2ndmix_live_codegen.txt >nul || (
  echo [ERROR] 2ndMix contiene RAW_SH4 en el grafo generado.
  type generated\2ndmix_live_codegen.txt
  exit /b 3
)

cmake -S generated\2ndmix_live -B generated\2ndmix_live-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\2ndmix_live-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp 0.0.170] 2ndMix PVR + AICA native live.
echo [INFO] El ARM7 ejecuta s3mplay y el mixer AICA sale por WinMM/waveOut.
echo [INFO] ESC o cerrar la ventana termina la prueba.
echo [INFO] AICA usa el reloj comun en modo host: 45.1584 MHz -> 44.1 kHz, independiente del rendimiento PVR; WinMM conserva ~1.11 s de prebuffer.
echo [INFO] Al cerrar, revisa dc-host-catchup, max-submit-gap-ms y underrun-restarts; idealmente underrun-restarts=0.
echo [INFO] Live ya no usa --aica-sh4-div ni --aica-pvr-sync; Timer A 16/21 conserva la calibracion temporal validada de este antiguo s3mplay.
echo [INFO] Maple host esta activo: teclado o control XInput alimentan un controlador Dreamcast en puerto A.
echo [INFO] Teclado: flechas=D-pad, J/Space=A, K=B, U=X, I=Y, Enter=Start, Q/E=triggers, WASD=analogico.
echo [INFO] El scheduler comun separa tiempo virtual determinista y reloj host para playback en vivo.
echo.

generated\2ndmix_live-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-play ^
  --aica-timer-rate=16/21 ^
  --device-clock-host ^
  --maple-host-input ^
  --pvr-window ^
  --pvr-frame-sync ^
  --pvr-window-scale=2 ^
  --pvr-window-interval=256 ^
  --pvr-window-throttle-ms=0 ^
  --pvr-window-fps=60

set RC=%ERRORLEVEL%
if not "%RC%"=="0" exit /b %RC%
echo.
echo [OK] 2ndMix live finalizado.
exit /b 0
