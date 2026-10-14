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

if not exist "%DISC%" (
  echo [ERROR] No encuentro CDI: "%DISC%"
  exit /b 1
)
if not exist "!GAME_DIR!\generated\BOOTSTRAP.BIN" (
  echo [ERROR] No encuentro el cache ABI1 existente de ChuChu:
  echo         !GAME_DIR!\generated\BOOTSTRAP.BIN
  echo [INFO] Esta version no debe recompilar el guest completo.
  exit /b 2
)

where cmake >nul 2>nul || (echo [ERROR] CMake no esta en PATH.& exit /b 2)
where python >nul 2>nul || (echo [ERROR] Python no esta en PATH.& exit /b 2)

echo ============================================================
echo  DreamcastRecomp cpp34v - Late-AOT x4
echo  0x8C036102 + 0x8C036118 + 0x8C036236 + 0x8C036526
echo ============================================================
echo  CDI   : %DISC%
echo  Cache : !GAME_DIR!
echo  El guest principal de 5185 funciones NO se recompila.
echo ============================================================
echo.

echo [1/3] Verificando dc_raw_recomp...
cmake -S . -B build -A x64
if errorlevel 1 exit /b 3
cmake --build build --config Release --target dc_raw_recomp --parallel 1
if errorlevel 1 exit /b 3

echo.
echo [2/3] Generando un unico overlay con los cuatro targets...
python "tools\late_target\BuildLateTargetPatch.py" --root "%CD%" --game-dir "!GAME_DIR!" --target 0x8C036102 --target 0x8C036118 --target 0x8C036236 --target 0x8C036526
if errorlevel 1 exit /b 4

echo.
echo [3/3] Ejecutando con cpp34s network + cpp34v Late-AOT...
call "%~dp0..\windows\run_game.bat" "%DISC%" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
