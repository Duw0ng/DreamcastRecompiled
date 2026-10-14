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
echo  DreamcastRecomp cpp34t - SECOND LATE-AOT ONLINE
echo ============================================================
echo  Mantiene cpp34s V42/LAPM reliability + fastpath.
echo  Requeridos: 0x8C036102 y 0x8C036118.
echo  NO recompila las 5185 funciones de ChuChu.
echo ============================================================

if exist "!SEEDS!" (
  findstr /I /X /C:"0x8C036102" "!SEEDS!" >nul 2>nul
  set "HAS1=!ERRORLEVEL!"
  findstr /I /X /C:"0x8C036118" "!SEEDS!" >nul 2>nul
  set "HAS2=!ERRORLEVEL!"
  if "!HAS1!"=="0" if "!HAS2!"=="0" goto :run
)

echo [INFO] El overlay actual no contiene ambos targets. Migrando cache de forma quirurgica...
call "%~dp0..\..\scripts\patches\PATCH_CPP34T_CHUCHU_LATE_AOT2.bat" "%DISC%"
exit /b %ERRORLEVEL%

:run
echo [INFO] Overlay cpp34t ya preparado; reutilizando guest/cache existente.
call "%~dp0..\windows\run_game.bat" "%DISC%" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
