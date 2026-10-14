@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34n V42 RAW-FLAG ONLINE TEST
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\to\ccr.cdi"
  exit /b 2
)
echo ============================================================
echo  DreamcastRecomp cpp34n - V42 RAW-FLAG ONLINE TEST
echo ============================================================
echo  Reuses the existing ABI1 ChuChu guest. No guest recompile.
echo  Expected path:
echo    ODP -^> ADP EC -^> raw HDLC flag -^> SABME/UA -^> PPP
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
