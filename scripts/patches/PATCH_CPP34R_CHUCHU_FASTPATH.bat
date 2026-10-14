@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\..\"

if "%~1"=="" (
  echo Uso: %~nx0 "ruta\ccr.cdi" [0xTARGET]
  echo Ejemplo: %~nx0 "C:\ruta\ccr.cdi" 0x8C036102
  exit /b 1
)

set "DISC=%~f1"
set "TARGET=%~2"
if not defined TARGET set "TARGET=0x8C036102"

if defined LOCALAPPDATA (
  set "GAME_DIR=%LOCALAPPDATA%\DreamcastRecompiled\games\CHUCHU_ROCKET"
) else (
  set "GAME_DIR=%CD%\games\CHUCHU_ROCKET"
)

if not exist "%DISC%" (
  echo [ERROR] No encuentro CDI: "%DISC%"
  exit /b 1
)
if not exist "!GAME_DIR!\generated\BOOTSTRAP.BIN" (
  echo [ERROR] No encuentro el cache existente de ChuChu:
  echo         !GAME_DIR!\generated\BOOTSTRAP.BIN
  echo [INFO] Ejecuta primero una version que ya tenga el guest ABI1 compilado.
  exit /b 2
)

where cmake >nul 2>nul || (echo [ERROR] CMake no esta en PATH.& exit /b 2)
where python >nul 2>nul || (echo [ERROR] Python no esta en PATH.& exit /b 2)

echo ============================================================
echo  DreamcastRecomp cpp34r - Network fastpath + existing Late-AOT overlay
echo ============================================================
echo  CDI    : %DISC%
echo  Cache  : !GAME_DIR!
echo  Target : !TARGET!
echo  El guest principal NO se recompila.
echo ============================================================
echo.

echo [1/3] Verificando dc_raw_recomp...
cmake -S . -B build -A x64
if errorlevel 1 exit /b 3
cmake --build build --config Release --target dc_raw_recomp --parallel 1
if errorlevel 1 exit /b 3

echo.
echo [2/3] Generando/compilando overlay AOT minimo...
python "tools\late_target\BuildLateTargetPatch.py" --root "%CD%" --game-dir "!GAME_DIR!" --target "!TARGET!"
if errorlevel 1 exit /b 4

echo.
echo [3/3] Ejecutando ChuChu con guest ABI1 + overlay + cpp34r network fastpath...
call "%~dp0..\windows\run_game.bat" "%DISC%" --reuse --rebuild-network
exit /b %ERRORLEVEL%
