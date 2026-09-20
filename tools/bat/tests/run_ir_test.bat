@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set EXE=build\Release\dc_analyzer.exe
set ELF=samples\sh4_literal_pool.elf

if not exist "%EXE%" (
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

echo.
echo ========================================
echo  DreamcastRecomp 0.0.170 - DCIR test
echo ========================================
echo.
"%EXE%" "%ELF%" --emit-ir _main
pause
