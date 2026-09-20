@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\micropython.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  exit /b 1
)
if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1
if exist generated\micropython_main rmdir /s /q generated\micropython_main
if exist generated\micropython_main-build rmdir /s /q generated\micropython_main-build
build\Release\dc_recomp.exe "%ELF%" --function _main --max-functions 1024 --output generated\micropython_main > generated\micropython_main_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\micropython_main_codegen.txt >nul || exit /b 2
cmake -S generated\micropython_main -B generated\micropython_main-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\micropython_main-build --config Release
if errorlevel 1 exit /b 1
generated\micropython_main-build\Release\dreamcast_program.exe > generated\micropython_main\native_output.txt 2>&1
set RC=%ERRORLEVEL%
type generated\micropython_main\native_output.txt
findstr /C:"(entering script)" generated\micropython_main\native_output.txt >nul || exit /b 3
echo.
echo [DIAGNOSTIC] MicroPython ya entra al script con RAW_SH4=0, pero esta ruta de diagnostico aun
echo              encuentra un puntero guest invalido dentro de _qstr_str.
echo              No se clasifica como hardware: queda como objetivo CPU/runtime real.
exit /b 0
