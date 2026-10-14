@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\.."

set "DCR_VERSION=DCRV1.2"
set "PERF=0"
set "DEBUG=0"
set "HEARTBEAT_ENABLED=0"
set "PVR_PROFILE=0"
set "SOFTWARE=0"
set "RENDERER=auto"
set "RENDERER_EXPLICIT=0"
set "NO_AUDIO=0"
set "AUDIO_BASIC=0"
set "CLEAN=0"
set "NOPAUSE=0"
set "REUSE=0"
set "FORCE_RECOMPILE_GAME=0"
set "REBUILD_RUNTIME=0"
set "REBUILD_NETWORK=0"
set "REBUILD_MAPLE=0"
set "REBUILD_GDROM=0"
set "MAPLE_TRACE=0"
set "GDROM_TRACE=0"
set "CODE_RELOC_TRACE=0"
rem interp-fallback-v1: SH-4 interpreter for addresses the AOT closure never compiled.
set "SH4_INTERP=1"
set "SH4_INTERP_TRACE=0"
set "SH4_DYNAMIC_AOT=0"
set "SH4_BAD_TARGET_TRACE=0"
set "NATIVE_SEEDS=1"
set "NATIVE_PATCH_LOAD=1"
set "DCR_GUEST_ABI=1"
set "DCR_RUNTIME_BUILD_ID=cpp38e-v8.6-sh4-dynamic-ram-aot-v1"
set "DCR_BUILD_SHORT=DCRV1.2"
set "AUTO_PORTABLE_SYNC=1"
rem cpp38b2: the historical SAFE seed was compiled with always-on Superblock telemetry.
rem Do not overwrite the one-time metric-free guest rebuild with that older DLL.
set "SAFE_SEED_AUTO=0"
set "SYNC_ONLY=0"
set "FPU_MODE=superblock"
set "FPU_METRICS=0"
set "HOST_SYNC_QUANTUM=262144"
set "SPG_WAIT_FF=1"
set "HOTPC=0"
set "HOTPC_STRIDE=2048"
set "HOTREC=1"
set "DCR_GUEST_IMPL_REV=cpp38e-hot-block-recovery-v1"

if "%~1"=="" goto :usage
if /I "%~1"=="--help" goto :usage_ok
if /I "%~1"=="-h" goto :usage_ok

set "DISC=%~f1"
shift

:parse_args
if "%~1"=="" goto :args_done
set "ARG=%~1"

if /I "!ARG!"=="--renderer" goto :parse_renderer_value
if /I "!ARG:~0,11!"=="--renderer=" (
  set "RENDERER=!ARG:~11!"
  set "RENDERER_EXPLICIT=1"
  shift
  goto :parse_args
)
if /I "!ARG:~0,6!"=="--gpu=" (
  set "RENDERER=!ARG:~6!"
  set "RENDERER_EXPLICIT=1"
  shift
  goto :parse_args
)

if /I "!ARG!"=="--heartbeat-on" (
  set "HEARTBEAT_ENABLED=1"
) else if /I "!ARG!"=="--heartbeat-off" (
  set "HEARTBEAT_ENABLED=0"
) else if /I "!ARG!"=="--perf" (
  set "PERF=1"
) else if /I "!ARG!"=="--debug" (
  set "DEBUG=1"
) else if /I "!ARG!"=="--profile" (
  set "PVR_PROFILE=1"
) else if /I "!ARG!"=="--software" (
  set "SOFTWARE=1"
  set "RENDERER=software"
  set "RENDERER_EXPLICIT=1"
) else if /I "!ARG!"=="--gpu" (
  set "RENDERER=dx11"
  set "RENDERER_EXPLICIT=1"
) else if /I "!ARG!"=="--audio-basic" (
  set "AUDIO_BASIC=1"
) else if /I "!ARG!"=="--no-audio" (
  set "NO_AUDIO=1"
) else if /I "!ARG!"=="--clean" (
  set "CLEAN=1"
) else if /I "!ARG!"=="--no-pause" (
  set "NOPAUSE=1"
) else if /I "!ARG!"=="--reuse" (
  set "REUSE=1"
) else if /I "!ARG!"=="--no-recompile" (
  set "REUSE=1"
) else if /I "!ARG!"=="--recompile-game" (
  set "FORCE_RECOMPILE_GAME=1"
) else if /I "!ARG!"=="--portableize" (
  set "FORCE_RECOMPILE_GAME=1"
) else if /I "!ARG!"=="--rebuild-runtime" (
  set "REBUILD_RUNTIME=1"
) else if /I "!ARG!"=="--rebuild-network" (
  set "REBUILD_NETWORK=1"
) else if /I "!ARG!"=="--rebuild-maple" (
  set "REBUILD_MAPLE=1"
) else if /I "!ARG!"=="--rebuild-gdrom" (
  set "REBUILD_GDROM=1"
) else if /I "!ARG!"=="--maple-trace" (
  set "MAPLE_TRACE=1"
) else if /I "!ARG!"=="--code-reloc-trace" (
  set "CODE_RELOC_TRACE=1"
) else if /I "!ARG!"=="--no-interp" (
  set "SH4_INTERP=0"
) else if /I "!ARG!"=="--interp-trace" (
  set "SH4_INTERP_TRACE=1"
) else if /I "!ARG!"=="--dynamic-aot" (
  set "SH4_DYNAMIC_AOT=1"
) else if /I "!ARG!"=="--bad-target-trace" (
  set "SH4_BAD_TARGET_TRACE=1"
) else if /I "!ARG!"=="--no-native-seeds" (
  set "NATIVE_SEEDS=0"
) else if /I "!ARG!"=="--native-seeds" (
  set "NATIVE_SEEDS=1"
) else if /I "!ARG!"=="--no-native-patches" (
  set "NATIVE_SEEDS=0"
  set "NATIVE_PATCH_LOAD=0"
) else if /I "!ARG!"=="--gdrom-trace" (
  set "GDROM_TRACE=1"
) else if /I "!ARG!"=="--fpu-superblock" (
  set "FPU_MODE=superblock"
  set "FPU_METRICS=0"
) else if /I "!ARG!"=="--fpu-regionplus" (
  set "FPU_MODE=regionplus"
  set "FPU_METRICS=0"
) else if /I "!ARG!"=="--fpu-metrics" (
  echo [WARN] v1.2 mantiene metrics=0; HOT BLOCK RECOVERY modifica solo el guest generado de ChuChu una vez.
  set "FPU_METRICS=0"
) else if /I "!ARG!"=="--spg-ff-off" (
  set "SPG_WAIT_FF=0"
) else if /I "!ARG!"=="--spg-ff-on" (
  set "SPG_WAIT_FF=1"
) else if /I "!ARG!"=="--hotpc" (
  set "HOTPC=1"
) else if /I "!ARG!"=="--hotpc-off" (
  set "HOTPC=0"
) else if /I "!ARG!"=="--hot-recovery-off" (
  set "HOTREC=0"
) else if /I "!ARG!"=="--hot-recovery-on" (
  set "HOTREC=1"
) else if /I "!ARG!"=="--sync-524" (
  set "HOST_SYNC_QUANTUM=524288"
) else if /I "!ARG!"=="--sync-portable" (
  set "REUSE=1"
  set "AUTO_PORTABLE_SYNC=1"
  set "SYNC_ONLY=1"
) else if /I "!ARG!"=="--no-portable-sync" (
  set "AUTO_PORTABLE_SYNC=0"
) else if /I "!ARG!"=="--no-safe-seed" (
  set "SAFE_SEED_AUTO=0"
) else if /I "!ARG!"=="--help" (
  goto :usage_ok
) else if /I "!ARG!"=="-h" (
  goto :usage_ok
) else (
  echo [ERROR] Opcion desconocida: %~1
  echo.
  goto :usage
)
shift
goto :parse_args

