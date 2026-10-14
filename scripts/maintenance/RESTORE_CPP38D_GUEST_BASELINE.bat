@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\..\"
if defined DCR_CACHE_ROOT (set "GAME_DIR=%DCR_CACHE_ROOT%\CHUCHU_ROCKET") else if defined LOCALAPPDATA (set "GAME_DIR=%LOCALAPPDATA%\DreamcastRecompiled\games\CHUCHU_ROCKET") else (set "GAME_DIR=%CD%\games\CHUCHU_ROCKET")
set "CPP=!GAME_DIR!\generated\cpp"
set "CB=!GAME_DIR!\build"
if not exist "!CB!\Release\dcr_game_module_cpp38d_baseline.dll" (
  echo [RESTORE ERROR] No existe backup cpp38d del guest.
  exit /b 1
)
copy /y "!CB!\Release\dcr_game_module_cpp38d_baseline.dll" "!CB!\Release\dcr_game_module.dll" >nul
if exist "!CPP!\CMakeLists.cpp38e_baseline.bak" copy /y "!CPP!\CMakeLists.cpp38e_baseline.bak" "!CPP!\CMakeLists.txt" >nul
del /q "!CPP!\.dcr_cpp38e_hot_block_recovery" >nul 2>nul
>"!GAME_DIR!\.dcr_guest_impl_revision" echo cpp38b2-metricfree-v1
echo [RESTORE] Guest cpp38d/cpp38b2 baseline restaurado. No se regenero SH-4.
exit /b 0
