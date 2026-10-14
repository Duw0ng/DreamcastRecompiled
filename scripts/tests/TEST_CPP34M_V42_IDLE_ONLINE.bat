@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34m V42/LAPM ONLINE TEST

if "%~1"=="" (
  echo Uso: %~nx0 "C:\ruta\juego.cdi"
  exit /b 2
)

echo ============================================================
echo  DreamcastRecomp cpp34m - V42/LAPM PPP ONLINE TEST
echo ============================================================
echo  - Reutiliza el guest SH-4 ABI1 ya compilado.
echo  - Recompila solo runtime/plugin.
echo  - Si no existe guest cache, aborta en vez de recompilar ChuChu.
echo  Busca en el log:
echo    [PLUGIN network] cpp34m V42/LAPM FULLSTACK
echo    [MODEM] PRE_CONNECTED -^> CONNECTED
echo    [NET] modem carrier online ^| transport=V42/LAPM
echo    [V42] ODP detected -^> ADP EC
echo    [V42] SABME -^> UA ^| LAPM connected
echo    [PPP] IPCP
echo.

call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
