@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set ELF=samples\2ndmix.elf
if not "%~1"=="" set ELF=%~1
if not exist "%ELF%" (
  echo [ERROR] No encuentro %ELF%
  exit /b 1
)
if not exist build\Release\dc_recomp.exe call build_windows.bat
if errorlevel 1 exit /b 1
if exist generated\2ndmix_crc rmdir /s /q generated\2ndmix_crc
if exist generated\2ndmix_crc-build rmdir /s /q generated\2ndmix_crc-build
build\Release\dc_recomp.exe "%ELF%" --function _net_crc16ccitt --output generated\2ndmix_crc > generated\2ndmix_crc_codegen.txt
if errorlevel 1 exit /b 1
findstr /C:"RAW_SH4:              0" generated\2ndmix_crc_codegen.txt >nul || exit /b 2
cmake -S generated\2ndmix_crc -B generated\2ndmix_crc-build -A x64
if errorlevel 1 exit /b 1
cmake --build generated\2ndmix_crc-build --config Release
if errorlevel 1 exit /b 1
generated\2ndmix_crc-build\Release\dreamcast_program.exe --r4=0x8C100000 --r5=9 --r6=0xFFFF --memstr=0x8C100000:123456789 > generated\2ndmix_crc\native_output.txt
set RC=%ERRORLEVEL%
type generated\2ndmix_crc\native_output.txt
if not "%RC%"=="0" exit /b %RC%
findstr /C:"R0=10673" generated\2ndmix_crc\native_output.txt >nul || exit /b 3
findstr /C:"PC=0xFFFFFFFF" generated\2ndmix_crc\native_output.txt >nul || exit /b 3
echo.
echo [OK] KallistiOS net_crc16ccitt("123456789", 0xFFFF) = 0x29B1.
exit /b 0
