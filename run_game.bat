@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "DCR_VERSION=v0.1.1 Official"
set "PERF=0"
set "DEBUG=0"
set "PVR_PROFILE=0"
set "SOFTWARE=0"
set "NO_AUDIO=0"
set "CLEAN=0"
set "NOPAUSE=0"

if "%~1"=="" goto :usage
if /I "%~1"=="--help" goto :usage_ok
if /I "%~1"=="-h" goto :usage_ok

set "DISC=%~f1"
shift

:parse_args
if "%~1"=="" goto :args_done
if /I "%~1"=="--perf" (
  set "PERF=1"
) else if /I "%~1"=="--debug" (
  set "DEBUG=1"
) else if /I "%~1"=="--profile" (
  set "PVR_PROFILE=1"
) else if /I "%~1"=="--software" (
  set "SOFTWARE=1"
) else if /I "%~1"=="--no-audio" (
  set "NO_AUDIO=1"
) else if /I "%~1"=="--clean" (
  set "CLEAN=1"
) else if /I "%~1"=="--no-pause" (
  set "NOPAUSE=1"
) else if /I "%~1"=="--help" (
  goto :usage_ok
) else if /I "%~1"=="-h" (
  goto :usage_ok
) else (
  echo [ERROR] Opcion desconocida: %~1
  echo.
  goto :usage
)
shift
goto :parse_args

:args_done
if not exist "%DISC%" (
  echo [ERROR] No encuentro el juego: "%DISC%"
  exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake no esta disponible en PATH.
  echo [INFO] Instala Visual Studio 2022 o Build Tools con "Desktop development with C++" y CMake.
  exit /b 2
)

set "OUT=generated\official_v0.1.1"
set "CPP=%OUT%\cpp"
set "CB=_cb_v0.1.1"
set "PADARG="
if exist "profiles\controller_profile.ini" (
  set "PADARG=--controller-profile=profiles\controller_profile.ini"
)

if "%CLEAN%"=="1" (
  echo [CLEAN] Eliminando cache de la release oficial...
  if exist "%OUT%" rmdir /s /q "%OUT%"
  if exist "%CB%" rmdir /s /q "%CB%"
)
if not exist "%OUT%" mkdir "%OUT%"

set "DCR_TRACE_HISTORY=0"
set "HEARTBEAT=1000"
set "EXTRA_RUN="
if "%PERF%"=="1" set "EXTRA_RUN=!EXTRA_RUN! --perf-profile --perf-sample-stride=64"
if "%DEBUG%"=="1" (
  set "DCR_TRACE_HISTORY=1"
  set "HEARTBEAT=500"
  set "EXTRA_RUN=!EXTRA_RUN! --host-window"
)
if "%PVR_PROFILE%"=="1" set "EXTRA_RUN=!EXTRA_RUN! --pvr-profile --host-window"

set "GPU_ARGS=--pvr-gpu --pvr-render-done-scheduled --pvr-mt"
if "%SOFTWARE%"=="1" set "GPU_ARGS=--pvr-mt"
set "AUDIO_ARGS_FULL=--aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768 --aica-play"
set "AUDIO_ARGS_BASIC=--aica-arm7 --aica-play"
if "%NO_AUDIO%"=="1" (
  set "AUDIO_ARGS_FULL=--aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768"
  set "AUDIO_ARGS_BASIC=--aica-arm7"
)

cls
echo ============================================================
echo  DreamcastRecomp %DCR_VERSION%
echo  Universal CDI runner
echo ============================================================
echo  Juego : %DISC%
echo  Perf  : %PERF%   Debug: %DEBUG%   PVR profile: %PVR_PROFILE%
echo  GPU   : %SOFTWARE% ^(0=D3D11+fallback, 1=software^)
echo  Audio : %NO_AUDIO% ^(0=on, 1=muted^)
echo ============================================================
echo.

rem Build only the tools needed by the universal path. CMake/MSBuild remain incremental.
echo [1/6] Preparando herramientas %DCR_VERSION%...
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

echo.
echo [2/6] Analizando CDI y creando mapa GD-ROM...
build\Release\dc_disc_probe.exe "%DISC%" ^
  --extract-ip="%OUT%\IP.BIN" ^
  --extract-boot="%OUT%\BOOT.DISC.BIN" ^
  --extract-disc-map="%OUT%\disc.map"