:parse_renderer_value
shift
if "%~1"=="" (
  echo [ERROR] --renderer requiere un backend.
  exit /b 1
)
set "RENDERER=%~1"
set "RENDERER_EXPLICIT=1"
shift
goto :parse_args

:args_done
set "DCR_FPU_MODE=!FPU_MODE!"
set "DCR_FPU_METRICS=!FPU_METRICS!"
set "DCR_HOST_SYNC_QUANTUM=!HOST_SYNC_QUANTUM!"
set "DCR_SPG_WAIT_FF=!SPG_WAIT_FF!"
set "DCR_CPP38D_HOTPC=!HOTPC!"
set "DCR_CPP38D_HOTPC_STRIDE=!HOTPC_STRIDE!"
set "DCR_CPP38E_HOT_RECOVERY=!HOTREC!"
echo [PERF CFG] FPU=!DCR_FPU_MODE! metrics=!DCR_FPU_METRICS! host-sync=!DCR_HOST_SYNC_QUANTUM! spg-ff=!DCR_SPG_WAIT_FF! hotpc=!DCR_CPP38D_HOTPC!/!DCR_CPP38D_HOTPC_STRIDE! hotrec=!DCR_CPP38E_HOT_RECOVERY!
if not exist "%DISC%" (
  echo [ERROR] No encuentro el juego: "%DISC%"
  exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake no esta disponible en PATH.
  echo [INFO] Instala Visual Studio 2022 o Build Tools con C++ y CMake.
  exit /b 2
)
where powershell >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PowerShell no esta disponible y es necesario para los perfiles por juego.
  exit /b 2
)
rem El ejecutable nativo cubre verificacion ABI, hash, caché runtime y seeds.
rem Python es requerido SOLO si necesitamos generar/modularizar un guest nuevo
rem o ejecutar los conversores legacy que aun no estan migrados.
set "DCR_NATIVE_TOOLS=%CD%\tools\native\dcr_native_tools.exe"

set "PROBE=generated\_profile_probe"
if exist "%PROBE%" rmdir /s /q "%PROBE%"
mkdir "%PROBE%" >nul 2>nul

cls
echo ============================================================
echo  DreamcastRecomp %DCR_VERSION%
echo  Universal CDI runner + per-game profiles
echo ============================================================
echo  Juego : %DISC%
echo  Perf  : %PERF%   Debug: %DEBUG%   PVR profile: %PVR_PROFILE%
echo ============================================================
echo.

echo [1/6] Preparando herramientas...
rem cpp38c FIX1: no confiar solo en que exista un archivo con extension .exe.
rem Algunos paquetes de desarrollo contenian placeholders de texto; Windows los
rem rechazaba antes de llegar al cache del juego. Validamos cabecera PE/MZ y,
rem si el tool no es ejecutable, lo eliminamos y compilamos SOLO las herramientas.
for %%T in (dc_disc_probe dc_boot_prepare) do (
  if exist "build\Release\%%T.exe" (
    powershell -NoProfile -Command "$p='build\Release\%%T.exe'; try{$b=[IO.File]::ReadAllBytes($p); if($b.Length -ge 2 -and $b[0] -eq 0x4D -and $b[1] -eq 0x5A){exit 0}}catch{}; exit 1" >nul 2>nul
    if errorlevel 1 (
      echo [TOOLS] %%T.exe invalido/placeholder; se reconstruira SOLO esta herramienta.
      del /q "build\Release\%%T.exe" >nul 2>nul
    )
  )
)
set "TOOL_BUILD_NEEDED=0"
if not exist "build\Release\dc_disc_probe.exe" set "TOOL_BUILD_NEEDED=1"
if not exist "build\Release\dc_boot_prepare.exe" set "TOOL_BUILD_NEEDED=1"
if "%REUSE%"=="0" if not exist "build\Release\dc_raw_recomp.exe" set "TOOL_BUILD_NEEDED=1"
if "!TOOL_BUILD_NEEDED!"=="1" (
  cmake -S . -B build -A x64
  if errorlevel 1 exit /b 2
  set "TOOL_TARGETS=dc_disc_probe dc_boot_prepare"
  if "%REUSE%"=="0" set "TOOL_TARGETS=!TOOL_TARGETS! dc_raw_recomp"
  if "%REUSE%"=="1" echo [TOOLS] --reuse: compilando SOLO probe/bootstrap; NO se toca ni recompila el guest SH-4.
  cmake --build build --config Release --target !TOOL_TARGETS! --parallel 1
  if errorlevel 1 exit /b 2
) else (
  if "%REUSE%"=="1" echo [TOOLS] --reuse: probe/bootstrap Windows validos; dc_raw_recomp NO se construye.
)

for %%T in (dc_disc_probe dc_boot_prepare) do (
  if not exist "build\Release\%%T.exe" (
    echo [ERROR] Falta build\Release\%%T.exe.
    exit /b 2
  )
)
if "%REUSE%"=="0" if not exist "build\Release\dc_raw_recomp.exe" (
  echo [ERROR] Falta build\Release\dc_raw_recomp.exe.
  exit /b 2
)

echo.
echo [2/6] Analizando CDI y resolviendo perfil...
build\Release\dc_disc_probe.exe "%DISC%" ^
  --extract-ip="%PROBE%\IP.BIN" ^
  --extract-boot="%PROBE%\BOOT.DISC.BIN" ^
  --extract-disc-map="%PROBE%\disc.map"
if errorlevel 1 exit /b 3

set "TITLE="
for /f "tokens=1,* delims==" %%A in ('findstr /B /I "title=" "%PROBE%\disc.map"') do set "TITLE=%%B"
set "VOLUME="
for /f "tokens=1,* delims==" %%A in ('findstr /B /I "volume=" "%PROBE%\disc.map"') do set "VOLUME=%%B"

if defined DCR_CACHE_ROOT (
  set "GAME_CACHE_ROOT=!DCR_CACHE_ROOT!"
) else if defined LOCALAPPDATA (
  set "GAME_CACHE_ROOT=!LOCALAPPDATA!\DreamcastRecompiled\games"
) else (
  set "GAME_CACHE_ROOT=%CD%\games"
)
if not exist "!GAME_CACHE_ROOT!" mkdir "!GAME_CACHE_ROOT!" >nul 2>nul
echo [CACHE] Guest ABI v%DCR_GUEST_ABI% root: !GAME_CACHE_ROOT!

if defined DCR_SYSTEM_DATA_ROOT (
  set "SYSTEM_DATA_ROOT=!DCR_SYSTEM_DATA_ROOT!"
) else if defined LOCALAPPDATA (
  set "SYSTEM_DATA_ROOT=!LOCALAPPDATA!\DreamcastRecompiled\system"
) else (
  set "SYSTEM_DATA_ROOT=%CD%\system-data"
)
if not exist "!SYSTEM_DATA_ROOT!" mkdir "!SYSTEM_DATA_ROOT!" >nul 2>nul
rem One-time convenience migration from a pre-cpp34 folder when files are beside this launcher.
if not exist "!SYSTEM_DATA_ROOT!\dreamcast_flash.bin" if exist "%CD%\dreamcast_flash.bin" copy /y "%CD%\dreamcast_flash.bin" "!SYSTEM_DATA_ROOT!\dreamcast_flash.bin" >nul
if not exist "!SYSTEM_DATA_ROOT!\dreamcast_vmu_a1.bin" if exist "%CD%\dreamcast_vmu_a1.bin" copy /y "%CD%\dreamcast_vmu_a1.bin" "!SYSTEM_DATA_ROOT!\dreamcast_vmu_a1.bin" >nul
if defined DCR_IMPORT_SYSTEM_DIR (
  if not exist "!SYSTEM_DATA_ROOT!\dreamcast_flash.bin" if exist "!DCR_IMPORT_SYSTEM_DIR!\dreamcast_flash.bin" copy /y "!DCR_IMPORT_SYSTEM_DIR!\dreamcast_flash.bin" "!SYSTEM_DATA_ROOT!\dreamcast_flash.bin" >nul
  if not exist "!SYSTEM_DATA_ROOT!\dreamcast_vmu_a1.bin" if exist "!DCR_IMPORT_SYSTEM_DIR!\dreamcast_vmu_a1.bin" copy /y "!DCR_IMPORT_SYSTEM_DIR!\dreamcast_vmu_a1.bin" "!SYSTEM_DATA_ROOT!\dreamcast_vmu_a1.bin" >nul
)
set "DCR_SYSTEM_DATA_DIR=!SYSTEM_DATA_ROOT!"
echo [DATA] Dreamcast persistente: !DCR_SYSTEM_DATA_DIR!

