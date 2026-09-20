@echo off
setlocal EnableExtensions
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\memtest32.elf
set OUT=generated\kos_memtest_device
set GENLOG=generated\kos_memtest_device_codegen.txt
set RUNLOG=%OUT%\native_output.txt

if not exist "%RECOMP%" (
    echo dc_recomp.exe todavia no existe. Compilando primero...
    call build_windows.bat --no-pause
    if errorlevel 1 goto :error
)

if not exist "%ELF%" (
    echo [ERROR] No existe %ELF%
    echo Copia memtest32.elf de KallistiOS a samples\memtest32.elf
    pause
    exit /b 2
)

if exist "%OUT%" rmdir /s /q "%OUT%"
if not exist "generated" mkdir "generated"
if exist "%GENLOG%" del /q "%GENLOG%"

 echo =================================================
 echo  DreamcastRecomp 0.0.170 - KOS memTestDevice
 echo =================================================
 echo.
 echo Funcion real: _memTestDevice
 echo Argumentos SH-4:
 echo   R4 = 0x8C100000  ^(baseAddress^)
 echo   R5 = 0x00010000  ^(64 KiB^)
 echo.
 echo Esta rutina realiza tres pasadas de memoria e incluye NOT Rm,Rn.
 echo Exito esperado: R0=0 y PC=0xFFFFFFFF
 echo.

 echo [1/4] Recompilando _memTestDevice real
"%RECOMP%" "%ELF%" --function _memTestDevice --output "%OUT%" > "%GENLOG%" 2>&1
set GENERR=%ERRORLEVEL%
type "%GENLOG%"
if not "%GENERR%"=="0" goto :error
findstr /C:"RAW_SH4:              0" "%GENLOG%" >nul || goto :unsupported

 echo.
 echo [2/4] Configurando C++ generado
cmake -S "%OUT%" -B "%OUT%\build" -A x64
if errorlevel 1 goto :error

 echo.
 echo [3/4] Compilando x64 Release
cmake --build "%OUT%\build" --config Release
if errorlevel 1 goto :error

 echo.
 echo [4/4] Ejecutando con 64 KiB de RAM Dreamcast controlada
"%OUT%\build\Release\dreamcast_program.exe" --r4 0x8C100000 --r5 0x00010000 > "%RUNLOG%" 2>&1
set RUNERR=%ERRORLEVEL%
type "%RUNLOG%"
if not "%RUNERR%"=="0" goto :runtime_failure
findstr /C:"R0=0" "%RUNLOG%" >nul || goto :wrong_result
findstr /C:"PC=0xFFFFFFFF" "%RUNLOG%" >nul || goto :wrong_result

echo.
echo [OK] _memTestDevice de KallistiOS se ejecuto nativamente y devolvio 0.
pause
exit /b 0

:unsupported
echo.
echo [INFO] Hay SH-4 aun no bajado a DCIR. Comparte %GENLOG%
pause
exit /b 4

:runtime_failure
echo.
echo [INFO] Se genero y compilo, pero fallo ejecutando. Comparte %RUNLOG%
pause
exit /b 5

:wrong_result
echo.
echo [INFO] Termino, pero no con R0=0 / PC=0xFFFFFFFF. Comparte %RUNLOG%
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo el test KallistiOS memTestDevice.
pause
exit /b 1
