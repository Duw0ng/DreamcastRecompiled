@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
rem AQUA Python/Tk, no web download. Standalone VMU: ABRIR_LAUNCHER.bat --vmu
if exist "%~dp0DreamcastRecompiled.exe" if exist "%~dp0_internal\python312.dll" (
  start "" "%~dp0DreamcastRecompiled.exe" %*
  exit /b 0
)
if exist "%~dp0.dcr_builder\Python312\python.exe" (
  "%~dp0.dcr_builder\Python312\python.exe" "%~dp0launcher\offline_host.py" %*
  exit /b %errorlevel%
)
rem Reuse a previous Aqua release's already-downloaded Python 3.12.
for /d %%D in ("%~dp0..\*") do (
  if exist "%%~fD\.dcr_builder\Python312\python.exe" (
    "%%~fD\.dcr_builder\Python312\python.exe" "%~dp0launcher\offline_host.py" %*
    exit /b !errorlevel!
  )
)
where py.exe >nul 2>nul
if not errorlevel 1 (
  py -3.12 "%~dp0launcher\offline_host.py" %*
  exit /b %errorlevel%
)
where python.exe >nul 2>nul
if not errorlevel 1 (
  python.exe "%~dp0launcher\offline_host.py" %*
  exit /b %errorlevel%
)
echo [AQUA] No se encontro el runtime offline ni Python instalado.
echo Compila una vez usando COMPILAR_LAUNCHER.bat, o descarga el artefacto
 echo Windows x64 OFFLINE generado por GitHub Actions.
pause
exit /b 2
