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
echo [INFO] WinMM arranca con ~1.11 s de prebuffer, chunks de ~46 ms y hasta ~2.97 s de cola para absorber jitter del runtime.
echo [INFO] Al cerrar, revisa "min-queue" y "underrun-restarts": idealmente min-queue > 0 y underrun-restarts=0.
echo [INFO] Live usa --aica-sh4-div=1 como modo temporal de throughput y Timer A 1/1 para comparar el modelo de temporizador AICA sin calibracion del reproductor historico.
echo [INFO] El scheduler comun ya no depende de --aica-sh4-div ni --aica-pvr-sync.
echo.

generated\2ndmix_live-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-play ^
  --aica-timer-rate=1/1 ^
  --aica-sh4-div=1 ^
  --probe-no-input ^
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
