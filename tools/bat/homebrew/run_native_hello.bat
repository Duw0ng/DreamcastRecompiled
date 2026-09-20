@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\hello.elf
set OUT=generated\hello_native

if not exist "%RECOMP%" (
    echo dc_recomp.exe todavia no existe. Compilando DreamcastRecomp primero...
    echo.
    call build_windows.bat --no-pause
    if errorlevel 1 goto :error
)

if not exist "%ELF%" (
    echo [ERROR] No existe %ELF%
    echo Copia tu hello.elf real de KallistiOS dentro de samples.
    pause
    exit /b 2
)

if exist "%OUT%" rmdir /s /q "%OUT%"

echo ========================================
echo  DreamcastRecomp 0.0.170 - Native Hello
echo ========================================
echo.

echo [1/5] SH-4 / CFG / DCIR -^> C++ nativo
"%RECOMP%" "%ELF%" --function _main --output "%OUT%"
if errorlevel 1 goto :error

echo.
echo [2/5] Configurando proyecto C++ generado
cmake -S "%OUT%" -B "%OUT%\build" -A x64
if errorlevel 1 goto :error

echo.
echo [3/5] Compilando proyecto generado como x64 Release
cmake --build "%OUT%\build" --config Release
if errorlevel 1 goto :error

echo.
echo [4/5] Smoke test del runtime generado
"%OUT%\build\Release\generated_compile_test.exe"
if errorlevel 1 goto :error

echo.
echo [5/5] EJECUTANDO _main RECOMPILADO
echo ----------------------------------------
"%OUT%\build\Release\dreamcast_program.exe"
if errorlevel 1 goto :error
echo ----------------------------------------

echo.
echo [OK] hello.elf::_main fue convertido de SH-4 a C++, compilado como x64
echo y ejecutado usando el native override de _printf.
echo.
echo El texto mostrado por _printf se lee de la .rodata original de hello.elf.
echo No esta hardcodeado en el runner.
echo.
echo Ejecutable nativo generado:
echo   %CD%\%OUT%\build\Release\dreamcast_program.exe
pause
exit /b 0

:error
echo.
echo [ERROR] Fallo alguna etapa del pipeline nativo 0.0.170.
echo Si dreamcast_program.exe alcanzo a iniciar, copia toda su salida para revisar
echo la direccion/operacion exacta que fallo.
pause
exit /b 1
