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

echo ============================================================
echo  DreamcastRecomp cpp34x - AUTO LATE-AOT ONLINE
echo ============================================================
echo  Mantiene cpp34s V42/LAPM reliability + cpp34r fastpath.
echo  Known x6: 036102 036118 036160 036236 036280 036526.
echo  Si el ultimo run crashea por otro missing target, vuelve a
echo  ejecutar ESTE MISMO BAT y lo agregara automaticamente.
echo  NO recompila las 5185 funciones de ChuChu.
echo ============================================================

call "%~dp0..\..\scripts\patches\PATCH_CPP34X_CHUCHU_AUTO_LATE_AOT.bat" "%DISC%"
exit /b %ERRORLEVEL%
