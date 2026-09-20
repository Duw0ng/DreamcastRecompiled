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
if exist generated\2ndmix_graphics rmdir /s /q generated\2ndmix_graphics
if exist generated\2ndmix_graphics-build rmdir /s /q generated\2ndmix_graphics-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\2ndmix_graphics > generated\2ndmix_graphics_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\2ndmix_graphics_codegen.txt >nul || (
  echo [ERROR] 2ndMix todavia contiene RAW_SH4 en el grafo generado.
  type generated\2ndmix_graphics_codegen.txt
  exit /b 3
)

cmake -S generated\2ndmix_graphics -B generated\2ndmix_graphics-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\2ndmix_graphics-build --config Release
if errorlevel 1 exit /b 5

generated\2ndmix_graphics-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-skip-audio ^
  --probe-no-input ^
  --pvr-stop-after-packets=35000 ^
  --pvr-dump=generated\2ndmix_graphics\frame.ppm ^
  > generated\2ndmix_graphics\native_output.txt 2>&1
set RC=%ERRORLEVEL%

type generated\2ndmix_graphics\native_output.txt

rem El runtime termina deliberadamente con codigo 2 al alcanzar el limite de paquetes.
if not "%RC%"=="2" (
  echo [ERROR] Se esperaba la parada controlada RC=2 y se recibio RC=%RC%.
  exit /b 6
)
findstr /C:"Starting display" generated\2ndmix_graphics\native_output.txt >nul || exit /b 7
findstr /C:"TA packets=35000" generated\2ndmix_graphics\native_output.txt >nul || exit /b 8
findstr /C:"PVR probe packet limit reached" generated\2ndmix_graphics\native_output.txt >nul || exit /b 9
if not exist generated\2ndmix_graphics\frame.ppm exit /b 10

echo.
echo [OK] 2ndMix alcanzo su loop grafico real, envio 35000 paquetes TA y genero frame.ppm.
echo [INFO] Es un raster probe basico, NO una emulacion PowerVR2 exacta.
exit /b 0
