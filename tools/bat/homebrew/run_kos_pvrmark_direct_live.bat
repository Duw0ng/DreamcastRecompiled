@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\pvrmark_strips_direct.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia pvrmark_strips_direct.elf a samples\ o pasalo como primer argumento.
  exit /b 1
)
if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1
if not exist generated mkdir generated
if exist generated\pvrmark_live rmdir /s /q generated\pvrmark_live
if exist generated\pvrmark_live-build rmdir /s /q generated\pvrmark_live-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\pvrmark_live > generated\pvrmark_live_codegen.txt
if errorlevel 1 exit /b 2
cmake -S generated\pvrmark_live -B generated\pvrmark_live-build -A x64
if errorlevel 1 exit /b 3
cmake --build generated\pvrmark_live-build --config Release
if errorlevel 1 exit /b 4

echo [INFO] ESC o cerrar la ventana termina la prueba.
generated\pvrmark_live-build\Release\dreamcast_program.exe ^
  --probe-video-default ^
  --probe-no-input ^
  --pvr-window ^
  --pvr-frame-sync ^
  --pvr-window-scale=2 ^
  --pvr-window-interval=128 ^
  --pvr-window-throttle-ms=0 ^
  --pvr-window-fps=60
exit /b %ERRORLEVEL%
