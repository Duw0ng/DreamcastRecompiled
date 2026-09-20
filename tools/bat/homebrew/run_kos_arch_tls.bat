@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\hello.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  echo Copia el hello.elf de KallistiOS a samples\hello.elf o pasa su ruta.
  exit /b 1
)

echo ========================================
echo  DreamcastRecomp 0.0.170 - KOS arch_tls_init
echo ========================================
echo.
echo Este test ejecuta _arch_tls_init + _thd_get_current reales.
echo Se inyecta un kthread_t minimo controlado y se espera GBR=0x8C123456.
echo.

if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1
if exist generated\kos_arch_tls rmdir /s /q generated\kos_arch_tls
if exist generated\kos_arch_tls-build rmdir /s /q generated\kos_arch_tls-build

build\Release\dc_recomp.exe "%ELF%" --function _arch_tls_init --output generated\kos_arch_tls
if errorlevel 1 exit /b 1
cmake -S generated\kos_arch_tls -B generated\kos_arch_tls-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\kos_arch_tls-build --config Release
if errorlevel 1 exit /b 1

rem KOS 2.2.1 corpus layout used by the supplied hello.elf:
rem _thd_current = 0x8C05E3EC. Mock thread at 0x8C100100, TLS field at +8.
generated\kos_arch_tls-build\Release\dreamcast_program.exe --mem32=0x8C05E3EC:0x8C100100 --mem32=0x8C100108:0x8C123456 > generated\kos_arch_tls\native_output.txt
set RC=%ERRORLEVEL%
type generated\kos_arch_tls\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"GBR=0x8C123456" generated\kos_arch_tls\native_output.txt >nul || exit /b 1
findstr /C:"PC=0xFFFFFFFF" generated\kos_arch_tls\native_output.txt >nul || exit /b 1

echo.
echo [OK] _arch_tls_init real de KallistiOS cargo GBR desde el TLS del hilo mock.
exit /b 0
