@echo off
setlocal EnableExtensions
cd /d "%~dp0..\..\"
title DreamcastRecomp cpp34k STACKSAFE HOMEPAGE TEST
if "%~1"=="" (
  echo Usage: %~nx0 "C:\path\to\ccr.cdi"
  echo.
  echo This test REUSES the existing ABI1 guest and rebuilds only runtime/plugin/runner.
  echo It will abort instead of recompiling ChuChu if the ABI1 guest cache is missing.
  pause
  exit /b 2
)
echo ============================================================
echo  DreamcastRecomp cpp34k - STACKSAFE IP.BIN REENTRY
 echo ============================================================
echo  Guest SH-4 recompilation: FORBIDDEN ^(--reuse^)
echo  Expected on Homepage chain-load:
echo    [DCR IPBIN] confirmed reentry ... live-pages-preserved=1+
echo  Then NO target 0x41474553 / "SEGA" crash.
echo  Network remains dormant until real modem MMIO.
echo ============================================================
call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --rebuild-runtime
exit /b %errorlevel%
