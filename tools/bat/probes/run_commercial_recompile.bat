@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
if "%~1"=="" (
  echo Uso: run_commercial_recompile.bat "ruta\juego.cdi"
  echo.
  echo 0.0.205 reproduce el arranque real de consola y amplia la closure comercial sin RAW_SH4: extrae IP.BIN y el boot,
  echo detecta GD-ROM vs selfboot, prepara la imagen 0x8C008000 y recompila
  echo tanto el bootstrap como la entrada comercial 0x8C010000.
  echo Los datos comerciales quedan solo en generated\ y nunca se redistribuyen.
  exit /b 1
)

set "DISC=%~f1"
if not exist "%DISC%" (
  echo [ERROR] No encuentro "%DISC%"
  exit /b 1
)

rem Always update the tools: an existing EXE may belong to an older generator.
cmake -S . -B build -A x64
if errorlevel 1 exit /b 2
cmake --build build --config Release --target dc_disc_probe dc_boot_prepare dc_raw_recomp --parallel 1
if errorlevel 1 exit /b 2
for %%T in (dc_disc_probe dc_boot_prepare dc_raw_recomp) do (
  if not exist "build\Release\%%T.exe" (
    echo [ERROR] Falta build\Release\%%T.exe despues del build.
    exit /b 2
  )
)

if not exist generated mkdir generated
if /I "%DCR_CLEAN_RECOMPILE%"=="1" (
  echo [BUILD] DCR_CLEAN_RECOMPILE=1: eliminando cache comercial anterior...
  if exist generated\commercial_recompiled rmdir /s /q generated\commercial_recompiled
  if exist _cb rmdir /s /q _cb
)
if not exist generated\commercial_recompiled mkdir generated\commercial_recompiled

echo [BUILD] Modo incremental activo: se conserva _cb y los objetos sin cambios.

build\Release\dc_disc_probe.exe "%DISC%" ^
  --extract-ip=generated\commercial_recompiled\IP.BIN ^
  --extract-boot=generated\commercial_recompiled\BOOT.DISC.BIN ^
  --extract-disc-map=generated\commercial_recompiled\disc.map
if errorlevel 1 exit /b 3

echo.
echo [DreamcastRecomp 0.0.205] Preparando boot de consola
build\Release\dc_boot_prepare.exe ^
  generated\commercial_recompiled\IP.BIN ^
  generated\commercial_recompiled\BOOT.DISC.BIN ^
  --mode=auto ^
  --boot-out=generated\commercial_recompiled\BOOT.BIN ^
  --combined-out=generated\commercial_recompiled\BOOTSTRAP.BIN
if errorlevel 1 exit /b 4

echo.
echo [DreamcastRecomp 0.0.205] Recompilacion comercial IP.BIN + juego
build\Release\dc_raw_recomp.exe generated\commercial_recompiled\BOOTSTRAP.BIN ^
  --base=0x8C008000 ^
  --entry=0x8C008300 ^
  --seed=0x8C010000 ^
  --max-functions=8192 ^
  --max-closure-entries=6144 ^
  --closure-passes=64 ^
  --map=generated\commercial_recompiled\function_map.csv ^
  --output=generated\commercial_recompiled\cpp
if errorlevel 1 exit /b 5

echo.
echo [INFO] Compilando el C++ generado en modo de bajo consumo...
cmake -S generated\commercial_recompiled\cpp -B _cb -A x64
if errorlevel 1 exit /b 6
echo [BUILD] Modo serial: 1 archivo a la vez, MSVC /MP desactivado.
echo [BUILD] Se mostrara una barra de progreso aproximada durante la compilacion.
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_progress.ps1 ^
  -BuildDir "_cb" ^
  -SourceDir "generated\commercial_recompiled\cpp" ^
  -Config "Release" ^
  -Target "dreamcast_program"
if errorlevel 1 exit /b 7

echo.
echo [OK] Bootstrap y cierre comercial recompilados y compilados incrementalmente.
echo [INFO] Ejecutable: _cb\Release\dreamcast_program.exe
exit /b 0
