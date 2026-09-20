@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\sh4_memory.elf
set OUT=generated\memory_native
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
echo  DreamcastRecomp 0.0.170 - Memory / Arrays
echo ========================================
echo.
echo Programa esperado:
echo   .data = {10,20,30,40,50}
echo   _main --BSR--^> _sum_array
echo   _sum_array usa MOV.L @R4+,R2 + DT/BF
echo   _main guarda el resultado en .bss y lo vuelve a leer
echo   resultado final: R0=150
echo.

echo [1/4] Analizando memoria/CFG/DCIR y generando C++
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
echo [4/4] Ejecutando acceso a RAM recompilado
"%OUT%\build\Release\dreamcast_program.exe" > "%LOG%" 2>&1
set RUNERR=%ERRORLEVEL%
type "%LOG%"
if not "%RUNERR%"=="0" goto :error
findstr /C:"Reachable functions: 2" "%LOG%" >nul || goto :wrong_result
findstr /C:"R0=150" "%LOG%" >nul || goto :wrong_result
findstr /C:"PC=0xFFFFFFFF" "%LOG%" >nul || goto :wrong_result

echo.
echo [OK] Array + RAM + load/store + post-increment ejecutados nativamente: R0=150.
pause
exit /b 0

:wrong_result
echo.
echo [ERROR] El programa termino pero no produjo R0=150 / retorno correcto.
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo alguna etapa del test de memoria 0.0.170.
pause
exit /b 1
