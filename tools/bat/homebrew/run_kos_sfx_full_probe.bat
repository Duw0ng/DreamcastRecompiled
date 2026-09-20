@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\sfx.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia el sound\sfx\sfx.elf real de KallistiOS a samples\sfx.elf o pasalo como primer argumento.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat --no-pause
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\kos_sfx_native rmdir /s /q generated\kos_sfx_native
if exist generated\kos_sfx_native-build rmdir /s /q generated\kos_sfx_native-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 4096 --output generated\kos_sfx_native > generated\kos_sfx_native_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\kos_sfx_native_codegen.txt >nul || (
  echo [ERROR] El _main del ejemplo SFX contiene RAW_SH4.
  type generated\kos_sfx_native_codegen.txt
  exit /b 3
)

cmake -S generated\kos_sfx_native -B generated\kos_sfx_native-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\kos_sfx_native-build --config Release
if errorlevel 1 exit /b 5

set WAV=generated\kos_sfx_native\sfx_standard_arm7.wav
set LOG=generated\kos_sfx_native\native_output.txt

echo.
echo [DreamcastRecomp 0.0.170] SFX standard KOS deterministic probe
echo [INFO] ROMFS real -^> WAV real -^> snd_sfx_play SH-4 -^> cola KOS -^> stream.drv ARM7 -^> AICA nativo.
echo [INFO] No usa --aica-kos-hle ni sustituye snd_sfx_load. El probe solo pulsa A, espera y luego Start.
echo.

generated\kos_sfx_native-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --device-clock ^
  --probe-controller-a-then-start ^
  --aica-wav=%WAV% ^
  > %LOG% 2>&1
set RC=%ERRORLEVEL%

type %LOG%
if not "%RC%"=="0" (
  echo [ERROR] El probe SFX termino con RC=%RC%.
  exit /b 6
)
findstr /R /C:"native-starts=[1-9]" %LOG% >nul || (
  echo [ERROR] No se observo ningun key-on AICA nativo.
  exit /b 7
)
findstr /R /C:"nonzero=[1-9]" %LOG% >nul || (
  echo [ERROR] No se mezclo PCM no nulo.
  exit /b 8
)
if not exist %WAV% (
  echo [ERROR] No se genero %WAV%.
  exit /b 9
)

echo.
echo [OK] SFX real: ROMFS -^> KOS SH-4 -^> stream.drv ARM7 -^> slot AICA -^> WAV nativo.
exit /b 0
