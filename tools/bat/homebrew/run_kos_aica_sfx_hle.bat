@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\sfx.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia el sfx.elf real de KallistiOS a samples\sfx.elf o pasalo como primer argumento.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\aica_sfx_hle rmdir /s /q generated\aica_sfx_hle
if exist generated\aica_sfx_hle-build rmdir /s /q generated\aica_sfx_hle-build

build\Release\dc_recomp.exe "%ELF%" --function _snd_sh4_to_aica --max-functions 256 --output generated\aica_sfx_hle > generated\aica_sfx_hle_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\aica_sfx_hle_codegen.txt >nul || (
  echo [ERROR] _snd_sh4_to_aica contiene RAW_SH4.
  type generated\aica_sfx_hle_codegen.txt
  exit /b 3
)

cmake -S generated\aica_sfx_hle -B generated\aica_sfx_hle-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\aica_sfx_hle-build --config Release
if errorlevel 1 exit /b 5

echo.
echo [DreamcastRecomp] Ejecutando cola AICA real de KallistiOS.
echo [INFO] Se deberia escuchar un tono corto de 440 Hz y generar tone.wav.
echo.

generated\aica_sfx_hle-build\Release\dreamcast_program.exe ^
  --aica-kos-hle ^
  --aica-play ^
  --aica-wav=generated\aica_sfx_hle\tone.wav ^
  --membin=0xA0830000:samples\aica_tone_pcm16.bin ^
  --membin=0x8C100000:samples\aica_chan_start_packet.bin ^
  --r4=0x8C100000 ^
  --r5=24 ^
  > generated\aica_sfx_hle\native_output.txt 2>&1
set RC=%ERRORLEVEL%
type generated\aica_sfx_hle\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"commands=1" generated\aica_sfx_hle\native_output.txt >nul || exit /b 6
findstr /C:"channel-starts=1" generated\aica_sfx_hle\native_output.txt >nul || exit /b 7
if not exist generated\aica_sfx_hle\tone.wav exit /b 8

echo.
echo [OK] SH-4 KallistiOS -^> AICA command queue HLE -^> PCM host/WAV funcionando.
echo [INFO] Esto NO ejecuta aun el ARM7; valida la interfaz estandar KOS y PCM 8/16-bit.
exit /b 0
