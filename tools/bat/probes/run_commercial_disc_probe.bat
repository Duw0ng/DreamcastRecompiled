@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_disc_probe.bat "ruta\juego.cdi"
  echo.
  echo Inspecciona un disco Dreamcast, localiza IP.BIN y el boot binary,
  echo y muestra los primeros datos utiles para la futura ruta comercial.
  exit /b 1
)

set DISC=%~1
if not exist "%DISC%" (
  echo [ERROR] No encuentro "%DISC%"
  exit /b 1
)

if not exist build\Release\dc_disc_probe.exe call build_windows.bat --no-pause
if errorlevel 1 exit /b 2

if not exist generated mkdir generated
if not exist generated\disc_probe mkdir generated\disc_probe

build\Release\dc_disc_probe.exe "%DISC%" ^
  --extract-ip=generated\disc_probe\IP.BIN ^
  --extract-boot=generated\disc_probe\BOOT.BIN
set RC=%ERRORLEVEL%

if "%RC%"=="0" (
  echo.
  echo [OK] Imagen reconocida. IP.BIN y boot extraidos en generated\disc_probe\ para analisis local.
) else (
  echo.
  echo [ERROR] El probe termino con RC=%RC%.
)
exit /b %RC%
