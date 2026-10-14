@echo off
setlocal
cd /d "%~dp0..\..\"
set "TARGET=generated\official_v0.1.1\cpp\dc_runtime.cpp"
if not exist "%TARGET%.cpp21_backup" (
  echo [ERROR] No existe backup cpp21.
  exit /b 1
)
copy /Y "%TARGET%.cpp21_backup" "%TARGET%" >nul
if errorlevel 1 exit /b 2
cmake --build "_cb_v0.1.1" --config Release --target dreamcast_program --parallel 1
exit /b %ERRORLEVEL%
