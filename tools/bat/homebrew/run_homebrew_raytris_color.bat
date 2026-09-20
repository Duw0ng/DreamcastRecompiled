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

if exist generated\raytris_color rmdir /s /q generated\raytris_color
if exist generated\raytris_color-build rmdir /s /q generated\raytris_color-build

build\Release\dc_recomp.exe "%ELF%" --function __rgba8888_to_argb4444 --output generated\raytris_color > generated\raytris_color_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\raytris_color_codegen.txt >nul || exit /b 2

cmake -S generated\raytris_color -B generated\raytris_color-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\raytris_color-build --config Release
if errorlevel 1 exit /b 1

generated\raytris_color-build\Release\dreamcast_program.exe --r4=0x8C100000 --r5=0x8C100100 --mem32=0x8C100000:0x78563412 --peek16=0x8C100100 > generated\raytris_color\native_output.txt
set RC=%ERRORLEVEL%
type generated\raytris_color\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"[PEEK16] 0x8C100100 = 0x7135" generated\raytris_color\native_output.txt >nul || exit /b 3
findstr /C:"PC=0xFFFFFFFF" generated\raytris_color\native_output.txt >nul || exit /b 3

echo.
echo [OK] Raytris RGBA8888 12/34/56/78 se convirtio a ARGB4444 0x7135.
exit /b 0
