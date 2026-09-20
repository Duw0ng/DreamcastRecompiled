@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_homebrew_sfx_autotest.bat "ruta\sfx.elf"
  echo.
  echo Ejecuta una prueba automatica A -^> espera -^> Start usando el mismo ARM7/AICA real.
  echo Sirve para separar un problema de bootstrap/audio de un problema de input interactivo.
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
if exist generated\sfx_autotest rmdir /s /q generated\sfx_autotest
if exist generated\sfx_autotest-build rmdir /s /q generated\sfx_autotest-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 4096 --output generated\sfx_autotest > generated\sfx_autotest_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\sfx_autotest_codegen.txt >nul || (
  echo [ERROR] El grafo generado contiene RAW_SH4.
  type generated\sfx_autotest_codegen.txt
  exit /b 3
)

cmake -S generated\sfx_autotest -B generated\sfx_autotest-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\sfx_autotest-build --config Release
if errorlevel 1 exit /b 5

set WAV=generated\sfx_autotest\sfx_autotest.wav

echo.
echo [DreamcastRecomp 0.0.170] SFX automatic Windows diagnostic
echo [INFO] Pulsa A automaticamente, deja correr el firmware y luego pulsa Start.
echo [INFO] Usa reloj virtual determinista para comprobar bootstrap ARM7/AICA sin depender del input manual.
echo [INFO] Debes oir al menos un beep si WinMM y la ruta nativa funcionan.
echo [INFO] 0.0.170 usa el mismo hilo WinMM dedicado y ring PCM del modo live.
echo.

generated\sfx_autotest-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-play ^
  --aica-wav=%WAV% ^
  --device-clock ^
  --probe-controller-a-then-start ^
  --host-window ^
  --diag-heartbeat-ms=1000
set RC=%ERRORLEVEL%

echo.
if exist %WAV% (
  echo [INFO] WAV generado: %WAV%
) else (
  echo [WARN] No se genero WAV; esto apunta a que AICA no produjo PCM.
)
if "%RC%"=="0" (
  echo [OK] Autotest SFX finalizado.
) else (
  echo [ERROR] Autotest SFX termino con RC=%RC%.
)
exit /b %RC%