if errorlevel 1 exit /b 3

set "TITLE="
for /f "tokens=1,* delims==" %%A in ('findstr /B /I "title=" "%OUT%\disc.map"') do set "TITLE=%%B"
set "VOLUME="
for /f "tokens=1,* delims==" %%A in ('findstr /B /I "volume=" "%OUT%\disc.map"') do set "VOLUME=%%B"

set "MODE=GENERIC"
if /I "!TITLE!"=="PIZZICATO POLKA" set "MODE=DAYTONA"
if /I "!TITLE!"=="RECORD OF LODOSS WAR" set "MODE=LODOSS"
if /I "!TITLE!"=="CHUCHU ROCKET" set "MODE=CHUCHU"
if /I "!TITLE!"=="CRAZY TAXI 2" set "MODE=CT2"

echo [INFO] Titulo : !TITLE!
echo [INFO] Volumen: !VOLUME!
echo [INFO] Perfil : !MODE!
if defined PADARG (echo [INFO] Mando  : profiles\controller_profile.ini) else (echo [INFO] Mando  : default teclado/XInput/PS4-DirectInput)

if /I "!MODE!"=="LODOSS" goto :prepare_lodoss

echo.
echo [3/6] Preparando bootstrap comercial...
build\Release\dc_boot_prepare.exe "%OUT%\IP.BIN" "%OUT%\BOOT.DISC.BIN" ^
  --mode=auto ^
  --boot-out="%OUT%\BOOT.BIN" ^
  --combined-out="%OUT%\BOOTSTRAP.BIN"
if errorlevel 1 exit /b 4

echo.
echo [4/6] Recompilando closure SH-4...
if /I "!MODE!"=="DAYTONA" (
  build\Release\dc_raw_recomp.exe "%OUT%\BOOTSTRAP.BIN" ^
    --base=0x8C008000 --entry=0x8C008300 ^
    --seed-file="profiles\daytona_usa_known_seeds.txt" ^
    --max-functions=8192 --max-closure-entries=6144 --closure-passes=64 ^
    --map="%OUT%\function_map.csv" --output="%CPP%"
) else if /I "!MODE!"=="CT2" (
  build\Release\dc_raw_recomp.exe "%OUT%\BOOTSTRAP.BIN" ^
    --base=0x8C008000 --entry=0x8C008300 --seed=0x8C010000 ^
    --seed-file="profiles\crazy_taxi_2_known_seeds.txt" ^
    --max-functions=8192 --max-closure-entries=6144 --closure-passes=64 ^
    --map="%OUT%\function_map.csv" --output="%CPP%"
) else (
  build\Release\dc_raw_recomp.exe "%OUT%\BOOTSTRAP.BIN" ^
    --base=0x8C008000 --entry=0x8C008300 --seed=0x8C010000 ^
    --max-functions=8192 --max-closure-entries=6144 --closure-passes=64 ^
    --map="%OUT%\function_map.csv" --output="%CPP%"
)
if errorlevel 1 exit /b 5
goto :compile_runner

:prepare_lodoss
echo.
echo [3/6] Extrayendo ejecutable secundario 1NOSDC.BIN de Lodoss...
build\Release\dc_disc_probe.exe "%DISC%" --extract-file=1NOSDC.BIN="%OUT%\1NOSDC.BIN"
if errorlevel 1 exit /b 4

echo.
echo [4/6] Recompilando closure SH-4 de Lodoss...
build\Release\dc_raw_recomp.exe "%OUT%\1NOSDC.BIN" ^
  --base=0x8C010000 --entry=0x8C010000 ^
  --seed-file="profiles\record_of_lodoss_war_known_seeds.txt" ^
  --max-functions=10000 --max-closure-entries=9000 --closure-passes=64 ^
  --map="%OUT%\function_map.csv" --output="%CPP%"
if errorlevel 1 exit /b 5

:compile_runner
echo.
echo [5/6] Compilando runner nativo generado...
cmake -S "%CPP%" -B "%CB%" -A x64
if errorlevel 1 exit /b 6
cmake --build "%CB%" --config Release --target dreamcast_program --parallel 1
if errorlevel 1 exit /b 7

