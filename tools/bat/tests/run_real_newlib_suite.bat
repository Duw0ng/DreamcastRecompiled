@echo off
setlocal EnableDelayedExpansion
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

for %%F in (_strlen _strcmp _memmove) do (
  set NAME=%%F
  set NAME=!NAME:_=!
  if exist generated\newlib_!NAME! rmdir /s /q generated\newlib_!NAME!
  if exist generated\newlib_!NAME!-build rmdir /s /q generated\newlib_!NAME!-build
  build\Release\dc_recomp.exe "%ELF%" --function %%F --output generated\newlib_!NAME! > generated\newlib_!NAME!_codegen.txt
  if errorlevel 1 exit /b 1
  cmake -S generated\newlib_!NAME! -B generated\newlib_!NAME!-build -A x64 >nul
  if errorlevel 1 exit /b 1
  cmake --build generated\newlib_!NAME!-build --config Release >nul
  if errorlevel 1 exit /b 1
)

generated\newlib_strlen-build\Release\dreamcast_program.exe --r4=0x8C100000 --memstr=0x8C100000:Dreamcast > generated\newlib_strlen\native_output.txt
findstr /C:"R0=9" generated\newlib_strlen\native_output.txt >nul || exit /b 3

generated\newlib_strcmp-build\Release\dreamcast_program.exe --r4=0x8C100000 --r5=0x8C100100 --memstr=0x8C100000:Dreamcast --memstr=0x8C100100:Dreamcast > generated\newlib_strcmp\native_output.txt
findstr /C:"R0=0" generated\newlib_strcmp\native_output.txt >nul || exit /b 3

generated\newlib_memmove-build\Release\dreamcast_program.exe --r4=0x8C100002 --r5=0x8C100000 --r6=6 --memstr=0x8C100000:ABCDEFGH --peek32=0x8C100000 --peek32=0x8C100004 > generated\newlib_memmove\native_output.txt
findstr /C:"[PEEK32] 0x8C100000 = 0x42414241" generated\newlib_memmove\native_output.txt >nul || exit /b 3
findstr /C:"[PEEK32] 0x8C100004 = 0x46454443" generated\newlib_memmove\native_output.txt >nul || exit /b 3

type generated\newlib_strlen\native_output.txt
type generated\newlib_strcmp\native_output.txt
type generated\newlib_memmove\native_output.txt
echo.
echo [OK] newlib real: strlen, strcmp y memmove con overlap pasan.
exit /b 0
