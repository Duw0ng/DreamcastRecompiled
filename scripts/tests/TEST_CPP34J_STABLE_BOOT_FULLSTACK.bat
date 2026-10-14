@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34j STABLE BOOT + LAZY FULLSTACK test

echo ============================================================
echo DreamcastRecomp v1.2 cpp34j STABLE_BOOT_LAZY_FULLSTACK_ABI1
echo ============================================================
echo Base guest/emitter cpp34g + cache recovery + lazy Flycast-style modem.
echo.
if "%~1"=="" (
  echo Uso:
  echo   TEST_CPP34J_STABLE_BOOT_FULLSTACK.bat "C:\ruta\al\ccr.cdi"
  pause
  exit /b 1
)

echo [INFO] CDI: "%~f1"
echo [INFO] Si el cache fue generado por cpp34h, se regenerara UNA sola vez automaticamente.
echo [INFO] El plugin de red queda dormido hasta el primer MMIO del modem.
echo.
call "%~dp0..\windows\run_game.bat" "%~f1" --rebuild-runtime
set "RC=%ERRORLEVEL%"
echo.
echo ============================================================
echo cpp34j termino con RC=%RC%
echo Busca primero:
echo   build=v1.2-cpp34j-stable-boot-lazy-fullstack
echo   pvr-fast=1/1 ... pvr-mt=on ... pvr-gpu=on
echo Luego, al entrar a Homepage/Online:
echo   [MODEM] lazy hardware activation on first MMIO access
echo   [MODEM] DISCONNECTED -^> DIALING ... -^> CONNECTED
echo   [PPP] IPCP
echo ============================================================
pause
exit /b %RC%
