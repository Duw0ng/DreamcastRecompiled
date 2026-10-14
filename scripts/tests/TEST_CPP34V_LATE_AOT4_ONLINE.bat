@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "ruta\ccr.cdi"
  exit /b 1
)
set "DISC=%~f1"
if defined LOCALAPPDATA (
  set "GAME_DIR=%LOCALAPPDATA%\DreamcastRecompiled\games\CHUCHU_ROCKET"
) else (
  set "GAME_DIR=%CD%\games\CHUCHU_ROCKET"
)
set "SEEDS=!GAME_DIR!\late_aot\late_target_seeds.txt"

echo ============================================================
echo  DreamcastRecomp cpp34v - FOURTH LATE-AOT ONLINE
echo ============================================================
echo  Mantiene cpp34s V42/LAPM reliability + fastpath.
echo  Targets: 0x8C036102, 0x8C036118, 0x8C036236, 0x8C036526.
echo  NO recompila las 5185 funciones de ChuChu.
echo ============================================================

set "READY=0"
if exist "!SEEDS!" (
  findstr /I /X /C:"0x8C036102" "!SEEDS!" >nul 2>nul
  set "HAS1=!ERRORLEVEL!"
  findstr /I /X /C:"0x8C036118" "!SEEDS!" >nul 2>nul
  set "HAS2=!ERRORLEVEL!"
  findstr /I /X /C:"0x8C036236" "!SEEDS!" >nul 2>nul
  set "HAS3=!ERRORLEVEL!"
  findstr /I /X /C:"0x8C036526" "!SEEDS!" >nul 2>nul
  set "HAS4=!ERRORLEVEL!"
  if "!HAS1!"=="0" if "!HAS2!"=="0" if "!HAS3!"=="0" if "!HAS4!"=="0" set "READY=1"
)

if "!READY!"=="1" goto :run

echo [INFO] El overlay actual no contiene los cuatro targets. Migrando cache de forma quirurgica...
call "%~dp0..\..\scripts\patches\PATCH_CPP34V_CHUCHU_LATE_AOT4.bat" "%DISC%"
exit /b %ERRORLEVEL%

:run
echo [INFO] Overlay cpp34v ya preparado; reutilizando guest/cache existente.
call "%~dp0..\windows\run_game.bat" "%DISC%" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
