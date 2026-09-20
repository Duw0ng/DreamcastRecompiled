@echo off
setlocal
cd /d "%~dp0"
where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PowerShell no esta disponible.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\controller_config\DreamcastControllerConfig.ps1" -ProfilePath "%~dp0profiles\controller_profile.ini"
