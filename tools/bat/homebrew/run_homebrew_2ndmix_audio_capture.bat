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
if exist generated\2ndmix_audio rmdir /s /q generated\2ndmix_audio
if exist generated\2ndmix_audio-build rmdir /s /q generated\2ndmix_audio-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\2ndmix_audio > generated\2ndmix_audio_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\2ndmix_audio_codegen.txt >nul || (
  echo [ERROR] 2ndMix contiene RAW_SH4 en el grafo generado.
  type generated\2ndmix_audio_codegen.txt
  exit /b 3
)

cmake -S generated\2ndmix_audio -B generated\2ndmix_audio-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\2ndmix_audio-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp 0.0.170] Capturando 5 segundos virtuales de AICA nativo...
echo.

generated\2ndmix_audio-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-kos-aica-defaults ^
  --aica-arm7 ^
  --aica-timer-rate=16/21 ^
  --device-clock ^
  --probe-no-input ^
  --aica-wav=generated\2ndmix_audio\2ndmix_native.wav ^
  --aica-capture-ms=5000

set RC=%ERRORLEVEL%
if not "%RC%"=="0" exit /b %RC%
if not exist generated\2ndmix_audio\2ndmix_native.wav (
  echo [ERROR] No se genero el WAV esperado.
  exit /b 6
)

echo.
echo [OK] WAV generado:
echo   %CD%\generated\2ndmix_audio\2ndmix_native.wav
exit /b 0
