@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ANALYZER=build\Release\dc_analyzer.exe
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\hello.elf

if not exist "%ANALYZER%" (
    echo DreamcastRecomp todavia no esta compilado. Compilando primero...
    echo.
    call build_windows.bat --no-pause
    if errorlevel 1 exit /b 1
)

if not exist "%ELF%" (
    echo [ERROR] No existe %ELF%
    echo Copia tu hello.elf real de KallistiOS dentro de la carpeta samples.
    echo.
    pause
    exit /b 2
)

echo.
echo ========================================
echo  DreamcastRecomp 0.0.170 - KOS hello.elf
echo ========================================
echo.

echo [1/4] Function analysis
"%ANALYZER%" "%ELF%" --analyze-function _main
if errorlevel 1 goto :error

echo.
echo [2/4] Basic Blocks + CFG
"%ANALYZER%" "%ELF%" --cfg _main
if errorlevel 1 goto :error

echo.
echo [3/4] DCIR
"%ANALYZER%" "%ELF%" --emit-ir _main
if errorlevel 1 goto :error

echo.
echo [4/4] Emitiendo C++
if exist "generated\hello_main" rmdir /s /q "generated\hello_main"
"%RECOMP%" "%ELF%" --function _main --output "generated\hello_main"
if errorlevel 1 goto :error

echo.
echo [OK] Las cuatro etapas terminaron correctamente.
echo Para compilar Y EJECUTAR el _main como x64 nativo ejecuta:
echo   run_native_hello.bat
echo.
pause
exit /b 0

:error
echo.
echo [ERROR] Una de las etapas fallo.
pause
exit /b 1
