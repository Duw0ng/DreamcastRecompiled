@echo off
setlocal EnableExtensions
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\build_offline_host.ps1" %*
if errorlevel 1 (
  echo [ERROR] Host build failed. Review the diagnostics above.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\package_offline_release.ps1"
if errorlevel 1 (
  echo [ERROR] The host built successfully, but the offline ZIP could not be packaged.
  pause
  exit /b 1
)
echo [OK] The offline Windows host and ZIP were generated. Start with OPEN_LAUNCHER.bat.
pause
exit /b 0
