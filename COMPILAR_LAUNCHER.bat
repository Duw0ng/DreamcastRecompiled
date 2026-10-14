@echo off
setlocal EnableExtensions
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\build_offline_host.ps1" %*
if errorlevel 1 (
  echo [ERROR] Fallo la construccion del host. Revisa el diagnostico arriba.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\package_offline_release.ps1"
if errorlevel 1 (
  echo [ERROR] El host se construyo, pero el ZIP offline no se pudo empaquetar.
  pause
  exit /b 1
)
echo [OK] Host y ZIP Windows offline generados. Abrir con ABRIR_LAUNCHER.bat.
pause
exit /b 0
