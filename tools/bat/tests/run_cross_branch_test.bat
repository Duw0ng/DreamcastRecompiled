@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if not exist build\Release\dc_recomp.exe (
  echo [ERROR] build\Release\dc_recomp.exe no existe. Ejecuta build_windows.bat primero.
  exit /b 1
)
if exist generated\cross_branch rmdir /s /q generated\cross_branch
build\Release\dc_recomp.exe samples\sh4_cross_branch.elf --function _main --output generated\cross_branch || exit /b 2
cmake -S generated\cross_branch -B generated\cross_branch\build -A x64 || exit /b 3
cmake --build generated\cross_branch\build --config Release || exit /b 4
generated\cross_branch\build\Release\dreamcast_program.exe > generated\cross_branch\native_output.txt
findstr /C:"R0=42" generated\cross_branch\native_output.txt >nul || exit /b 5
findstr /C:"PC=0xFFFFFFFF" generated\cross_branch\native_output.txt >nul || exit /b 6
echo [OK] Cross-symbol shared-tail branch devuelve R0=42 sin corromper el retorno.
