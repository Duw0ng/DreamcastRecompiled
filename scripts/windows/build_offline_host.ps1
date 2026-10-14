# Offline, reproducible host build. Does not install or download Python/pip.
param([string]$PythonCommand = '', [string]$PreviousFolder = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$python = $null
if ($PythonCommand) {
  $python = $PythonCommand
} elseif ($PreviousFolder -and (Test-Path (Join-Path $PreviousFolder '.dcr_builder\Python312\python.exe'))) {
  $python = Join-Path $PreviousFolder '.dcr_builder\Python312\python.exe'
} elseif (Test-Path (Join-Path $root '.dcr_builder\Python312\python.exe')) {
  $python = Join-Path $root '.dcr_builder\Python312\python.exe'
} else {
  # Reuse Python from an older release located next to this folder.
  $parent = Split-Path -Parent $root
  $other = Get-ChildItem -LiteralPath $parent -Directory -ErrorAction SilentlyContinue |
    ForEach-Object { Join-Path $_.FullName '.dcr_builder\Python312\python.exe' } |
    Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
  if ($other) { $python = $other }
}
if (-not $python) {
  $c = Get-Command python.exe -ErrorAction SilentlyContinue
  if ($c) { $python = $c.Source }
}
if (-not $python) { throw 'No se encontro Python local. Usá el ZIP final Windows x64 compilado por GitHub Actions.' }
Write-Host ('Python existente: ' + $python) -ForegroundColor Cyan
# In Windows PowerShell, quoting nested strings passed to python -c can strip
# Python's quotes and generate a misleading SyntaxError. Run a .py probe instead.
$probePath = Join-Path ([IO.Path]::GetTempPath()) ('dcr_probe_' + [Guid]::NewGuid().ToString('N') + '.py')
$probeCode = @'
import sys
try:
    import tkinter
except ImportError as exc:
    print('Tkinter no disponible:', exc)
    sys.exit(3)
if sys.version_info[:2] != (3, 12):
    print('Python incorrecto:', sys.version)
    sys.exit(4)
print('Python:', sys.version)
print('Tkinter:', tkinter.TkVersion)
'@
try {
  [IO.File]::WriteAllText($probePath, $probeCode, [Text.Encoding]::UTF8)
  & $python $probePath
  if ($LASTEXITCODE -ne 0) { throw 'Python 3.12 con tkinter no disponible en la instalacion seleccionada.' }
} finally {
  Remove-Item -LiteralPath $probePath -ErrorAction SilentlyContinue
}
& $python -m PyInstaller --version
if ($LASTEXITCODE -ne 0) {
  throw 'PyInstaller no esta disponible en ese Python. Usa una instalacion anterior completa o construye el ZIP con GitHub Actions (no descarga nada para el tester).'
}
$build = Join-Path $root '.build_offline'
New-Item -Force -ItemType Directory -Path $build | Out-Null
$dist = Join-Path $build 'dist'
$work = Join-Path $build 'work'
$spec = Join-Path $build 'spec'
$args = @('-m','PyInstaller','--noconfirm','--clean','--onedir','--windowed',
          '--noupx','--name','DreamcastRecompiled','--distpath',$dist,
          '--workpath',$work,'--specpath',$spec,
          '--icon',(Join-Path $root 'launcher\assets\DreamcastRecompiled.ico'),
          (Join-Path $root 'launcher\offline_host.py'))
Push-Location $root
try { & $python @args; if ($LASTEXITCODE -ne 0) { throw 'PyInstaller no pudo construir el host.' } }
finally { Pop-Location }
$bundle = Join-Path $dist 'DreamcastRecompiled'
if (-not (Test-Path (Join-Path $bundle 'DreamcastRecompiled.exe'))) { throw 'Falta el EXE de salida.' }
if (-not (Test-Path (Join-Path $bundle '_internal\python312.dll'))) { throw 'Falta el motor Python 3.12 offline.' }
if (-not (Test-Path (Join-Path $bundle '_internal\_tkinter.pyd'))) { throw 'Falta Tkinter en el bundle offline.' }
$bundleLib = Join-Path $bundle '_internal'
foreach ($dllName in @('tcl86t.dll','tk86t.dll')) {
  if (-not (Get-ChildItem -LiteralPath $bundleLib -File -Recurse -ErrorAction SilentlyContinue | Where-Object Name -EQ $dllName | Select-Object -First 1)) {
    throw ('Falta ' + $dllName + ' en el paquete offline. Verifica que Tcl/Tk este instalado junto a Python 3.12.')
  }
}

$target = Join-Path $root '_internal'
$staging = Join-Path $root '_internal_new'
$backup = Join-Path $build '_internal_previous'
if (Test-Path $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
if (Test-Path $backup) { Remove-Item -LiteralPath $backup -Recurse -Force }
Copy-Item -LiteralPath (Join-Path $bundle '_internal') -Destination $staging -Recurse -Force
if (Test-Path $target) { Move-Item -LiteralPath $target -Destination $backup }
try { Move-Item -LiteralPath $staging -Destination $target }
catch {
  if (Test-Path $backup) { Move-Item -LiteralPath $backup -Destination $target }
  throw
}
Copy-Item -LiteralPath (Join-Path $bundle 'DreamcastRecompiled.exe') -Destination (Join-Path $root 'DreamcastRecompiled.exe') -Force
# Include upstream redistribution notices when available in local Python/Tcl/Tk.
$licenseDir = Join-Path $root 'licenses'
New-Item -Path $licenseDir -ItemType Directory -Force | Out-Null
$pyBase = (& $python -c 'import sys; print(sys.base_prefix)' | Select-Object -Last 1).Trim()
$pyLicense = Join-Path $pyBase 'LICENSE.txt'
if (Test-Path $pyLicense) { Copy-Item -LiteralPath $pyLicense -Destination (Join-Path $licenseDir 'CPYTHON_LICENSE.txt') -Force }
foreach ($spec in @(@('tcl8.6','TCL_LICENSE.txt'),@('tk8.6','TK_LICENSE.txt'))) {
  $license = Join-Path (Join-Path (Join-Path $pyBase 'tcl') $spec[0]) 'license.terms'
  if (Test-Path $license) { Copy-Item -LiteralPath $license -Destination (Join-Path $licenseDir $spec[1]) -Force }
}
$manifest = @{ host_abi=1; python_major_minor='3.12'; packaged_offline=$true; source_launcher_external=$true }
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'launcher_offline_runtime.json') -Encoding UTF8
$checkFile = Join-Path $root '.offline_check.json'
if (Test-Path $checkFile) { Remove-Item -LiteralPath $checkFile -Force }
$run = Start-Process -FilePath (Join-Path $root 'DreamcastRecompiled.exe') -ArgumentList '--self-check' -PassThru -Wait
if ($run.ExitCode -ne 0) { throw ('El host offline reporto error: ' + $run.ExitCode) }
if (-not (Test-Path (Join-Path $root '.offline_check.json'))) { throw 'No se pudo validar el host offline.' }
$check = Get-Content -Raw -LiteralPath (Join-Path $root '.offline_check.json') | ConvertFrom-Json
if ($check.missing.Count -ne 0) { throw 'Paquete incompleto (faltan recursos Aqua).' }
Remove-Item -Force (Join-Path $root '.offline_check.json')
Write-Host '[OK] Python 3.12 / Tcl-Tk embebidos. Se pueden actualizar los .py sin recompilar.' -ForegroundColor Green
