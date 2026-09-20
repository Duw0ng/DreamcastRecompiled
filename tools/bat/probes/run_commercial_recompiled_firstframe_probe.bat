@echo off
setlocal

set "DCR_ROOT=%~dp0..\..\.."
pushd "%DCR_ROOT%" >nul
set "SCRIPT_DIR=%CD%\"
set "PATH=%CD%\tools\bat\tests;%CD%\tools\bat\homebrew;%CD%\tools\bat\probes;%CD%\tools\bat\legacy;%CD%\tools\bat\dev;%PATH%"

set "DISC=%~1"
if "%DISC%"=="" set "DISC=%DCR_DISC%"
if "%DISC%"=="" (
  echo Usage: %~nx0 ^<disc.cdi^>
  echo or set DCR_DISC to the CDI path.
  popd >nul
  exit /b 1
)

set "OUTDIR=%SCRIPT_DIR%generated\commercial_recompiled_firstframe"
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set "DIRECT_GAME_ARG="
if not "%DCR_DIRECT_GAME_ENTRY%"=="" set "DIRECT_GAME_ARG=--direct-game-entry=%DCR_DIRECT_GAME_ENTRY%"

build_frame\dc_disc_probe "%DISC%" > "%OUTDIR%\disc_probe.txt" || exit /b 1
build_frame\dc_boot_prepare --disc "%DISC%" --out "%OUTDIR%" || exit /b 1
build_frame\dc_raw_recomp --input "%OUTDIR%\BOOTSTRAP.BIN" --elf "%OUTDIR%\game.elf" --output "%OUTDIR%\generated_program.cpp" --map "%OUTDIR%\function_map.csv" || exit /b 1

cmake -S . -B build_frame >nul || exit /b 1
cmake --build build_frame --config Release --target dreamcast_program || exit /b 1

build_frame\dreamcast_program --commercial-boot --disc-map="%OUTDIR%\disc.map" --device-clock --aica-arm7 --pvr-dump="%SCRIPT_DIR%daytona_firstframe.ppm" %DIRECT_GAME_ARG%
set ERR=%ERRORLEVEL%

popd >nul
exit /b %ERR%
