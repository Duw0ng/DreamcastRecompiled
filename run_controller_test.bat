@echo off
setlocal
cd /d "%~dp0"
where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo [ERROR] PowerShell no esta disponible.
  pause
  exit /b 1
)
echo DreamcastRecomp v0.1.1 Official - Controller Visual Test
echo.
set "TESTER_PS1=%~dp0tools\controller_test\DreamcastControllerTest.ps1"

echo [1/2] Validando sintaxis del tester...
if not exist "%TESTER_PS1%" (
  echo [ERROR] No se encontro el tester:
  echo         %TESTER_PS1%
  pause
  exit /b 2
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$p=$env:TESTER_PS1; if([string]::IsNullOrWhiteSpace($p) -or -not (Test-Path -LiteralPath $p -PathType Leaf)){ Write-Host ('[PARSE ERROR] Ruta invalida o archivo inexistente: ' + $p) -ForegroundColor Red; exit 2 }; $tokens=$null; $parseErrors=$null; $null=[System.Management.Automation.Language.Parser]::ParseFile($p,[ref]$tokens,[ref]$parseErrors); if(@($parseErrors).Count -gt 0){ foreach($e in $parseErrors){ Write-Host ('[PARSE ERROR] ' + $e.Message) -ForegroundColor Red }; exit 2 }"
if errorlevel 1 (
  echo.
  echo [ERROR] El tester tiene un error de sintaxis y no se ejecutara.
  pause
  exit /b 2
)

echo [2/2] Abriendo tester...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%TESTER_PS1%" -ProfilePath "%~dp0profiles\controller_profile.ini"
if errorlevel 1 (
  echo.
  echo [ERROR] El tester termino con codigo %errorlevel%.
  pause
)
endlocal
