@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo  DreamcastRecomp v0.1.1 Official - System Check
echo ============================================================
set "FAIL=0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo [FAIL] CMake no esta en PATH.
  set "FAIL=1"
) else (
  for /f "delims=" %%V in ('cmake --version ^| findstr /B /C:"cmake version"') do echo [ OK ] %%V
)
where powershell >nul 2>nul
if errorlevel 1 (echo [WARN] PowerShell no encontrado.) else (echo [ OK ] PowerShell disponible.)
where git >nul 2>nul
if errorlevel 1 (echo [INFO] Git no es obligatorio para ejecutar una release ZIP.) else (echo [ OK ] Git disponible.)
if exist "profiles\controller_profile.ini" (echo [ OK ] Perfil de mando personalizado encontrado.) else (echo [INFO] Perfil de mando default; usa Configurar_Mando.bat para personalizar.)
if "%FAIL%"=="1" (
  echo.
  echo Instala Visual Studio 2022 o Build Tools 2022 con:
  echo   - Desktop development with C++
  echo   - CMake tools for Windows
  exit /b 1
)
echo.
echo [OK] Entorno basico listo. Siguiente paso:
echo      run_game.bat "C:\ruta\juego.cdi"
exit /b 0
