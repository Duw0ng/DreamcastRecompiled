@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\raytris.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia raytris.elf del corpus a samples\raytris.elf o pasa su ruta.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\raytris_pixeldata rmdir /s /q generated\raytris_pixeldata
if exist generated\raytris_pixeldata-build rmdir /s /q generated\raytris_pixeldata-build

build\Release\dc_recomp.exe "%ELF%" --function _rlGetPixelDataSize --output generated\raytris_pixeldata > generated\raytris_pixeldata_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\raytris_pixeldata_codegen.txt >nul || (
  type generated\raytris_pixeldata_codegen.txt
  echo [ERROR] El test contiene RAW_SH4.
  exit /b 2
)

cmake -S generated\raytris_pixeldata -B generated\raytris_pixeldata-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\raytris_pixeldata-build --config Release
if errorlevel 1 exit /b 1

generated\raytris_pixeldata-build\Release\dreamcast_program.exe --r4=64 --r5=32 --r6=7 > generated\raytris_pixeldata\native_output.txt
set RC=%ERRORLEVEL%
type generated\raytris_pixeldata\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=8192" generated\raytris_pixeldata\native_output.txt >nul || exit /b 3
findstr /C:"PC=0xFFFFFFFF" generated\raytris_pixeldata\native_output.txt >nul || exit /b 3

echo.
echo [OK] Raytris _rlGetPixelDataSize(64,32,7) devolvio 8192 nativamente.
exit /b 0
