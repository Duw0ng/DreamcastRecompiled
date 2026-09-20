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
echo  DreamcastRecomp 0.0.170 - KOS ieee754 sqrtf
echo ========================================
echo.
echo Ejecutando ___ieee754_sqrtf con FR5=9.0. Resultado esperado: FR0=3.
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1

if exist generated\kos_ieee_sqrtf rmdir /s /q generated\kos_ieee_sqrtf
if exist generated\kos_ieee_sqrtf-build rmdir /s /q generated\kos_ieee_sqrtf-build

build\Release\dc_recomp.exe "%ELF%" --function ___ieee754_sqrtf --output generated\kos_ieee_sqrtf
if errorlevel 1 exit /b 1
cmake -S generated\kos_ieee_sqrtf -B generated\kos_ieee_sqrtf-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\kos_ieee_sqrtf-build --config Release
if errorlevel 1 exit /b 1

generated\kos_ieee_sqrtf-build\Release\dreamcast_program.exe --fr5=9.0 > generated\kos_ieee_sqrtf\native_output.txt
set RC=%ERRORLEVEL%
type generated\kos_ieee_sqrtf\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"FR0=3" generated\kos_ieee_sqrtf\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\kos_ieee_sqrtf\native_output.txt >nul || exit /b 1

echo.
echo [OK] ___ieee754_sqrtf real de KallistiOS: sqrtf(9.0) -^> FR0=3.
exit /b 0
