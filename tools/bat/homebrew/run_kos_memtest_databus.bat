@echo off
setlocal EnableExtensions
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\memtest32.elf
set OUT=generated\kos_memtest_databus
set GENLOG=generated\kos_memtest_databus_codegen.txt
set RUNLOG=%OUT%\native_output.txt

if not exist "%RECOMP%" (
    echo dc_recomp.exe todavia no existe. Compilando primero...
    echo.
    call build_windows.bat --no-pause
    if errorlevel 1 goto :error
)

if not exist "%ELF%" (
    echo [ERROR] No existe %ELF%
    echo.
    echo Copia el memtest32.elf oficial de KallistiOS a:
    echo   samples\memtest32.elf
    echo.
    echo El ejemplo esta en KallistiOS:
    echo   examples\dreamcast\basic\memtest32
    pause
    exit /b 2
)

if exist "%OUT%" rmdir /s /q "%OUT%"
if not exist "generated" mkdir "generated"
if exist "%GENLOG%" del /q "%GENLOG%"

echo ==================================================
echo  DreamcastRecomp 0.0.170 - KOS memTestDataBus EXP
echo ==================================================
echo.
echo Funcion real de KallistiOS a probar:
echo   _memTestDataBus
echo.
echo Se inicializa R4 con una direccion de RAM controlada:
echo   R4 = 0x8C100000
echo.
echo Exito esperado:
echo   R0=0
echo   PC=0xFFFFFFFF
echo.

echo [1/4] Recompilando _memTestDataBus desde memtest32.elf
"%RECOMP%" "%ELF%" --function _memTestDataBus --output "%OUT%" > "%GENLOG%" 2>&1
set GENERR=%ERRORLEVEL%
type "%GENLOG%"
if not "%GENERR%"=="0" goto :error

findstr /C:"RAW_SH4:              0" "%GENLOG%" >nul
if errorlevel 1 goto :unsupported

echo.
echo [2/4] Configurando proyecto C++ generado
cmake -S "%OUT%" -B "%OUT%\build" -A x64
if errorlevel 1 goto :error

echo.
echo [3/4] Compilando x64 Release
cmake --build "%OUT%\build" --config Release
if errorlevel 1 goto :error

echo.
echo [4/4] Ejecutando la funcion KOS real con R4=0x8C100000
"%OUT%\build\Release\dreamcast_program.exe" --r4 0x8C100000 > "%RUNLOG%" 2>&1
set RUNERR=%ERRORLEVEL%
type "%RUNLOG%"
if not "%RUNERR%"=="0" goto :runtime_failure
findstr /C:"R0=0" "%RUNLOG%" >nul || goto :wrong_result
findstr /C:"PC=0xFFFFFFFF" "%RUNLOG%" >nul || goto :wrong_result

echo.
echo [OK] _memTestDataBus de KallistiOS se ejecuto nativamente y devolvio 0.
pause
exit /b 0

:unsupported
echo.
echo [INFO] Este memtest32.elf contiene una o mas instrucciones SH-4 que esta ruta de diagnostico puede no bajar aun a DCIR en 0.0.170.
echo        Eso es un resultado util: comparte el contenido de:
echo        %GENLOG%
echo        y esas instrucciones pasan a ser el objetivo de la siguiente version.
pause
exit /b 4

:runtime_failure
echo.
echo [INFO] La funcion se genero y compilo, pero fallo durante la ejecucion.
echo        Comparte %RUNLOG% y %GENLOG% para aislar el siguiente patron real de KallistiOS.
pause
exit /b 5

:wrong_result
echo.
echo [INFO] La funcion termino, pero no devolvio R0=0 / PC=0xFFFFFFFF.
echo        Comparte %RUNLOG% para revisar la semantica SH-4 implicada.
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo alguna etapa del test experimental KallistiOS memTestDataBus.
pause
exit /b 1
