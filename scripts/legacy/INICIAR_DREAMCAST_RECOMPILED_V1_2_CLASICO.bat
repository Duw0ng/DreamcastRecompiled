@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0..\.."
title Dreamcast Recompiled v1.2
echo ============================================================
echo                Dreamcast Recompiled v1.2
echo                 Stable: cpp38e HOT BLOCK
echo ============================================================
if "%~1"=="" goto :uso
echo [STABLE] Ejecutando cpp38e con HOT BLOCK RECOVERY.
echo [STABLE] Sin profiler HOTPC, sin recompilar el guest si ya esta listo.
echo.
call "%CD%\scripts\stable\TEST_CPP38E_FAST.bat" %*
set "RC=%errorlevel%"
if not "%RC%"=="0" (
  echo.
  echo [AVISO] Dreamcast Recompiled v1.2 termino con codigo %RC%.
  echo [INFO] No se borro ni reemplazo tu cache existente.
)
exit /b %RC%
:uso
echo.
echo Para iniciar un juego, arrastra tu archivo .cdi sobre este .bat.
echo Tambien puedes ejecutarlo desde PowerShell:
echo   .\INICIAR_DREAMCAST_RECOMPILED_V1_2.bat "C:\ruta\ccr.cdi"
echo.
echo Esta distribucion es la cpp38e original; sin cambios de cpp38ay-cpp38ba.
echo.
pause
exit /b 1
