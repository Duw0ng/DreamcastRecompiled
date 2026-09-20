@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_bootstrap_probe.bat "ruta\juego.cdi"
  echo.
  echo Extrae IP.BIN + boot, prepara la imagen igual que una consola y realiza
  echo discovery estatico desde 0x8C008300. No modifica el CDI.
  exit /b 1
)

set DISC=%~1
if not exist "%DISC%" (
  echo [ERROR] No encuentro "%DISC%"
  exit /b 1
)

if not exist build\Release\dc_disc_probe.exe call build_windows.bat --no-pause
if errorlevel 1 exit /b 2
if not exist build\Release\dc_boot_prepare.exe exit /b 2
if not exist build\Release\dc_raw_boot_probe.exe exit /b 2

if not exist generated mkdir generated
if exist generated\commercial_bootstrap rmdir /s /q generated\commercial_bootstrap
mkdir generated\commercial_bootstrap

build\Release\dc_disc_probe.exe "%DISC%" ^
  --extract-ip=generated\commercial_bootstrap\IP.BIN ^
  --extract-boot=generated\commercial_bootstrap\BOOT.DISC.BIN
if errorlevel 1 exit /b 3

build\Release\dc_boot_prepare.exe ^
  generated\commercial_bootstrap\IP.BIN ^
  generated\commercial_bootstrap\BOOT.DISC.BIN ^
  --mode=auto ^
  --boot-out=generated\commercial_bootstrap\BOOT.BIN ^
  --combined-out=generated\commercial_bootstrap\BOOTSTRAP.BIN
if errorlevel 1 exit /b 4

echo.
echo [DreamcastRecomp 0.0.170] Raw console bootstrap discovery
build\Release\dc_raw_boot_probe.exe generated\commercial_bootstrap\BOOTSTRAP.BIN ^
  --base=0x8C008000 ^
  --entry=0x8C008300 ^
  --max-blocks=8192 ^
  --max-instructions=300000
set RC=%ERRORLEVEL%

echo.
if "%RC%"=="0" (
  echo [OK] Baseline comercial finalizado.
) else (
  echo [INFO] Probe parcial RC=%RC%; los UNKNOWN del probe agresivo no son faltas ISA definitivas.
)
exit /b %RC%
