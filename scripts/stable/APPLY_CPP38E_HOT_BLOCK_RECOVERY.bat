@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\..\"

set "GAME_DIR="
if /I "%~1"=="--game-dir" set "GAME_DIR=%~2"
if not defined GAME_DIR (
  if defined DCR_CACHE_ROOT (
    set "GAME_DIR=%DCR_CACHE_ROOT%\CHUCHU_ROCKET"
  ) else if defined LOCALAPPDATA (
    set "GAME_DIR=%LOCALAPPDATA%\DreamcastRecompiled\games\CHUCHU_ROCKET"
  ) else (
    set "GAME_DIR=%CD%\games\CHUCHU_ROCKET"
  )
)
set "CPP=!GAME_DIR!\generated\cpp"
set "CB=!GAME_DIR!\build"
set "DLL=!CB!\Release\dcr_game_module.dll"
set "STAMP=!GAME_DIR!\.dcr_guest_impl_revision"

if not exist "!CPP!\CMakeLists.txt" (
  echo [cpp38e ERROR] No existe guest generado: "!CPP!\CMakeLists.txt"
  echo [cpp38e] Ejecuta primero el juego con una version que ya tenga el cache ABI1 o usa --recompile-game.
  exit /b 20
)
if not exist "!DLL!" (
  echo [cpp38e ERROR] Falta el guest ABI1 compilado: "!DLL!"
  exit /b 21
)

set "READY=0"
if exist "!STAMP!" (
  findstr /X /C:"cpp38e-hot-block-recovery-v1" "!STAMP!" >nul 2>nul
  if not errorlevel 1 if exist "!CPP!\.dcr_cpp38e_hot_block_recovery" set "READY=1"
)
if "!READY!"=="1" (
  echo [cpp38e] HOT BLOCK RECOVERY ya aplicado. Guest intacto.
  exit /b 0
)

echo ============================================================
echo  cpp38e HOT BLOCK RECOVERY - one-time guest specialization
echo ============================================================
echo  Workspace: !GAME_DIR!
echo  No redescubre SH-4 ni vuelve a analizar el CDI.
echo  Recompila el guest generado porque cambian fast paths de codigo host.
echo ============================================================

if not exist "!CPP!\CMakeLists.cpp38e_baseline.bak" copy /y "!CPP!\CMakeLists.txt" "!CPP!\CMakeLists.cpp38e_baseline.bak" >nul
if not exist "!CB!\Release\dcr_game_module_cpp38d_baseline.dll" copy /y "!DLL!" "!CB!\Release\dcr_game_module_cpp38d_baseline.dll" >nul

python "tools\perf\patch_cpp38e_hot_block_recovery.py" "!CPP!"
if errorlevel 1 exit /b 22

cmake -S "!CPP!" -B "!CB!" -A x64 -DDCR_RUNTIME_ROOT="%CD%\runtime"
if errorlevel 1 exit /b 23
cmake --build "!CB!" --config Release --target dcr_game_module --parallel 1
if errorlevel 1 exit /b 24
if not exist "!DLL!" exit /b 25

>"!STAMP!" echo cpp38e-hot-block-recovery-v1
powershell -NoProfile -Command "$h=(Get-FileHash -Algorithm SHA256 -LiteralPath $env:DLL).Hash.ToLower(); Write-Host ('[cpp38e] guest SHA256='+$h)" 2>nul

echo [cpp38e] LISTO: HOTPC sigue apagado por defecto para benchmark limpio.
exit /b 0
