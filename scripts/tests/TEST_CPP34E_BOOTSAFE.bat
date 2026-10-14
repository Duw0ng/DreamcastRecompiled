@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34e BOOTSAFE IPBIN test

echo ============================================================
echo DreamcastRecomp v1.2 cpp34e BOOTSAFE_IPBIN_REENTRY_ABI1
echo ============================================================
echo Corrige la regresion cpp34d de 0x8C00FA00 durante bootstrap.
echo Fuerza runtime/plugin/runner nuevos; conserva guest SH-4 ABI v1.
echo.
if "%~1"=="" (
  echo Arrastra ccr.cdi sobre este BAT.
  pause
  exit /b 1
)

call "%~dp0..\windows\run_game.bat" "%~f1" --rebuild-runtime
set "RC=%ERRORLEVEL%"
echo.
echo ============================================================
echo cpp34e termino con RC=%RC%
echo Buscar en el log:
echo   build=v1.2-cpp34e-bootsafe-ipbin-reentry
echo   [DCR code-reloc] 0x8C00FA00 ...   ^<-- debe volver a ocurrir
echo   [DCR IPBIN] reentry guard armed   ^<-- solo despues de renderizar
echo   [DCR IPBIN] reentry detected      ^<-- solo al volver luego a IP.BIN
echo ============================================================
pause
exit /b %RC%
