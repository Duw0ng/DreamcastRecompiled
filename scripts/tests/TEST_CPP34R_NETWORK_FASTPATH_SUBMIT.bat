@echo off
setlocal EnableExtensions
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
echo  DreamcastRecomp cpp34r - NETWORK FASTPATH + SUBMIT TRACE
echo ============================================================
echo  Guest ABI1 existente: NO recompila las 5185 funciones.
echo  Rebuild: DreamcastRuntime + dcr_network solamente/relink.
echo  START online: buscar [SUBMIT TRACE] en el log.
echo ============================================================
if exist "%GAME_DIR%\build\plugins\Release\dcr_guest_patch.dll" goto :run
if exist "%GAME_DIR%\build\plugins\Release\dcr_guest_patch.so" goto :run
echo [INFO] No encuentro el Late-AOT 0x8C036102. Lo genero una sola vez.
call "%~dp0..\..\scripts\patches\PATCH_CPP34R_CHUCHU_FASTPATH.bat" "%DISC%" 0x8C036102
exit /b %ERRORLEVEL%
:run
echo [INFO] Late-AOT existente detectado. Se conserva sin regenerarlo.
call "%~dp0..\windows\run_game.bat" "%DISC%" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
