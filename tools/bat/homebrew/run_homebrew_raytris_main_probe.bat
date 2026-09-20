@echo off
setlocal EnableDelayedExpansion
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\raytris.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia el raytris.elf suministrado a samples\raytris.elf o pasa su ruta.
  exit /b 1
)

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\raytris_main rmdir /s /q generated\raytris_main
if exist generated\raytris_main-build rmdir /s /q generated\raytris_main-build

build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\raytris_main > generated\raytris_main_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\raytris_main_codegen.txt >nul || (
  type generated\raytris_main_codegen.txt
  echo [ERROR] El _main de Raytris contiene RAW_SH4.
  exit /b 2
)

cmake -S generated\raytris_main -B generated\raytris_main-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\raytris_main-build --config Release
if errorlevel 1 exit /b 1

rem El ELF del corpus normalmente entra a _main despues del startup de KOS.
rem Como este probe salta el startup, sembramos vid_mode con el primer modo
rem incorporado del mismo ELF para atravesar las assertions iniciales de pvr_init.
rem Estos dos simbolos corresponden al raytris.elf suministrado:
rem   _vid_mode    = 0x8C43B7BC
rem   _vid_builtin = 0x8C42FF60; usamos vid_builtin[1] = 0x8C42FF90 (modo valido)

generated\raytris_main-build\Release\dreamcast_program.exe --mem32=0x8C43B7BC:0x8C42FF90 > generated\raytris_main\native_output.txt 2>&1
set RC=%ERRORLEVEL%
type generated\raytris_main\native_output.txt

findstr /C:"Welcome to GLdc!" generated\raytris_main\native_output.txt >nul || (
  echo [ERROR] Raytris no alcanzo la inicializacion de GLdc.
  exit /b 3
)
findstr /C:"address=0xA05F8008" generated\raytris_main\native_output.txt >nul || (
  echo [ERROR] El probe no se detuvo en el limite PVR esperado.
  exit /b 4
)

echo.
echo [OK CPU PROBE] Raytris recompilo su grafo de CPU y entro a GLdc.
echo [OK CPU PROBE] Se detuvo exactamente en 0xA05F8008, registro PVR_RESET,
echo                que pertenece a la fase de hardware Dreamcast pospuesta.
exit /b 0