echo.
echo [6/6] Ejecutando !MODE!...
echo [INFO] Controles: flechas=D-pad, WASD=analogico, Z/J/Space=A, X/K=B, C/U=X, V/I=Y, Enter=START.
echo [INFO] Opciones activas: !EXTRA_RUN!
echo.

set "EXE=%CB%\Release\dreamcast_program.exe"
if not exist "!EXE!" (
  echo [ERROR] No se genero !EXE!
  exit /b 7
)

if /I "!MODE!"=="DAYTONA" (
  "!EXE!" ^
    --direct-game-entry=0x8C010000 --commercial-boot --disc-map="%OUT%\disc.map" ^
    --pvr-window --pvr-frame-sync --fast-dispatch --direct-dispatch --sh4-tick-batch=256 ^
    --maple-host-input !PADARG! --device-clock !AUDIO_ARGS_BASIC! --diag-heartbeat-ms=!HEARTBEAT! !EXTRA_RUN!
) else if /I "!MODE!"=="CT2" (
  "!EXE!" ^
    --commercial-boot --ct2-compat --disc-map="%OUT%\disc.map" ^
    --pvr-window --pvr-frame-sync !GPU_ARGS! ^
    --fast-dispatch --direct-dispatch --sh4-tick-batch=256 ^
    --maple-host-input !PADARG! !AUDIO_ARGS_FULL! ^
    --device-clock-host --device-clock-host-max-catchup=4096 --diag-heartbeat-ms=!HEARTBEAT! !EXTRA_RUN!
) else if /I "!MODE!"=="LODOSS" (
  "!EXE!" ^
    --direct-game-entry=0x8C010000 --commercial-boot --disc-map="%OUT%\disc.map" ^
    --pvr-window --pvr-frame-sync --pvr-mt --fast-dispatch --direct-dispatch --sh4-tick-batch=256 ^
    --maple-host-input !PADARG! --device-clock !AUDIO_ARGS_BASIC! --diag-heartbeat-ms=!HEARTBEAT! !EXTRA_RUN!
) else (
  "!EXE!" ^
    --commercial-boot --disc-map="%OUT%\disc.map" ^
    --pvr-window --pvr-frame-sync !GPU_ARGS! ^
    --fast-dispatch --direct-dispatch --sh4-tick-batch=256 ^
    --maple-host-input !PADARG! !AUDIO_ARGS_FULL! ^
    --device-clock-host --device-clock-host-max-catchup=4096 --diag-heartbeat-ms=!HEARTBEAT! !EXTRA_RUN!
)

set "RC=!ERRORLEVEL!"
echo.
echo [INFO] DreamcastRecomp %DCR_VERSION% termino con RC=!RC!.
if "!RC!"=="130" echo [INFO] RC=130 = cierre manual de la ventana.
exit /b !RC!

:usage
echo.
echo DreamcastRecomp v0.1.1 Official
echo.
echo Uso:
echo   %~nx0 "juego.cdi" [opciones]
echo.
echo Ejemplos:
echo   %~nx0 "C:\Juegos\ChuChu Rocket.cdi"
echo   %~nx0 "C:\Juegos\Crazy Taxi 2.cdi" --perf
echo   %~nx0 "C:\Juegos\Daytona USA.cdi" --debug
echo   %~nx0 "C:\Juegos\Record of Lodoss War.cdi" --clean
 echo.
echo Opciones:
echo   --perf       Perfil de rendimiento de bajo overhead.
echo   --debug      Diagnostico adicional + heartbeat cada 500 ms.
echo   --profile    Perfil detallado PVR ^(mas lento^).
echo   --software   Fuerza renderer PVR software en perfiles compatibles.
echo   --no-audio   Ejecuta ARM7/AICA sin salida de audio al host.
echo   --clean      Regenera el workspace oficial desde cero.
echo   --no-pause   Reservado para wrappers/automatizacion.
echo   --help, -h   Muestra esta ayuda.
echo.
echo Tambien puedes arrastrar un archivo .cdi sobre run_game.bat.
exit /b 1

:usage_ok
call :usage
exit /b 0
