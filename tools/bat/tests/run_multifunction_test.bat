@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\sh4_multifunc.elf
set OUT=generated\multifunc_native
set LOG=%OUT%\native_output.txt

if not exist "%RECOMP%" (
    echo dc_recomp.exe todavia no existe. Compilando primero...
    echo.
    call build_windows.bat --no-pause
    if errorlevel 1 goto :error
)

if not exist "%ELF%" (
    echo [ERROR] No existe %ELF%
    pause
    exit /b 2
)

if exist "%OUT%" rmdir /s /q "%OUT%"

echo ========================================
echo  DreamcastRecomp 0.0.170 - Multi Function
echo ========================================
echo.
echo Call graph esperado:
echo   _main --JSR--^> _helper --BSR--^> _leaf
echo   _leaf devuelve 41; _helper suma 1; resultado final R0=42.
echo.

echo [1/4] Descubriendo funciones y generando C++
"%RECOMP%" "%ELF%" --function _main --output "%OUT%"
if errorlevel 1 goto :error

echo.
echo [2/4] Configurando proyecto generado
cmake -S "%OUT%" -B "%OUT%\build" -A x64
if errorlevel 1 goto :error

echo.
echo [3/4] Compilando x64 Release
cmake --build "%OUT%\build" --config Release
if errorlevel 1 goto :error

echo.
echo [4/4] Ejecutando las tres funciones recompiladas
"%OUT%\build\Release\dreamcast_program.exe" > "%LOG%" 2>&1
set RUNERR=%ERRORLEVEL%
type "%LOG%"
if not "%RUNERR%"=="0" goto :error
findstr /C:"Reachable functions: 3" "%LOG%" >nul || goto :wrong_result
findstr /C:"R0=42" "%LOG%" >nul || goto :wrong_result
findstr /C:"PC=0xFFFFFFFF" "%LOG%" >nul || goto :wrong_result

echo.
echo [OK] Multi-function recompilation confirmada:
echo      _main -^> _helper -^> _leaf, resultado R0=42.
pause
exit /b 0

:wrong_result
echo.
echo [ERROR] El runner termino, pero la salida no contiene el resultado esperado.
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo alguna etapa del test multi-funcion 0.0.170.
pause
exit /b 1
