@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\sh4_structs.elf
set OUT=generated\structs_native
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
echo  DreamcastRecomp 0.0.170 - Structs / Bits
echo ========================================
echo.
echo Este test combina:
echo   MOV.B / MOV.W / MOV.L con desplazamientos
echo   direccionamiento indexado por R0
echo   EXTU / EXTS
echo   AND / XOR / OR
echo   shifts y rotaciones
echo   struct con campos byte, word y long
echo.
echo Resultado final esperado: R0=42
 echo.

echo [1/4] Analizando CFG/DCIR y generando C++
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
echo [4/4] Ejecutando struct/bitops recompilado
"%OUT%\build\Release\dreamcast_program.exe" > "%LOG%" 2>&1
set RUNERR=%ERRORLEVEL%
type "%LOG%"
if not "%RUNERR%"=="0" goto :error
findstr /C:"Reachable functions: 2" "%LOG%" >nul || goto :wrong_result
findstr /C:"R0=42" "%LOG%" >nul || goto :wrong_result
findstr /C:"PC=0xFFFFFFFF" "%LOG%" >nul || goto :wrong_result

echo.
echo [OK] Structs + mixed-width memory + bitops ejecutados nativamente: R0=42.
pause
exit /b 0

:wrong_result
echo.
echo [ERROR] El programa termino pero no produjo R0=42 / retorno correcto.
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo alguna etapa del test structs/bitops 0.0.170.
pause
exit /b 1
