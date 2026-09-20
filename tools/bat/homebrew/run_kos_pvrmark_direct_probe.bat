@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\pvrmark_strips_direct.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia pvrmark_strips_direct.elf a samples\pvrmark_strips_direct.elf o pasalo como primer argumento.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if not exist generated mkdir generated
if exist generated\pvrmark_direct rmdir /s /q generated\pvrmark_direct
if exist generated\pvrmark_direct-build rmdir /s /q generated\pvrmark_direct-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\pvrmark_direct > generated\pvrmark_direct_codegen.txt
if errorlevel 1 exit /b 2
findstr /C:"RAW_SH4:              0" generated\pvrmark_direct_codegen.txt >nul || exit /b 3

cmake -S generated\pvrmark_direct -B generated\pvrmark_direct-build -A x64
if errorlevel 1 exit /b 4
cmake --build generated\pvrmark_direct-build --config Release
if errorlevel 1 exit /b 5

generated\pvrmark_direct-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-no-input ^
  --pvr-stop-after-packets=500 ^
  --pvr-dump=generated\pvrmark_direct\frame.ppm ^
  > generated\pvrmark_direct\native_output.txt 2>&1
set RC=%ERRORLEVEL%

type generated\pvrmark_direct\native_output.txt
if not "%RC%"=="2" exit /b 6
findstr /C:"Beginning new test" generated\pvrmark_direct\native_output.txt >nul || exit /b 7
findstr /C:"TA packets=500" generated\pvrmark_direct\native_output.txt >nul || exit /b 8
findstr /C:"PVR probe packet limit reached" generated\pvrmark_direct\native_output.txt >nul || exit /b 9
if not exist generated\pvrmark_direct\frame.ppm exit /b 10

echo.
echo [OK] pvrmark_strips_direct alimento el camino PVR/SQ/TA real y genero frame.ppm.
echo [INFO] Es un raster probe basico, NO una emulacion PowerVR2 exacta.
exit /b 0
