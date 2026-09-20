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

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\2ndmix_perf rmdir /s /q generated\2ndmix_perf
if exist generated\2ndmix_perf-build rmdir /s /q generated\2ndmix_perf-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\2ndmix_perf > generated\2ndmix_perf_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\2ndmix_perf_codegen.txt >nul || exit /b 3

cmake -S generated\2ndmix_perf -B generated\2ndmix_perf-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\2ndmix_perf-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp] Perfilando 35000 paquetes TA sin ventana ni escritura de frames...
echo.

generated\2ndmix_perf-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-skip-audio ^
  --probe-no-input ^
  --pvr-frame-sync ^
  --pvr-profile ^
  --pvr-stop-after-packets=35000 ^
  > generated\2ndmix_perf\native_output.txt 2>&1
set RC=%ERRORLEVEL%

type generated\2ndmix_perf\native_output.txt
if not "%RC%"=="2" (
  echo [ERROR] Se esperaba la parada controlada RC=2 y se recibio RC=%RC%.
  exit /b 6
)
findstr /C:"[PVR profile]" generated\2ndmix_perf\native_output.txt >nul || exit /b 7
findstr /C:"TA packets=35000" generated\2ndmix_perf\native_output.txt >nul || exit /b 8

echo.
echo [OK] Perfil PVR completado. El valor "unthrottled" muestra la capacidad CPU/PVR host sin limite artificial de FPS.
exit /b 0