set "PROFILE_CMD=%TEMP%\dcr_profile_%RANDOM%_%RANDOM%.cmd"
powershell -NoProfile -ExecutionPolicy Bypass -File "tools\game_profiles\ResolveGameProfile.ps1" ^
  -Title "!TITLE!" -Volume "!VOLUME!" -Disc "%DISC%" -Root "!GAME_CACHE_ROOT!" -EmitCmd "!PROFILE_CMD!"
if errorlevel 1 (
  echo [ERROR] No se pudo crear/resolver el perfil por juego.
  exit /b 3
)
call "!PROFILE_CMD!"
del /q "!PROFILE_CMD!" >nul 2>nul

rem Per-game function limits are part of the generated SH-4 closure, not runtime.
rem Only modified profiles trigger a guest rebuild; other caches remain intact.
set "INTERP_SEED_FILE=!DCR_GAME_DIR!\interp_seeds.txt"
set "INTERP_SEED_COUNT=0"
if exist "!INTERP_SEED_FILE!" for /f %%C in ('findstr /B /C:"0x" "!INTERP_SEED_FILE!" ^| find /C /V ""') do set "INTERP_SEED_COUNT=%%C"
set "DCR_LIMITS_RECOMPILE_MARKER=!DCR_GAME_DIR!\.dcr_limits_recompile_required"
if exist "!DCR_LIMITS_RECOMPILE_MARKER!" (
  if "!REUSE!"=="1" (
    echo [ERROR] Se cambiaron limites SH-4 de !DCR_GAME_SLUG!; --reuse no puede utilizar el modulo anterior.
    echo [INFO] Inicia sin --reuse para regenerarlo. Se conserva el cache hasta compilar con exito.
    exit /b 14
  )
  echo [FUNCTION LIMITS] Configuracion SH-4 modificada para !DCR_GAME_SLUG!. Regenerando modulo.
  set "FORCE_RECOMPILE_GAME=1"
)
echo [FUNCTION LIMITS] !DCR_GAME_SLUG!: max-functions=!DCR_MAX_FUNCTIONS! closure-entries=!DCR_MAX_CLOSURE_ENTRIES! passes=!DCR_CLOSURE_PASSES!

rem cpp38b: keep user-visible window names short and game-specific even when
rem reusing an old cached runner whose generated title contains a historical version.
set "DCR_GAME_TITLE=!TITLE!"
if not defined DCR_GAME_TITLE set "DCR_GAME_TITLE=!DCR_GAME_SLUG!"
set "DCR_WINDOW_TITLE=DCRV1.2 - !DCR_GAME_TITLE!"
set "DCR_STATUS_WINDOW_TITLE=DCRV1.2 Diagnostics - !DCR_GAME_TITLE!"

set "OUT=!DCR_GAME_DIR!\generated"
set "CPP=!OUT!\cpp"
set "CB=!DCR_GAME_DIR!\build"
set "PORTABLE_DIR=!DCR_GAME_DIR!\portable"
set "PORTABLE_GUEST_MARKER=!CPP!\.dcr_portable_guest_v1"
set "GUEST_PORTABLE=0"

if "%CLEAN%"=="1" (
  echo [CLEAN] Limpiando guest cache modular de !DCR_GAME_SLUG!...
  if exist "!OUT!" rmdir /s /q "!OUT!"
  if exist "!CB!" rmdir /s /q "!CB!"
  set "FORCE_RECOMPILE_GAME=1"
)
if not exist "!OUT!" mkdir "!OUT!" >nul 2>nul
if not exist "!CB!" mkdir "!CB!" >nul 2>nul

set "GUEST_CACHE_VALID=0"
if exist "!CPP!\.dcr_guest_abi" (
  findstr /B /C:"DCR_GUEST_ABI=%DCR_GUEST_ABI%" "!CPP!\.dcr_guest_abi" >nul 2>nul
  if not errorlevel 1 set "GUEST_CACHE_VALID=1"
)
if "%FORCE_RECOMPILE_GAME%"=="1" set "GUEST_CACHE_VALID=0"

rem cpp34j recovery: cpp34h accidentally changed the ABI1 struct layout. A cache
rem generated while testing cpp34h must never be reused, even if .dcr_guest_abi says 1.
set "DCR_CACHE_CONTAMINATED=0"
if "!GUEST_CACHE_VALID!"=="1" if exist "!CPP!\generated_runner.cpp" (
  findstr /C:"cpp34h BOOTROM_RO_FLYCAST_MODEM_TRACE_ABI1" "!CPP!\generated_runner.cpp" >nul 2>nul
  if not errorlevel 1 set "DCR_CACHE_CONTAMINATED=1"
)
if "!GUEST_CACHE_VALID!"=="1" if exist "!CPP!\dc_runtime.hpp" (
  rem cpp35b1: compare ONLY the frozen ABI1 DCRuntime structure. Runtime/plugin
  rem declarations outside the guest ABI are allowed to change without forcing SH-4 rebuilds.
  if exist "%DCR_NATIVE_TOOLS%" (
    "%DCR_NATIVE_TOOLS%" abi "!CPP!\dc_runtime.hpp" "%CD%\runtime\guest_abi\v1\dc_runtime.hpp" >nul 2>nul
  ) else (
    python "tools\modular\guest_abi_fingerprint.py" "!CPP!\dc_runtime.hpp" "%CD%\runtime\guest_abi\v1\dc_runtime.hpp" >nul 2>nul
  )
  if errorlevel 1 set "DCR_CACHE_CONTAMINATED=1"
)
if "!GUEST_CACHE_VALID!"=="1" if not exist "!CPP!\dc_runtime.hpp" (
  if exist "!CB!\Release\dcr_game_module.dll" (
    echo [GUEST CACHE] Cache ABI1 legado sin header local; se conserva el modulo compilado.
  ) else (
    set "DCR_CACHE_CONTAMINATED=1"
  )
)
if "!DCR_CACHE_CONTAMINATED!"=="1" (
  if "%REUSE%"=="1" (
    echo [ERROR] --reuse detecto un Guest ABI1 incompatible/contaminado.
    echo [SAFE] Cache preservado: NO se borra, NO se parchea y NO se recompila nada del guest.
    exit /b 12
  )
  echo [CACHE RECOVERY] El layout Guest ABI1 no coincide con el ABI congelado. Regenerando guest por ejecucion explicita sin --reuse...
  if exist "!OUT!" rmdir /s /q "!OUT!"
  if exist "!CB!" rmdir /s /q "!CB!"
  mkdir "!OUT!" >nul 2>nul
  mkdir "!CB!" >nul 2>nul
  set "GUEST_CACHE_VALID=0"
)

