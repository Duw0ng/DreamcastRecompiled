@echo off
setlocal EnableExtensions
set "GAMES=%LOCALAPPDATA%\DreamcastRecompiled\games"
echo DreamcastRecompiled - Runtime CMake cache repair
echo.
echo This deletes ONLY runtime_build_cpp35b1 directories under:
echo   %GAMES%
echo.
echo It does NOT delete generated, build, dcr_game_module.dll, VMU or flash.
echo.
if not exist "%GAMES%" (
  echo [INFO] Games cache directory not found.
  exit /b 0
)
for /d %%G in ("%GAMES%\*") do (
  if exist "%%~fG\runtime_build_cpp35b1" (
    echo [FIX] Removing "%%~fG\runtime_build_cpp35b1"
    rmdir /s /q "%%~fG\runtime_build_cpp35b1"
  )
)
echo [OK] Runtime-only CMake cache cleared.
exit /b 0
