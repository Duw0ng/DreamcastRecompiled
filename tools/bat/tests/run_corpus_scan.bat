@echo off
setlocal
set "DCR_ROOT=%~dp0..\..\.."
cd /d "%DCR_ROOT%"
set "PATH=%DCR_ROOT%\tools\bat\tests;%DCR_ROOT%\tools\bat\homebrew;%DCR_ROOT%\tools\bat\probes;%DCR_ROOT%\tools\bat\legacy;%DCR_ROOT%\tools\bat\dev;%PATH%"
set "CORPUS=%~1"
if "%CORPUS%"=="" set "CORPUS=corpus"

if not exist "build\Release\dc_corpus_scan.exe" (
    echo [INFO] Compilando DreamcastRecomp primero...
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

if not exist "%CORPUS%" (
    echo [ERROR] No existe el corpus: %CORPUS%
    echo.
    echo Uso:
    echo   run_corpus_scan.bat C:\ruta\a\dreamcast
    echo.
    echo Tambien puedes crear una carpeta "corpus" junto a este BAT y copiar alli los ELF.
    exit /b 2
)

if not exist "generated" mkdir generated

echo ========================================
echo  DreamcastRecomp 0.0.170 - Corpus Scanner
echo ========================================
echo Corpus: %CORPUS%
echo.

"build\Release\dc_corpus_scan.exe" "%CORPUS%" ^
  --text "generated\kos_corpus_report.txt" ^
  --csv "generated\kos_corpus_report.csv" ^
  --top 60

if errorlevel 1 exit /b %errorlevel%

echo.
echo [OK] Escaneo terminado.
echo Reportes:
echo   generated\kos_corpus_report.txt
echo   generated\kos_corpus_report.csv
endlocal