rem cpp38e: the hot-block investigation requires one generated-guest rebuild.
rem It reuses the existing generated SH-4 closure: no CDI rediscovery/re-analysis.
set "GUEST_IMPL_STAMP=!DCR_GAME_DIR!\.dcr_guest_impl_revision"
if "!GUEST_CACHE_VALID!"=="1" if "!DCR_CPP38E_HOT_RECOVERY!"=="1" if /I "!DCR_GAME_SLUG!"=="CHUCHU_ROCKET" (
  set "CPP38E_READY=0"
  if exist "!GUEST_IMPL_STAMP!" (
    findstr /X /C:"cpp38e-hot-block-recovery-v1" "!GUEST_IMPL_STAMP!" >nul 2>nul
    if not errorlevel 1 set "CPP38E_READY=1"
  )
  if "!CPP38E_READY!"=="0" (
    echo [v1.2] Aplicando HOT BLOCK RECOVERY una sola vez al guest ABI1 existente...
    call "%CD%\scripts\stable\APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat" --game-dir "!DCR_GAME_DIR!"
    if errorlevel 1 exit /b !errorlevel!
  ) else (
    echo [v1.2] HOT BLOCK RECOVERY cacheado: no se recompila el guest.
  )
)
if "!GUEST_CACHE_VALID!"=="1" (
  if exist "!GUEST_IMPL_STAMP!" (
    set /p DCR_EXISTING_GUEST_IMPL=<"!GUEST_IMPL_STAMP!"
    echo [v1.2] Guest impl: !DCR_EXISTING_GUEST_IMPL!
  ) else (
    echo [v1.2] Guest impl sin stamp; se conserva sin migrar.
  )
)

rem cpp36b1d: robust safe-guest adoption.
rem The Python helper normalizes title/volume and validates slug + ABI + SHA256.
rem If the current module already IS the safe seed it only writes the marker; no copy, no compile.
set "SAFE_SEED_DIR=%CD%\safe_guests\!DCR_GAME_SLUG!"
set "SAFE_SEED_DLL=!SAFE_SEED_DIR!\dcr_game_module.dll"
set "SAFE_SEED_MANIFEST=!SAFE_SEED_DIR!\manifest.ini"
set "PORTABLE_GAME_MARKER=!DCR_GAME_DIR!\.dcr_portable_guest_v1"
if "!SAFE_SEED_AUTO!"=="1" if "!GUEST_CACHE_VALID!"=="1" if exist "!SAFE_SEED_DLL!" if exist "!SAFE_SEED_MANIFEST!" (
  if exist "%DCR_NATIVE_TOOLS%" (
    "%DCR_NATIVE_TOOLS%" adopt-safe ^
      --manifest "!SAFE_SEED_MANIFEST!" --seed "!SAFE_SEED_DLL!" ^
      --module "!CB!\Release\dcr_game_module.dll" ^
      --marker "!PORTABLE_GAME_MARKER!" --legacy-marker "!PORTABLE_GUEST_MARKER!" ^
      --slug "!DCR_GAME_SLUG!" --title "!TITLE!" --volume "!VOLUME!" --abi "%DCR_GUEST_ABI%"
  ) else (
    python "tools\portable\adopt_safe_guest.py" ^
      --manifest "!SAFE_SEED_MANIFEST!" --seed "!SAFE_SEED_DLL!" ^
      --module "!CB!\Release\dcr_game_module.dll" ^
      --marker "!PORTABLE_GAME_MARKER!" --legacy-marker "!PORTABLE_GUEST_MARKER!" ^
      --slug "!DCR_GAME_SLUG!" --title "!TITLE!" --volume "!VOLUME!" --abi "%DCR_GUEST_ABI%"
  )
  if not errorlevel 1 set "GUEST_PORTABLE=1"
)
if exist "!PORTABLE_GAME_MARKER!" set "GUEST_PORTABLE=1"
if exist "!PORTABLE_GUEST_MARKER!" set "GUEST_PORTABLE=1"

rem Always refresh the disc map path without touching generated SH-4 code.
copy /y "%PROBE%\disc.map" "!OUT!\disc.map" >nul

rem cpp36b1d: portable-safe guest = trusted marker or raw-free generated source.
if exist "!PORTABLE_GAME_MARKER!" set "GUEST_PORTABLE=1"
if exist "!PORTABLE_GUEST_MARKER!" set "GUEST_PORTABLE=1"
if "!GUEST_PORTABLE!"=="0" if "!GUEST_CACHE_VALID!"=="1" if exist "!CPP!\dc_image.cpp" (
  findstr /C:".raw_boot @" "!CPP!\dc_image.cpp" >nul 2>nul
  if errorlevel 1 (
    >"!PORTABLE_GUEST_MARKER!" echo DCR_PORTABLE_GUEST=1
    >"!PORTABLE_GAME_MARKER!" echo DCR_PORTABLE_GUEST=1
    >>"!PORTABLE_GAME_MARKER!" echo DCR_GUEST_ABI=%DCR_GUEST_ABI%
    set "GUEST_PORTABLE=1"
    echo [PORTABLE] Guest cache existente ya es raw-free; se adopta sin recompilar.
  ) else (
    echo [PORTABLE] Guest legado: contiene .raw_boot embebido.
    echo [PORTABLE] --reuse lo conserva para test normal, pero NO lo distribuye como portable.
    echo [PORTABLE] Ejecuta --portableize una sola vez para migrarlo; despues --reuse mantendra la portable automaticamente.
  )
)

if "!GUEST_CACHE_VALID!"=="1" (
  echo [GUEST CACHE] ABI v%DCR_GUEST_ABI% valido: NO se recompila SH-4.
  goto :profile_ready
)

if "%REUSE%"=="1" (
  echo [ERROR] --no-recompile/--reuse solicitado pero no existe un guest ABI v%DCR_GUEST_ABI% valido.
  echo [INFO] --reuse nunca recompila automaticamente. Usa --portableize/--recompile-game solo si quieres regenerar el guest.
  exit /b 9
)

copy /y "%PROBE%\IP.BIN" "!OUT!\IP.BIN" >nul
copy /y "%PROBE%\BOOT.DISC.BIN" "!OUT!\BOOT.DISC.BIN" >nul

:profile_ready
rem cpp36b1: retail bytes live only in this temporary local probe.
set "EXTERNAL_BOOTSTRAP=%PROBE%\BOOTSTRAP.BIN"
set "EXTERNAL_BOOT=%PROBE%\BOOT.BIN"
build\Release\dc_boot_prepare.exe "%PROBE%\IP.BIN" "%PROBE%\BOOT.DISC.BIN" ^
  --mode=auto ^
  --boot-out="!EXTERNAL_BOOT!" ^
  --combined-out="!EXTERNAL_BOOTSTRAP!"
if errorlevel 1 exit /b 4

call :normalize_renderer
if errorlevel 1 exit /b !errorlevel!

if "%RENDERER_EXPLICIT%"=="0" (
  set "ACTIVE_RENDERER=!DCR_PROFILE_RENDERER!"
) else (
  set "ACTIVE_RENDERER=!RENDERER!"
)
if /I "!ACTIVE_RENDERER!"=="auto" set "ACTIVE_RENDERER=dx11"
set "RENDERER=!ACTIVE_RENDERER!"
call :normalize_renderer
if errorlevel 1 exit /b !errorlevel!
set "ACTIVE_RENDERER=!RENDERER!"

set "PADARG="
if defined DCR_CONTROLLER_PROFILE (
  if exist "!DCR_CONTROLLER_PROFILE!" set "PADARG=--controller-profile=!DCR_CONTROLLER_PROFILE!"
)
if not defined PADARG if exist "profiles\controller_profile.ini" (
  set "PADARG=--controller-profile=profiles\controller_profile.ini"
)

