@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\fpu_exc.elf
if not "%~1"=="" set ELF=%~1

if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Usa examples\dreamcast\basic\fpu\exc\fpu_exc.elf de KallistiOS
  echo o pasa su ruta como primer argumento.
  exit /b 1
)

echo ========================================
echo  DreamcastRecomp 0.0.170 - KOS FPU underflow arithmetic path
echo ========================================
echo.
echo NOTA: este test valida recompilacion/ejecucion FPU, no aun los flags de excepcion FPSCR.
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\kos_fpu_underflow rmdir /s /q generated\kos_fpu_underflow
if exist generated\kos_fpu_underflow-build rmdir /s /q generated\kos_fpu_underflow-build

build\Release\dc_recomp.exe "%ELF%" --function _fpscr_underflow --output generated\kos_fpu_underflow
if errorlevel 1 exit /b 1
cmake -S generated\kos_fpu_underflow -B generated\kos_fpu_underflow-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\kos_fpu_underflow-build --config Release
if errorlevel 1 exit /b 1

generated\kos_fpu_underflow-build\Release\dreamcast_program.exe > generated\kos_fpu_underflow\native_output.txt
set RC=%ERRORLEVEL%
type generated\kos_fpu_underflow\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"PC=0xFFFFFFFF" generated\kos_fpu_underflow\native_output.txt >nul || exit /b 1

echo.
echo [OK] _fpscr_underflow real de KallistiOS ejecuto su ruta FPU y retorno al host.
echo      Los flags/cause de excepcion FPSCR siguen pendientes.
exit /b 0
