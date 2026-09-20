@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set EXE=build\Release\dc_analyzer.exe

if not exist "%EXE%" (
    echo dc_analyzer.exe todavia no existe. Compilando primero...
    echo.
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

echo.
echo ========================================
echo  DreamcastRecomp - SH4 Smoke Test
echo ========================================
echo.

"%EXE%" samples\sh4_smoke.elf --disassemble --from-entry --max-instructions 13 --stats --functions

echo.
echo Resultado esperado:
echo   Total:    13
echo   Known:    13
echo   Unknown:  0
echo   Coverage: 100.00%%
echo.
pause
