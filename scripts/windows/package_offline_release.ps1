# Assemble a release that includes Python 3.12, tkinter, Tcl/Tk binaries.
# This script NEVER installs or downloads anything.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$internal = Join-Path $root '_internal'
if (-not (Test-Path (Join-Path $root 'DreamcastRecompiled.exe'))) { throw 'Falta DreamcastRecompiled.exe. Ejecuta COMPILAR_LAUNCHER.bat primero.' }
foreach ($path in @('python312.dll','_tkinter.pyd')) {
  if (-not (Test-Path (Join-Path $internal $path))) { throw ('Falta la dependencia Windows: _internal\' + $path) }
}
foreach ($lib in @('tcl86t.dll','tk86t.dll')) {
  $matches = Get-ChildItem -LiteralPath $internal -Recurse -File -ErrorAction SilentlyContinue | Where-Object Name -EQ $lib
  if (-not $matches) { throw ('Falta la DLL de Tcl/Tk: ' + $lib) }
}
$outputDir = Join-Path $root 'dist'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$output = Join-Path $outputDir 'DreamcastRecompiled_V1.2_Stable_Release_WIN64.zip'
$staging = Join-Path $root '.release_staging'
if (Test-Path $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging -Force | Out-Null
$excluded = @('.git','.github','.dcr_builder','.build_offline','.release_staging','dist',
              '__pycache__','.pytest_cache','build','.offline_check.json','games','system-data')
try {
  Get-ChildItem -LiteralPath $root -Force | Where-Object { $_.Name -notin $excluded } | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $staging -Recurse -Force
  }
  # Only the verified, game-specific native binaries are bundled. Never copy
  # user-provided CDI/GDI images from arbitrary games/ paths.
  $bundle = Join-Path $root 'games\CHUCHU_ROCKET'
  if (Test-Path (Join-Path $bundle 'portable_manifest.txt')) {
    $dest = Join-Path $staging 'games\CHUCHU_ROCKET'
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
    foreach ($name in @('portable_manifest.txt','checksums.sha256')) {
      Copy-Item -LiteralPath (Join-Path $bundle $name) -Destination $dest -Force
    }
    $bin = Join-Path $dest 'bin'
    New-Item -ItemType Directory -Path $bin -Force | Out-Null
    foreach ($name in @('dreamcast_program.exe','dcr_game_module.dll','DreamcastRuntime.dll','dc_disc_probe.exe','dc_boot_prepare.exe')) {
      Copy-Item -LiteralPath (Join-Path $bundle ('bin\'+$name)) -Destination $bin -Force
    }
    Copy-Item -LiteralPath (Join-Path $bundle 'bin\plugins') -Destination $bin -Recurse -Force
  }
  if (Test-Path $output) { Remove-Item -LiteralPath $output -Force }
  Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $output -CompressionLevel Optimal
  if (-not (Test-Path $output)) { throw 'No se genero el ZIP offline.' }
  Write-Host ('[OK] Windows x64 OFFLINE: ' + $output) -ForegroundColor Green
} finally {
  if (Test-Path $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
}