set "GPU_ARGS="
if /I "!ACTIVE_RENDERER!"=="dx11" (
  set "GPU_ARGS=--pvr-gpu --pvr-render-done-scheduled --pvr-mt"
) else (
  set "GPU_ARGS=--pvr-mt"
)

set "DCR_TRACE_HISTORY=0"
set "DCR_CODE_RELOC_TRACE=0"
if "!CODE_RELOC_TRACE!"=="1" set "DCR_CODE_RELOC_TRACE=1"
rem interp-fallback-v1: unknown SH-4 targets are interpreted instead of aborting,
rem and verified entrypoints are learned for the next "Preparar SH-4".
set "DCR_SH4_INTERP=!SH4_INTERP!"
set "DCR_SH4_INTERP_TRACE=!SH4_INTERP_TRACE!"
set "DCR_SH4_INTERP_SEEDS=!INTERP_SEED_FILE!"
set "DCR_SH4_DYNAMIC_AOT=!SH4_DYNAMIC_AOT!"
set "DCR_SH4_BAD_TARGET_TRACE=!SH4_BAD_TARGET_TRACE!"
set "DCR_SH4_DYNAMIC_DIR=!DCR_GAME_DIR!\incremental\dynamic"
set "HEARTBEAT=0"
set "EXTRA_RUN=!DCR_PROFILE_EXTRA_RUN!"
if "%PERF%"=="1" set "EXTRA_RUN=!EXTRA_RUN! --perf-profile --perf-sample-stride=64"
if "%DEBUG%"=="1" (
  set "DCR_TRACE_HISTORY=1"
  set "EXTRA_RUN=!EXTRA_RUN! --host-window"
)
rem Heartbeat y debug son independientes. Por defecto no se emite ninguno.
if "!HEARTBEAT_ENABLED!"=="1" set "HEARTBEAT=1000"
if "%PVR_PROFILE%"=="1" set "EXTRA_RUN=!EXTRA_RUN! --pvr-profile --host-window"

set "AUDIO_ARGS=--aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768 --aica-play"
if /I "!DCR_AUDIO_PROFILE!"=="basic" set "AUDIO_ARGS=--aica-arm7 --aica-play"
if "!AUDIO_BASIC!"=="1" set "AUDIO_ARGS=--aica-arm7 --aica-play"
if "%NO_AUDIO%"=="1" (
  set "AUDIO_ARGS=--aica-arm7"
  if /I not "!DCR_AUDIO_PROFILE!"=="basic" set "AUDIO_ARGS=--aica-arm7 --aica-arm7-slice=128 --aica-arm7-boot=32768"
)

set "CLOCK_ARGS=--device-clock-host --device-clock-host-max-catchup=4096"
if /I "!DCR_DEVICE_CLOCK!"=="device" set "CLOCK_ARGS=--device-clock"

echo.
echo [INFO] Titulo     : !TITLE!
echo [INFO] Volumen    : !VOLUME!
echo [INFO] Perfil     : !DCR_MODE!
echo [INFO] CFG        : !DCR_PROFILE_CFG!
echo [INFO] Workspace  : !DCR_GAME_DIR!
echo [INFO] Render     : !ACTIVE_RENDERER!
echo [INFO] Boot stage : !DCR_BOOT_STAGE!
if defined DCR_SEED_FILE echo [INFO] Seeds      : !DCR_SEED_FILE!
if "!SH4_INTERP!"=="1" (echo [INFO] Interprete : activo ^(entradas SH-4 sin AOT se interpretan^)) else (echo [INFO] Interprete : desactivado por --no-interp)
if not "!INTERP_SEED_COUNT!"=="0" echo [INTERP] !INTERP_SEED_COUNT! entradas SH-4 aprendidas en !INTERP_SEED_FILE!; "Preparar SH-4" las compila de forma nativa.
if defined PADARG (echo [INFO] Mando      : !PADARG!) else (echo [INFO] Mando      : default teclado/XInput/DirectInput)

if "!GUEST_CACHE_VALID!"=="1" goto :cached_guest_ready

if defined DCR_SPECIAL_BOOT_FILE goto :prepare_special_boot

echo.
echo [3/6] Usando bootstrap externo temporal ^(guest portable-safe^)...
set "RECOMP_INPUT=!EXTERNAL_BOOTSTRAP!"
goto :recompile

:prepare_special_boot
echo.
echo [3/6] Extrayendo ejecutable especial !DCR_SPECIAL_BOOT_FILE!...
build\Release\dc_disc_probe.exe "%DISC%" --extract-file=!DCR_SPECIAL_BOOT_FILE!="%PROBE%\!DCR_SPECIAL_BOOT_FILE!"
if errorlevel 1 exit /b 4
set "RECOMP_INPUT=%PROBE%\!DCR_SPECIAL_BOOT_FILE!"

:recompile
echo.
echo [4/6] Recompilando closure SH-4 para !DCR_MODE!...
set "SEED_ARGS="
if defined DCR_ENTRY_SEED set "SEED_ARGS=!SEED_ARGS! --seed=!DCR_ENTRY_SEED!"
if defined DCR_SEED_FILE (
  if exist "!DCR_SEED_FILE!" (
    set "SEED_ARGS=!SEED_ARGS! --seed-file="!DCR_SEED_FILE!""
  ) else (
    echo [WARN] El seed_file del perfil no existe: !DCR_SEED_FILE!
  )
)
if not "!INTERP_SEED_COUNT!"=="0" (
  set "SEED_ARGS=!SEED_ARGS! --seed-file="!INTERP_SEED_FILE!""
  echo [INTERP] Agregando !INTERP_SEED_COUNT! entradas aprendidas por el interprete al closure SH-4.
)

build\Release\dc_raw_recomp.exe "!RECOMP_INPUT!" ^
  --base=!DCR_RAW_BASE! --entry=!DCR_RAW_ENTRY! !SEED_ARGS! ^
  --max-functions=!DCR_MAX_FUNCTIONS! ^
  --max-closure-entries=!DCR_MAX_CLOSURE_ENTRIES! ^
  --closure-passes=!DCR_CLOSURE_PASSES! ^
  --no-embed-raw-image ^
  --map="!OUT!\function_map.csv" --output="!CPP!"
if errorlevel 1 exit /b 5
findstr /C:".raw_boot @" "!CPP!\dc_image.cpp" >nul 2>nul
if not errorlevel 1 (
  echo [ERROR] PORTABLE AUDIT: .raw_boot sigue embebido en dc_image.cpp.
  exit /b 5
)
>"!PORTABLE_GUEST_MARKER!" echo DCR_PORTABLE_GUEST=1
>"!PORTABLE_GAME_MARKER!" echo DCR_PORTABLE_GUEST=1
>>"!PORTABLE_GAME_MARKER!" echo DCR_GUEST_ABI=%DCR_GUEST_ABI%
set "GUEST_PORTABLE=1"
echo [PORTABLE] Guest raw-free: el mismo modulo sirve para test normal y portable.

:modular_prepare
echo.
echo [MODULAR] Preparando NUEVO guest ABI v%DCR_GUEST_ABI% con headers congelados...
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" modularize "!CPP!" --runtime-root "%CD%\runtime"
) else (
  python "tools\modular\modularize_generated.py" "!CPP!" --runtime-root "%CD%\runtime"
)
if errorlevel 1 exit /b 6
rem cpp38b2: even if the bundled Windows recompiler was produced by an older
rem checkpoint, normalize its generated source to the metric-free implementation.
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" metricfree "!CPP!"
) else (
  python "tools\perf\patch_metricfree_guest.py" "!CPP!"
)
if errorlevel 1 exit /b 6
if "!DCR_CPP38E_HOT_RECOVERY!"=="1" if /I "!DCR_GAME_SLUG!"=="CHUCHU_ROCKET" (
  if exist "%DCR_NATIVE_TOOLS%" (
    "%DCR_NATIVE_TOOLS%" hot-recovery "!CPP!"
  ) else (
    python "tools\perf\patch_cpp38e_hot_block_recovery.py" "!CPP!"
  )
  if errorlevel 1 exit /b 6
)
set "GUEST_CACHE_VALID=1"

