@echo off
setlocal
cd /d "%~dp0"

set "NOPAUSE=0"
if /I "%~1"=="--no-pause" set "NOPAUSE=1"

echo ========================================
echo  DreamcastRecomp v0.1.1 Official - Build
echo ========================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake no esta instalado o no esta en PATH.
    echo Instala Visual Studio 2022 o Build Tools 2022 con:
    echo   - Desktop development with C++
    echo   - CMake tools for Windows
    if "%NOPAUSE%"=="0" pause
    exit /b 1
)

echo [1/3] Configurando CMake x64...
cmake -S . -B build -A x64
if errorlevel 1 goto :build_error

echo.
echo [2/3] Compilando DreamcastRecomp Release...
echo [BUILD] Modo de bajo consumo: un trabajo de compilacion a la vez.
cmake --build build --config Release --parallel 1
if errorlevel 1 goto :build_error

echo.
echo [3/3] Ejecutando tests...
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 (
    echo.
    echo [WARN] La compilacion termino, pero uno o mas tests fallaron.
    if "%NOPAUSE%"=="0" pause
    exit /b 2
)

echo.
echo [OK] Compilacion completada y tests aprobados.
echo.
echo Ejecutables principales:
echo   %CD%\build\Release\dc_analyzer.exe
echo   %CD%\build\Release\dc_recomp.exe
echo   %CD%\build\Release\dc_corpus_scan.exe
echo   %CD%\build\Release\dc_disc_probe.exe
echo   %CD%\build\Release\dc_raw_boot_probe.exe
echo   %CD%\build\Release\dc_boot_prepare.exe
echo   %CD%\build\Release\dc_raw_recomp.exe
echo.
if "%NOPAUSE%"=="0" pause
exit /b 0

:build_error
echo.
echo [ERROR] No se pudo compilar DreamcastRecomp.
echo Asegurate de tener Visual Studio 2022 / Build Tools con C++ instalado.
echo Si reutilizaste una carpeta build creada con otro generador, elimina la carpeta build y vuelve a ejecutar este BAT.
if "%NOPAUSE%"=="0" pause
exit /b 1
