@echo off
setlocal
cd /d "%~dp0..\..\"
if "%~1"=="" (
  echo Uso: %~nx0 "ruta\ccr.cdi"
  exit /b 1
)
call "%~dp0..\..\scripts\patches\PATCH_CPP34Q_CHUCHU_LATE_TARGET.bat" "%~f1" 0x8C036102
exit /b %ERRORLEVEL%