echo.
echo [5/6] Primera compilacion del guest modular...
cmake -S "!CPP!" -B "!CB!" -A x64 -DDCR_RUNTIME_ROOT="%CD%\runtime"
if errorlevel 1 exit /b 6
cmake --build "!CB!" --config Release --target dreamcast_program --parallel 1
if errorlevel 1 exit /b 7
rem Clear the pending-limit marker ONLY after the new guest linked successfully.
if exist "!DCR_LIMITS_RECOMPILE_MARKER!" del /q "!DCR_LIMITS_RECOMPILE_MARKER!" >nul 2>nul
>"!DCR_GAME_DIR!\.dcr_runtime_build_id" echo %DCR_RUNTIME_BUILD_ID%
>"!DCR_GAME_DIR!\.dcr_guest_impl_revision" echo !DCR_GUEST_IMPL_REV!
goto :execute_runner

:cached_guest_ready
rem cpp35b1 invariant: from here on the guest source/build metadata is read-only.
rem Do NOT run modularize_generated.py and do NOT reconfigure the guest CMake tree.
if not exist "!CB!\Release\dreamcast_program.exe" (
  echo [ERROR] El cache ABI1 es valido pero falta dreamcast_program.exe.
  echo [DCRV1.2] No se recompila el guest automaticamente para evitar una recompilacion sorpresa.
  echo [INFO] Recupera el build anterior o usa --recompile-game si realmente quieres regenerarlo.
  exit /b 10
)
if not exist "!CB!\Release\dcr_game_module.dll" (
  echo [ERROR] Falta dcr_game_module.dll en el cache ABI1 valido.
  echo [INFO] No se recompila automaticamente para proteger el cache existente.
  exit /b 10
)

set "RUNTIME_STAMP=!DCR_GAME_DIR!\.dcr_runtime_build_id"
set "DCR_RUNTIME_MANIFEST=!DCR_GAME_DIR!\.dcr_runtime_artifact_manifest.json"
set "RUNTIME_BUILD_NEEDED=0"
set "RUNTIME_HASH_MISMATCH=0"
if not exist "!CB!\Release\DreamcastRuntime.dll" set "RUNTIME_BUILD_NEEDED=1"
if not exist "!CB!\plugins\Release\dcr_network.dll" set "RUNTIME_BUILD_NEEDED=1"
if not exist "!CB!\plugins\Release\dcr_maple.dll" set "RUNTIME_BUILD_NEEDED=1"
if not exist "!CB!\plugins\Release\dcr_gdrom.dll" set "RUNTIME_BUILD_NEEDED=1"
if not exist "!RUNTIME_STAMP!" set "RUNTIME_BUILD_NEEDED=1"
if exist "!RUNTIME_STAMP!" (
  findstr /X /C:"%DCR_RUNTIME_BUILD_ID%" "!RUNTIME_STAMP!" >nul 2>nul
  if errorlevel 1 set "RUNTIME_BUILD_NEEDED=1"
)
rem Source and artifact hashes guard against stale DLLs even when the old stamp matches.
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" runtime-verify check "%CD%\runtime" "!CB!" "!DCR_RUNTIME_MANIFEST!"
) else (
  python "tools\runtime_cache\verify_runtime_artifacts.py" check "%CD%\runtime" "!CB!" "!DCR_RUNTIME_MANIFEST!"
)
if errorlevel 1 (
  set "RUNTIME_HASH_MISMATCH=1"
  set "RUNTIME_BUILD_NEEDED=1"
)
if "%REBUILD_RUNTIME%"=="1" set "RUNTIME_BUILD_NEEDED=1"
if "%REBUILD_NETWORK%"=="1" set "RUNTIME_BUILD_NEEDED=1"
if "%REBUILD_MAPLE%"=="1" set "RUNTIME_BUILD_NEEDED=1"
if "%REBUILD_GDROM%"=="1" set "RUNTIME_BUILD_NEEDED=1"

if "!RUNTIME_BUILD_NEEDED!"=="0" (
  echo [RUNTIME CACHE] %DCR_RUNTIME_BUILD_ID% valido; guest y runtime ya estan listos.
  goto :execute_runner
)

echo.
echo [RUNTIME] Build separado: el guest ABI1 NO se toca.
set "RB=!DCR_GAME_DIR!\runtime_build_cpp35b1"
set "DCR_RUNTIME_SOURCE=%CD%\runtime"
set "DCR_RUNTIME_CACHE=!RB!\CMakeCache.txt"
rem cpp36b1b: CMake caches the absolute source directory. A runtime build tree
rem created by another extracted DreamcastRecomp version cannot be reused as-is.
rem Detect that condition and delete ONLY the runtime CMake tree. Guest generated/
rem build trees and dcr_game_module.dll are deliberately left untouched.
if exist "!RB!\CMakeCache.txt" (
  if exist "%DCR_NATIVE_TOOLS%" (
    "%DCR_NATIVE_TOOLS%" cmake-home "!RB!\CMakeCache.txt" "%CD%\runtime"
  ) else (
    python "tools\runtime_cache\check_cmake_home.py" "!RB!\CMakeCache.txt" "%CD%\runtime"
  )
  if errorlevel 1 (
    echo [RUNTIME CACHE] CMake pertenece a otra copia/version. Limpiando SOLO runtime_build...
    rmdir /s /q "!RB!"
  )
)
if not exist "!RB!" mkdir "!RB!" >nul 2>nul
cmake -S "%CD%\runtime" -B "!RB!" -A x64 -DCMAKE_SUPPRESS_REGENERATION=ON -DDCR_RUNTIME_OUTPUT_DIR="!CB!"
if errorlevel 1 exit /b 6

set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple dcr_gdrom"
rem A singular plugin rebuild can stay isolated; combinations deliberately rebuild the full modular set.
if "%REBUILD_RUNTIME%"=="0" if "%REBUILD_NETWORK%"=="1" if "%REBUILD_MAPLE%"=="0" if "%REBUILD_GDROM%"=="0" set "RUNTIME_TARGETS=dcr_network"
if "%REBUILD_RUNTIME%"=="0" if "%REBUILD_NETWORK%"=="0" if "%REBUILD_MAPLE%"=="1" if "%REBUILD_GDROM%"=="0" set "RUNTIME_TARGETS=dcr_maple"
if "%REBUILD_RUNTIME%"=="0" if "%REBUILD_NETWORK%"=="0" if "%REBUILD_MAPLE%"=="0" if "%REBUILD_GDROM%"=="1" set "RUNTIME_TARGETS=dcr_gdrom"
rem A version/stamp change always rebuilds the complete runtime set once.
if not exist "!RUNTIME_STAMP!" set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple dcr_gdrom"
if exist "!RUNTIME_STAMP!" (
  findstr /X /C:"%DCR_RUNTIME_BUILD_ID%" "!RUNTIME_STAMP!" >nul 2>nul
  if errorlevel 1 set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple dcr_gdrom"
)
rem Always rebuild all runtime dependencies when source/binary manifest diverges.
if "!RUNTIME_HASH_MISMATCH!"=="1" set "RUNTIME_TARGETS=DreamcastRuntime dcr_network dcr_maple dcr_gdrom"
if "!RUNTIME_HASH_MISMATCH!"=="1" (
  echo [RUNTIME VERIFY] Recompilacion limpia SOLO del runtime. El guest SH-4 permanece intacto.
  cmake --build "!RB!" --config Release --target !RUNTIME_TARGETS! --clean-first --parallel 1
) else (
  cmake --build "!RB!" --config Release --target !RUNTIME_TARGETS! --parallel 1
)
if errorlevel 1 exit /b 7
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" runtime-verify write "%CD%\runtime" "!CB!" "!DCR_RUNTIME_MANIFEST!"
) else (
  python "tools\runtime_cache\verify_runtime_artifacts.py" write "%CD%\runtime" "!CB!" "!DCR_RUNTIME_MANIFEST!"
)
if errorlevel 1 exit /b 7
>"!RUNTIME_STAMP!" echo %DCR_RUNTIME_BUILD_ID%
echo [RUNTIME] Actualizado sin configurar, compilar ni relinkear dcr_game_module.dll.
if "%REBUILD_NETWORK%"=="1" echo [MODULAR] dcr_network.dll actualizado; guest SH-4 intacto.
if "%REBUILD_MAPLE%"=="1" echo [MODULAR] dcr_maple.dll ABI2 actualizado; guest SH-4 intacto.
if "%REBUILD_GDROM%"=="1" echo [MODULAR] dcr_gdrom.dll ABI2 actualizado; guest SH-4 intacto.
if "%REBUILD_RUNTIME%"=="1" echo [MODULAR] DreamcastRuntime + plugins actualizados; guest SH-4 intacto.

