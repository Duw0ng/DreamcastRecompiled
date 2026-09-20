@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\sub.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Usa el sub.elf oficial de KallistiOS examples\dreamcast\basic\exec\sub.elf
  echo o pasa su ruta como primer argumento.
  exit /b 1
)

echo ========================================
echo  DreamcastRecomp 0.0.170 - KOS exec/sub
echo ========================================
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\kos_exec_sub rmdir /s /q generated\kos_exec_sub
if exist generated\kos_exec_sub-build rmdir /s /q generated\kos_exec_sub-build

build\Release\dc_recomp.exe "%ELF%" --function _main --output generated\kos_exec_sub
if errorlevel 1 exit /b 1
cmake -S generated\kos_exec_sub -B generated\kos_exec_sub-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\kos_exec_sub-build --config Release
if errorlevel 1 exit /b 1

generated\kos_exec_sub-build\Release\dreamcast_program.exe > generated\kos_exec_sub\native_output.txt
set RC=%ERRORLEVEL%
type generated\kos_exec_sub\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"Hello world from sub.bin" generated\kos_exec_sub\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\kos_exec_sub\native_output.txt >nul || exit /b 1

echo.
echo [OK] KallistiOS exec/sub.elf recompilado y ejecutado nativamente en Windows.
exit /b 0
