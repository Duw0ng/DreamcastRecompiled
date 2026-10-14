@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34p TCP BRIDGE ONLINE TEST
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\to\ccr.cdi"
  exit /b 2
)
echo ============================================================
echo  DreamcastRecomp cpp34p - IPV4 LENGTH + TCP PAYLOAD FIX TEST
echo ============================================================
echo  Reuses the existing ABI1 ChuChu guest. No guest recompile.
echo  Focus: TCP :9000 request/response after PPP/IPCP/DNS.
echo  Look for TCP OPEN / DC-TX / HOST-TX / HOST-RX / DC-RX.
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %ERRORLEVEL%