:execute_runner
rem Compile only newly discovered, verified SH-4 entrypoints into independent
rem ABI1 native DLLs. The original dcr_game_module.dll/build is never modified.
rem Fail open: the interpreter continues if a patch is rejected by MSVC.
set "INCREMENTAL_INPUT=!EXTERNAL_BOOTSTRAP!"
if defined DCR_SPECIAL_BOOT_FILE if exist "!INTERP_SEED_FILE!" (
  build\Release\dc_disc_probe.exe "%DISC%" --extract-file=!DCR_SPECIAL_BOOT_FILE!="%PROBE%\!DCR_SPECIAL_BOOT_FILE!"
  if not errorlevel 1 set "INCREMENTAL_INPUT=%PROBE%\!DCR_SPECIAL_BOOT_FILE!"
)
set "DCR_SH4_BOOT_SHA256="
set "DCR_SH4_INCREMENTAL_DIR=!DCR_GAME_DIR!\incremental\native"
if "!NATIVE_PATCH_LOAD!"=="0" set "DCR_SH4_INCREMENTAL_DIR="
set "DCR_SH4_PATCH_IMAGE=!INCREMENTAL_INPUT!"
set "DCR_SH4_PATCH_BASE=!DCR_RAW_BASE!"
if "!SH4_DYNAMIC_AOT!"=="1" (set "DYNAMIC_AOT_FLAG=--dynamic") else (set "DYNAMIC_AOT_FLAG=")
if "!NATIVE_SEEDS!"=="1" if "!GUEST_CACHE_VALID!"=="1" (
  echo [NATIVE SEEDS] Resolviendo nuevas entradas sin recompilar el guest base...
  if exist "%DCR_NATIVE_TOOLS%" (
    "%DCR_NATIVE_TOOLS%" native-seeds ^
      --root "%CD%" --game-dir "!DCR_GAME_DIR!" ^
      --image "!INCREMENTAL_INPUT!" --base=!DCR_RAW_BASE! !DYNAMIC_AOT_FLAG!
  ) else (
    if "!SH4_DYNAMIC_AOT!"=="1" echo [DCR SH4-DYNAMIC] WARN: falta dcr_native_tools.exe; capturas guardadas pero DLL dinamica pendiente.
    python "tools\incremental\build_native_patches.py" ^
      --root "%CD%" --game-dir "!DCR_GAME_DIR!" ^
      --image "!INCREMENTAL_INPUT!" --base=!DCR_RAW_BASE!
  )
  if errorlevel 1 echo [NATIVE SEEDS] WARN: compilacion incremental no disponible; seguimos con el interprete.
)
rem Boot hash is checked at DLL load time so patches from a different disc
rem cannot register SH-4 code. Boot image is already staged from this CDI.
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" sha256 "!INCREMENTAL_INPUT!" >"!DCR_GAME_DIR!\incremental_boot_hash.txt"
) else (
  python -c "import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],'rb').read()).hexdigest())" "!INCREMENTAL_INPUT!" >"!DCR_GAME_DIR!\incremental_boot_hash.txt"
)
if exist "!DCR_GAME_DIR!\incremental_boot_hash.txt" set /p DCR_SH4_BOOT_SHA256=<"!DCR_GAME_DIR!\incremental_boot_hash.txt"
if "!AUTO_PORTABLE_SYNC!"=="1" (
  if "!GUEST_PORTABLE!"=="1" (
    call :sync_portable
    if errorlevel 1 (
      echo [PORTABLE] ERROR: no se pudo crear la portable. El guest NO fue recompilado.
      if "!SYNC_ONLY!"=="1" exit /b 13
    )
  ) else (
    echo [PORTABLE] WARN: guest ABI1 valido pero aun no marcado portable-safe; no se exporta.
    if "!SYNC_ONLY!"=="1" exit /b 13
  )
)
if "!SYNC_ONLY!"=="1" (
  echo [PORTABLE] Sync-only completado. No se inicia el juego.
  if exist "%PROBE%" rmdir /s /q "%PROBE%" ^>nul 2^>nul
  exit /b 0
)
echo.
if "!GUEST_CACHE_VALID!"=="1" (echo [GUEST CACHE] Ejecutando modulo SH-4 ABI v%DCR_GUEST_ABI% de !DCR_GAME_SLUG!.) else (echo [6/6] Ejecutando !DCR_MODE!...)
echo [INFO] Controles: flechas=D-pad, WASD=analogico, Z/J/Space=A, X/K=B, C/U=X, V/I=Y, Enter=START.
echo.

set "EXE=!CB!\Release\dreamcast_program.exe"
if not exist "!EXE!" (
  echo [ERROR] No se genero !EXE!
  exit /b 7
)

set "ENTRY_ARG="
if defined DCR_DIRECT_GAME_ENTRY set "ENTRY_ARG=--direct-game-entry=!DCR_DIRECT_GAME_ENTRY!"
set "CT2_ARG="
if "!DCR_CT2_COMPAT!"=="1" set "CT2_ARG=--ct2-compat"
set "DCR_EXTERNAL_RAW_IMAGE="
set "DCR_EXTERNAL_RAW_BASE="
if "!GUEST_PORTABLE!"=="1" (
  for %%I in ("!EXTERNAL_BOOTSTRAP!") do set "DCR_EXTERNAL_RAW_IMAGE=%%~fI"
  set "DCR_EXTERNAL_RAW_BASE=!DCR_RAW_BASE!"
  echo [PORTABLE] Bootstrap externo via Runtime ENV: !DCR_EXTERNAL_RAW_IMAGE! @ !DCR_EXTERNAL_RAW_BASE!
)
set "BOOT_ARG="
if "!DCR_COMMERCIAL_BOOT!"=="1" set "BOOT_ARG=--commercial-boot --commercial-boot-stage=!DCR_BOOT_STAGE!"
set "DCR_PLUGIN_DIR=!CB!\plugins\Release"
set "DCR_MAPLE_TRACE=!MAPLE_TRACE!"
set "DCR_GDROM_TRACE=!GDROM_TRACE!"
echo [PLUGIN] Dir: !DCR_PLUGIN_DIR!
if "!MAPLE_TRACE!"=="1" echo [MAPLE] Trace verbose activado.
if "!GDROM_TRACE!"=="1" echo [GDROM] Trace verbose activado.

