@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34g PRESERVE RELOC IPBIN test

echo ============================================================
echo DreamcastRecomp v1.2 cpp34g PRESERVE_RELOC_IPBIN_ABI1
echo ============================================================
echo Restaura IP.BIN sin borrar relocations vivas 0xF400/0xFA00.
echo.
if "%~1"=="" (
  echo Uso:
  echo   TEST_CPP34G_PRESERVE_RELOC.bat "C:\ruta\al\juego.cdi"
  echo.
  pause
  exit /b 1
)

echo [INFO] CDI: "%~f1"
echo [INFO] Forzando rebuild del runtime/plugin/runner cpp34g...
echo.
call "%~dp0..\windows\run_game.bat" "%~f1" --rebuild-runtime
set "RC=%ERRORLEVEL%"

echo.
echo ============================================================
echo cpp34g termino con RC=%RC%
echo Busca en el log:
echo   build=v1.2-cpp34g-preserve-reloc-ipbin
echo   [DCR IPBIN] confirmed reentry ... reloc-aliases-preserved=2
echo   [DCR IPBIN] reentry suppressed ... 0x8C00FA00
echo ============================================================
pause
exit /b %RC%
