$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Write-Host 'Dreamcast Recompiled v1.2 AQUA RC7 - Auditoria offline' -ForegroundColor Cyan
Write-Host ('Windows x64: ' + [Environment]::Is64BitOperatingSystem)
foreach ($file in @('launcher\launch.py', 'launcher\main.py',
    'launcher\assets\water_night_1152.png', 'launcher\assets\water_starry_1152.png',
    'ABRIR_LAUNCHER.bat', 'COMPILAR_LAUNCHER.bat', 'scripts\windows\run_game.bat')) {
    $found = Test-Path -LiteralPath (Join-Path $root $file)
    Write-Host ($(if($found){'[OK]'}else{'[FALTA]'}) + ' ' + $file) -ForegroundColor $(if($found){'Green'}else{'Yellow'})
}
$exe = Join-Path $root 'DreamcastRecompiled.exe'
$dll = Join-Path $root '_internal\python312.dll'
if ((Test-Path $exe) -and (Test-Path $dll)) {
    Write-Host '[OK] Host portable offline Windows presente' -ForegroundColor Green
    & $exe '--self-check'
    $selfCheck = Join-Path $root '.offline_check.json'
    if (Test-Path $selfCheck) {
        Get-Content $selfCheck
        Remove-Item $selfCheck -Force
    }
} else {
    Write-Host '[INFO] Paquete FUENTE: host y DLLs Windows todavia no generados.' -ForegroundColor Yellow
    Write-Host 'Ejecuta COMPILAR_LAUNCHER.bat en Windows o construye por Actions.'
}
