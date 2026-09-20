@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\2ndmix.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia dreamcast\2ndmix\2ndmix.elf a samples\2ndmix.elf o pasa su ruta.
  exit /b 1
)
if not exist samples\pcx_1x1_8bpp.bin (
  echo [ERROR] Falta samples\pcx_1x1_8bpp.bin
  exit /b 1
)
if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\2ndmix_pcx rmdir /s /q generated\2ndmix_pcx
if exist generated\2ndmix_pcx-build rmdir /s /q generated\2ndmix_pcx-build

build\Release\dc_recomp.exe "%ELF%" --function _load_pcx --max-functions 64 --output generated\2ndmix_pcx > generated\2ndmix_pcx_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\2ndmix_pcx_codegen.txt >nul || exit /b 2

cmake -S generated\2ndmix_pcx -B generated\2ndmix_pcx-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\2ndmix_pcx-build --config Release
if errorlevel 1 exit /b 1

rem 2ndMix globals in the supplied ELF:
rem   _pcxpal      = 0x8C0982A4
rem   _image       = 0x8C0982A8
rem   _imageHeight = 0x8C0982AC
rem   _imageWidth  = 0x8C0982AE
rem Guest buffers are ordinary Dreamcast main RAM.
generated\2ndmix_pcx-build\Release\dreamcast_program.exe ^
  --r4=0x8C100000 ^
  --membin=0x8C100000:samples\pcx_1x1_8bpp.bin ^
  --mem32=0x8C0982A8:0x8C110000 ^
  --mem32=0x8C0982A4:0x8C120000 ^
  --peek8=0x8C110000 ^
  --peek8=0x8C120000 ^
  --peek8=0x8C1202FF ^
  --peek16=0x8C0982AE ^
  --peek16=0x8C0982AC > generated\2ndmix_pcx\native_output.txt
set RC=%ERRORLEVEL%
type generated\2ndmix_pcx\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=1" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK8] 0x8C110000 = 0x2A" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK8] 0x8C120000 = 0x11" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK8] 0x8C1202FF = 0x10" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK16] 0x8C0982AE = 0x1" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK16] 0x8C0982AC = 0x1" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3
findstr /C:"PC=0xFFFFFFFF" generated\2ndmix_pcx\native_output.txt >nul || exit /b 3

echo.
echo [OK] 2ndMix load_pcx real: RLE/struct/memcpy/paleta correctos.
exit /b 0
