@echo off
setlocal EnableExtensions
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set RECOMP=build\Release\dc_recomp.exe
set ELF=samples\sh4_not.elf
set OUT=generated\not_native
set GENLOG=generated\not_codegen.txt
set RUNLOG=%OUT%\native_output.txt

if not exist "%RECOMP%" (
    call build_windows.bat --no-pause
    if errorlevel 1 goto :error
)

if exist "%OUT%" rmdir /s /q "%OUT%"
if not exist "generated" mkdir "generated"

 echo ==============================================
 echo  DreamcastRecomp 0.0.170 - NOT native test
 echo ==============================================
 echo.
 echo _main computes: ~(-43) = 42
 echo.

"%RECOMP%" "%ELF%" --function _main --output "%OUT%" > "%GENLOG%" 2>&1
if errorlevel 1 goto :error
findstr /C:"RAW_SH4:              0" "%GENLOG%" >nul || goto :error

cmake -S "%OUT%" -B "%OUT%\build" -A x64
if errorlevel 1 goto :error
cmake --build "%OUT%\build" --config Release
if errorlevel 1 goto :error

"%OUT%\build\Release\dreamcast_program.exe" > "%RUNLOG%" 2>&1
if errorlevel 1 goto :error
type "%RUNLOG%"
findstr /C:"R0=42" "%RUNLOG%" >nul || goto :wrong
findstr /C:"PC=0xFFFFFFFF" "%RUNLOG%" >nul || goto :wrong

echo.
echo [OK] NOT Rm,Rn ejecutado nativamente: R0=42.
pause
exit /b 0

:wrong
echo.
echo [ERROR] NOT termino con un resultado inesperado.
pause
exit /b 3

:error
echo.
echo [ERROR] Fallo el test nativo de NOT.
pause
exit /b 1
