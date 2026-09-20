@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_homebrew_maple_live.bat "ruta\demo.elf"
  echo.
  echo Recompila un ELF KallistiOS y lo ejecuta con PVR + reloj host + controlador Maple.
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
if exist generated\maple_live rmdir /s /q generated\maple_live
if exist generated\maple_live-build rmdir /s /q generated\maple_live-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 4096 --output generated\maple_live > generated\maple_live_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\maple_live_codegen.txt >nul || (
  echo [ERROR] El grafo generado todavia contiene RAW_SH4. Este ELF revelo una instruccion/funcion que debemos implementar.
  type generated\maple_live_codegen.txt
  exit /b 3
)

cmake -S generated\maple_live -B generated\maple_live-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\maple_live-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp 0.0.170] Maple interactive homebrew runner
echo [INFO] Teclado: flechas=D-pad, J/Space=A, K=B, U=X, I=Y, Enter=Start.
echo [INFO] Q/E=triggers y WASD=stick analogico. XInput se detecta automaticamente si esta disponible.
echo [INFO] ESC/cerrar ventana termina la prueba host.
echo.

generated\maple_live-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-play ^
  --device-clock-host ^
  --maple-host-input ^
  --pvr-window ^
  --pvr-frame-sync ^
  --pvr-window-scale=2 ^
  --pvr-window-fps=60

exit /b %ERRORLEVEL%
