@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0..\..\"

set "CPP=generated\official_v0.1.1\cpp"
set "CB=_cb_v0.1.1"
set "PATCH=hotpatch_cpp22\dc_runtime.cpp"
set "TARGET=%CPP%\dc_runtime.cpp"

 echo ============================================================
 echo  DreamcastRecomp cpp22 - GPU texture cache HOTPATCH
 echo ============================================================
 echo  Este parche NO regenera ni recompila las funciones SH-4.
 echo  Solo reemplaza dc_runtime.cpp y relinkea dreamcast_program.exe.
 echo ============================================================
 echo.

if not exist "%PATCH%" (
  echo [ERROR] Falta %PATCH%
  exit /b 1
)
if not exist "%TARGET%" (
  echo [ERROR] No existe %TARGET%
  echo [INFO] Necesitas conservar la carpeta generated de tu compilacion actual.
  exit /b 2
)
if not exist "%CB%\CMakeCache.txt" (
  echo [ERROR] No existe %CB%\CMakeCache.txt
  echo [INFO] Necesitas conservar _cb_v0.1.1 de tu compilacion actual.
  exit /b 2
)
if not exist "%CB%\Release\dreamcast_program.exe" (
  echo [ERROR] Falta el runner ya compilado en %CB%\Release\dreamcast_program.exe
  exit /b 2
)

if not exist "%TARGET%.cpp21_backup" (
  echo [1/3] Guardando backup del runtime anterior...
  copy /Y "%TARGET%" "%TARGET%.cpp21_backup" >nul
) else (
  echo [1/3] Backup cpp21 ya existe; se conserva.
)

echo [2/3] Aplicando runtime cpp22...
copy /Y "%PATCH%" "%TARGET%" >nul
if errorlevel 1 (
  echo [ERROR] No pude reemplazar dc_runtime.cpp.
  exit /b 3
)

echo [3/3] Recompilando SOLO runtime + relink...
echo [INFO] Los generated_program_part_XX.obj existentes se reutilizan.
cmake --build "%CB%" --config Release --target dreamcast_program --parallel 1
if errorlevel 1 (
  echo.
  echo [ERROR] Fallo el rebuild incremental.
  echo [INFO] El backup esta en %TARGET%.cpp21_backup
  exit /b 4
)

echo.
echo [OK] Hotpatch cpp22 aplicado.
echo [OK] No se regenero la closure SH-4.
echo.
if not "%~1"=="" (
  echo [RUN] Lanzando el CDI usando --reuse y DX11...
  call "%~dp0..\windows\run_game.bat" "%~f1" --reuse --renderer=dx11
  exit /b !ERRORLEVEL!
)

echo Para probar SIN recompilar el juego:
echo   run_game.bat "C:\ruta\a\ccr.cdi" --reuse --renderer=dx11
 echo.
echo Tambien puedes arrastrar el CDI sobre este BAT para parchear y ejecutarlo directamente.
exit /b 0