"!EXE!" ^
  !ENTRY_ARG! !BOOT_ARG! !CT2_ARG! --disc-map="!OUT!\disc.map" ^
  --pvr-window --pvr-frame-sync !GPU_ARGS! ^
  --fast-dispatch --direct-dispatch --sh4-tick-batch=256 ^
  --maple-host-input !PADARG! !AUDIO_ARGS! !CLOCK_ARGS! !EXTRA_RUN! ^
  --diag-heartbeat-ms=!HEARTBEAT!

set "RC=!ERRORLEVEL!"
echo.
echo [INFO] DreamcastRecomp termino con RC=!RC!.
echo [INFO] Perfil persistente: !DCR_PROFILE_CFG!
echo [INFO] Cache por juego    : !DCR_GAME_DIR!
if "!RC!"=="130" echo [INFO] RC=130 = cierre manual de la ventana.
if exist "%PROBE%" rmdir /s /q "%PROBE%" >nul 2>nul
exit /b !RC!

:sync_portable
if exist "%DCR_NATIVE_TOOLS%" (
  "%DCR_NATIVE_TOOLS%" portable-sync ^
    --root "%CD%" --game-dir "!DCR_GAME_DIR!" --build-dir "!CB!" ^
    --slug "!DCR_GAME_SLUG!" --title "!TITLE!" ^
    --abi "%DCR_GUEST_ABI%" --runtime-id "%DCR_RUNTIME_BUILD_ID%"
) else (
  python "tools\portable\sync_portable.py" ^
    --root "%CD%" --game-dir "!DCR_GAME_DIR!" --build-dir "!CB!" ^
    --slug "!DCR_GAME_SLUG!" --title "!TITLE!" ^
    --abi "%DCR_GUEST_ABI%" --runtime-id "%DCR_RUNTIME_BUILD_ID%"
)
if errorlevel 1 (
  echo [PORTABLE] WARN: auto-sync no pudo completarse; el juego normal puede seguir ejecutandose.
  exit /b 13
)
exit /b 0

:normalize_renderer
if /I "!RENDERER!"=="AUTO" set "RENDERER=auto"
if /I "!RENDERER!"=="D3D11" set "RENDERER=dx11"
if /I "!RENDERER!"=="DIRECT3D11" set "RENDERER=dx11"
if /I "!RENDERER!"=="DIRECTX11" set "RENDERER=dx11"
if /I "!RENDERER!"=="DX11" set "RENDERER=dx11"
if /I "!RENDERER!"=="SW" set "RENDERER=software"
if /I "!RENDERER!"=="CPU" set "RENDERER=software"
if /I "!RENDERER!"=="SOFTWARE" set "RENDERER=software"
if /I "!RENDERER!"=="VK" set "RENDERER=vulkan"
if /I "!RENDERER!"=="D3D12" set "RENDERER=dx12"
if /I "!RENDERER!"=="D3D10" set "RENDERER=dx10"
if /I "!RENDERER!"=="D3D9" set "RENDERER=dx9"

if /I "!RENDERER!"=="vulkan" goto :renderer_not_implemented
if /I "!RENDERER!"=="dx12" goto :renderer_not_implemented
if /I "!RENDERER!"=="dx10" goto :renderer_not_implemented
if /I "!RENDERER!"=="dx9" goto :renderer_not_implemented
if /I not "!RENDERER!"=="auto" if /I not "!RENDERER!"=="dx11" if /I not "!RENDERER!"=="software" (
  echo [ERROR] Renderer desconocido: !RENDERER!
  exit /b 8
)
exit /b 0

:renderer_not_implemented
echo.
echo [ERROR] El backend !RENDERER! esta reservado pero todavia no esta implementado.
echo [INFO] Disponibles hoy: dx11 y software.
exit /b 8

:usage
echo.
echo DreamcastRecomp %DCR_VERSION%
echo.
echo Uso:
echo   %~nx0 "juego.cdi" [opciones]
echo.
echo Perfiles:
echo   La primera ejecucion crea un cache persistente en %%LOCALAPPDATA%%\DreamcastRecompiled\games.
echo   ChuChu, CT2, Daytona, Lodoss y SGGG reciben defaults conocidos.
echo   Juegos nuevos reciben un CFG GENERIC editable sin tocar otros juegos.
echo   Limites SH-4 por juego: max_functions, max_closure_entries, closure_passes.
echo   SGGG recibe defaults 10240/7680; otros juegos conservan los propios.
echo.
echo Opciones:
echo   --perf            Perfil de rendimiento de bajo overhead.
echo   --debug           Diagnostico adicional.
echo   --heartbeat-on    Activa heartbeat cada 1 s ^(por defecto OFF^).
echo   --heartbeat-off   Apaga heartbeat, incluso durante Debug.
echo   --profile         Perfil detallado PVR.
echo   --renderer=dx11   Fuerza Direct3D 11.
echo   --renderer=software Fuerza renderer CPU.
echo   --gpu             Alias de --renderer=dx11.
echo   --software        Alias de --renderer=software.
echo   --no-audio        Ejecuta AICA/ARM7 sin salida host.
echo   --audio-basic     Activa el modo de audio basico.
echo   --reuse           Exige un guest ABI ya compilado; nunca recompila SH-4.
echo   --native-seeds    Compila solo seeds nuevas como DLL ABI1; default: ON.
echo   --dynamic-aot     Captura y compila snapshots RAM ^(experimental, por defecto OFF^).
echo   --bad-target-trace Registros y opcodes al saltar a un target invalido.
echo   --no-native-seeds Desactiva compilacion de seeds; DLLs cacheadas siguen activas.
echo   --no-native-patches Desactiva build y carga de DLLs incrementales.
echo   --no-recompile    Alias de --reuse.
echo   --recompile-game  Fuerza regenerar/recompilar el modulo SH-4.
echo   --rebuild-runtime Recompila SOLO DreamcastRuntime/plugins; jamas configura el guest.
echo   --rebuild-network Recompila solo dcr_network.dll sin tocar el guest.
echo   --rebuild-maple   Recompila solo dcr_maple.dll ABI2/V3 sin tocar el guest.
echo   --rebuild-gdrom   Recompila solo dcr_gdrom.dll ABI2 sin tocar el guest.
echo   --maple-trace     Activa DCR_MAPLE_TRACE=1 para DMA/device routing A-D.
echo   --gdrom-trace     Activa DCR_GDROM_TRACE=1 para DiscMap/TOC/sesiones/lecturas.
echo   --code-reloc-trace Registra reubicaciones SH-4 ^(por defecto OFF^).
echo   --no-interp       Desactiva el interprete SH-4: un objetivo sin AOT vuelve a abortar.
echo   --interp-trace    Registra cada entrada SH-4 nueva que se interpreta.
echo   --fpu-superblock  Fuerza FPU superblock historico; default historico.
echo   --fpu-regionplus  A/B con Region+ sin recompilar el guest.
echo   --fpu-metrics     Activa telemetria FPU caliente; default OFF.
echo   --hotpc           Activa el profiler HOTPC heredado del profiler anterior para diagnostico.
echo   --hotpc-off       Desactiva HOTPC (recomendado para medir FPS limpio).
echo   --hot-recovery-on  Activa especializacion historica para ChuChu ^(default^).
echo   --hot-recovery-off Ejecuta sin aplicar/migrar la especializacion historica.
echo   --sync-524        A/B con host sync 524288; default 262144.
echo   --clean           Borra el cache guest/build de este titulo.
echo   --portableize     Migra UNA vez un guest legado al formato portable-safe.
echo   --no-portable-sync No refresca automaticamente carpeta/ZIP portable.
echo   --help, -h        Muestra esta ayuda.
echo.
echo Tambien puedes arrastrar un archivo .cdi sobre run_game.bat.
exit /b 1

:usage_ok
call :usage
exit /b 0
