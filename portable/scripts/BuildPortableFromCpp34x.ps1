param(
    [Parameter(Mandatory=$true)]
    [string]$Cpp34xRoot,

    [string]$OutputDirectory = "$PSScriptRoot\out",

    [string]$LauncherSourceDirectory = "$PSScriptRoot\..\launcher"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Fail([string]$Message) {
    Write-Host "[ERROR] $Message" -ForegroundColor Red
    exit 1
}

function Info([string]$Message) {
    Write-Host "[PORTABLE] $Message" -ForegroundColor Cyan
}

function Find-NewestFile {
    param(
        [string[]]$Roots,
        [string]$Name
    )
    $matches = @()
    foreach ($root in $Roots) {
        if ([string]::IsNullOrWhiteSpace($root) -or !(Test-Path $root)) { continue }
        $matches += Get-ChildItem -LiteralPath $root -Filter $Name -File -Recurse -ErrorAction SilentlyContinue
    }
    return $matches | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Invoke-Checked {
    param(
        [string]$Exe,
        [string[]]$Arguments,
        [string]$WorkingDirectory
    )
    Push-Location $WorkingDirectory
    try {
        & $Exe @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "$Exe returned $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

$Cpp34xRoot = (Resolve-Path $Cpp34xRoot).Path
if (!(Test-Path $Cpp34xRoot)) { Fail "Cpp34xRoot no existe: $Cpp34xRoot" }
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "CMake no está disponible. Este paso es SOLO para quien prepara la release; los testers no necesitarán CMake."
}

$LocalRoot = Join-Path $env:LOCALAPPDATA "DreamcastRecompiled"
$GameRoot  = Join-Path $LocalRoot "games\CHUCHU_ROCKET"
if (!(Test-Path $GameRoot)) {
    Fail "No encuentro el cache CHUCHU_ROCKET en $GameRoot. Ejecuta cpp34x al menos una vez con tu copia original."
}

$Generated = Join-Path $GameRoot "generated\cpp"
if (!(Test-Path (Join-Path $Generated "dcr_game_exports.cpp"))) {
    $candidate = Get-ChildItem -LiteralPath $GameRoot -Filter dcr_game_exports.cpp -File -Recurse -ErrorAction SilentlyContinue |
                 Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if (!$candidate) {
        Fail "No encuentro dcr_game_exports.cpp en el cache. Ejecuta cpp34x una vez para generar el Guest ABI1."
    }
    $Generated = $candidate.Directory.FullName
}

$Bootstrap = Find-NewestFile -Roots @($GameRoot) -Name "BOOTSTRAP.BIN"
if (!$Bootstrap) {
    Fail "No encuentro BOOTSTRAP.BIN local. Ejecuta cpp34x una vez; este archivo NO se incluirá en la release."
}

$KnownSeeds = @(
    "0x8C036102",
    "0x8C036118",
    "0x8C036160",
    "0x8C036236",
    "0x8C036280",
    "0x8C036526"
)

$SeedFile = Join-Path $GameRoot "late_aot\late_target_seeds.txt"
if (!(Test-Path $SeedFile)) {
    Fail "No encuentro $SeedFile. Ejecuta cpp34x una vez para construir el overlay conocido."
}
$seedText = (Get-Content -LiteralPath $SeedFile -Raw).ToUpperInvariant()
foreach ($seed in $KnownSeeds) {
    if (!$seedText.Contains($seed.ToUpperInvariant())) {
        Fail "El overlay todavía no contiene $seed. Ejecuta TEST_CPP34X_AUTO_LATE_AOT_ONLINE.bat una vez y vuelve a ejecutar este packager."
    }
}

$GuestPatch = Find-NewestFile -Roots @($GameRoot) -Name "dcr_guest_patch.dll"
if (!$GuestPatch) {
    Fail "No encuentro dcr_guest_patch.dll precompilado en el cache."
}

$Work = Join-Path $env:TEMP "DreamcastRecompiled_PortableGuest"
if (Test-Path $Work) { Remove-Item -LiteralPath $Work -Recurse -Force }
New-Item -ItemType Directory -Path $Work | Out-Null
$PortableSource = Join-Path $Work "guest"
Copy-Item -LiteralPath $Generated -Destination $PortableSource -Recurse

$Exports = Join-Path $PortableSource "dcr_game_exports.cpp"
$CMake   = Join-Path $PortableSource "CMakeLists.txt"
if (!(Test-Path $Exports) -or !(Test-Path $CMake)) {
    Fail "El guest generado no tiene la estructura modular esperada."
}

Info "Auditando y convirtiendo Guest ABI1 a external-image mode..."

$exportsText = Get-Content -LiteralPath $Exports -Raw
if (!$exportsText.Contains("load_embedded_elf_image")) {
    Fail "dcr_game_exports.cpp no contiene load_embedded_elf_image; no aplicaré un parche ambiguo."
}

$exportsText = [regex]::Replace(
    $exportsText,
    '(?m)^\s*#include\s+"dc_image\.hpp"\s*\r?\n',
    ''
)

$loader = @'
#include <cstdlib>
#include <fstream>
#include <limits>

static bool dcr_portable_load_external_game_image(dcrecomp_generated::DCRuntime& runtime) {
    const char* image_path = std::getenv("DCR_EXTERNAL_GAME_IMAGE");
    if (image_path == nullptr || *image_path == '\0') {
        return false;
    }

    std::ifstream input(image_path, std::ios::binary | std::ios::ate);
    if (!input) return false;

    const std::streamoff end = input.tellg();
    if (end <= 0) return false;
    if (static_cast<unsigned long long>(end) >
        static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    constexpr std::size_t kRawBootMainRamOffset = 0x8000u; // 0x8C008000
    const std::size_t size = static_cast<std::size_t>(end);
    if (runtime.main_ram.size() < kRawBootMainRamOffset ||
        size > runtime.main_ram.size() - kRawBootMainRamOffset) {
        return false;
    }

    input.seekg(0, std::ios::beg);
    input.read(
        reinterpret_cast<char*>(runtime.main_ram.data() + kRawBootMainRamOffset),
        static_cast<std::streamsize>(size)
    );
    return static_cast<std::size_t>(input.gcount()) == size;
}

'@

$firstExport = $exportsText.IndexOf("DCR_GAME_EXPORT")
if ($firstExport -lt 0) {
    Fail "No encuentro DCR_GAME_EXPORT en dcr_game_exports.cpp."
}
$exportsText = $exportsText.Insert($firstExport, $loader)
$exportsText = $exportsText.Replace(
    "load_embedded_elf_image(*runtime);",
    "if (!dcr_portable_load_external_game_image(*runtime)) return false;"
)

Set-Content -LiteralPath $Exports -Value $exportsText -Encoding UTF8

$cmakeText = Get-Content -LiteralPath $CMake -Raw
$beforeCmake = $cmakeText
$cmakeText = [regex]::Replace($cmakeText, '(?m)^[^\r\n]*dc_image\.cpp[^\r\n]*\r?\n', '')
if ($cmakeText -eq $beforeCmake) {
    Fail "No pude retirar dc_image.cpp del CMake del guest."
}
Set-Content -LiteralPath $CMake -Value $cmakeText -Encoding UTF8

$PortableBuild = Join-Path $Work "build"
Invoke-Checked -Exe "cmake" -Arguments @("-S",$PortableSource,"-B",$PortableBuild,"-A","x64") -WorkingDirectory $Work
Invoke-Checked -Exe "cmake" -Arguments @("--build",$PortableBuild,"--config","Release","--target","dcr_game_module","--parallel","1") -WorkingDirectory $Work

$CleanGuest = Find-NewestFile -Roots @($PortableBuild) -Name "dcr_game_module.dll"
if (!$CleanGuest) { Fail "No se generó dcr_game_module.dll external-image." }

Info "Localizando runtime cpp34x/cpp34s ya probado..."
$Runner = Find-NewestFile -Roots @($GameRoot,$Cpp34xRoot) -Name "dreamcast_program.exe"
$Runtime = Find-NewestFile -Roots @($Cpp34xRoot,$GameRoot) -Name "DreamcastRuntime.dll"
$Network = Find-NewestFile -Roots @($Cpp34xRoot,$GameRoot) -Name "dcr_network.dll"
$Probe = Find-NewestFile -Roots @($Cpp34xRoot) -Name "dc_disc_probe.exe"
$BootPrepare = Find-NewestFile -Roots @($Cpp34xRoot) -Name "dc_boot_prepare.exe"

foreach ($pair in @(
    @("dreamcast_program.exe",$Runner),
    @("DreamcastRuntime.dll",$Runtime),
    @("dcr_network.dll",$Network),
    @("dc_disc_probe.exe",$Probe),
    @("dc_boot_prepare.exe",$BootPrepare)
)) {
    if (!$pair[1]) { Fail "No encuentro $($pair[0]) dentro de cpp34x/cache." }
}

Info "Compilando launcher Win32 sin consola..."
$LauncherBuild = Join-Path $Work "launcher-build"
Invoke-Checked -Exe "cmake" -Arguments @("-S",$LauncherSourceDirectory,"-B",$LauncherBuild,"-A","x64") -WorkingDirectory $Work
Invoke-Checked -Exe "cmake" -Arguments @("--build",$LauncherBuild,"--config","Release","--parallel","1") -WorkingDirectory $Work
$Launcher = Find-NewestFile -Roots @($LauncherBuild) -Name "DreamcastRecompiled.exe"
if (!$Launcher) { Fail "No se pudo compilar DreamcastRecompiled.exe." }

$Stage = Join-Path $OutputDirectory "DreamcastRecompiled_ChuChu_Online_Preview_v0.1"
if (Test-Path $Stage) { Remove-Item -LiteralPath $Stage -Recurse -Force }
New-Item -ItemType Directory -Path $Stage | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "plugins") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "tools") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "profiles") | Out-Null

Copy-Item $Launcher.FullName (Join-Path $Stage "DreamcastRecompiled.exe")
Copy-Item $Runner.FullName (Join-Path $Stage "dreamcast_program.exe")
Copy-Item $Runtime.FullName (Join-Path $Stage "DreamcastRuntime.dll")
Copy-Item $CleanGuest.FullName (Join-Path $Stage "dcr_game_module.dll")
Copy-Item $Network.FullName (Join-Path $Stage "plugins\dcr_network.dll")
Copy-Item $GuestPatch.FullName (Join-Path $Stage "plugins\dcr_guest_patch.dll")
Copy-Item $Probe.FullName (Join-Path $Stage "tools\dc_disc_probe.exe")
Copy-Item $BootPrepare.FullName (Join-Path $Stage "tools\dc_boot_prepare.exe")

$profile = @"
[ChuChuRocketPortable]
title=CHUCHU ROCKET
product=MK-51049
version=V1.007
ip_sha256=9925c18f0857ccd363cb8d641122f410083b133102ac99f06bae6bb83f9aad4d
boot_sha256=b43cb7977871e0c1ce7971da3da7f5a7ea4eb6903a61fbefc5a50de4230d2927
cdi_sha256=6e95281b2feaa98e8b1327b33d8e60147c329cd314f633bba88eb25229031d3b

[KnownOnlineSeeds]
seed0=0x8C036102
seed1=0x8C036118
seed2=0x8C036160
seed3=0x8C036236
seed4=0x8C036280
seed5=0x8C036526
"@
Set-Content -LiteralPath (Join-Path $Stage "profiles\chuchu_rocket.ini") -Value $profile -Encoding ASCII

$readme = @"
Dreamcast Recompiled - ChuChu Rocket! Online Preview v0.1
=========================================================

1. Extrae esta carpeta.
2. Abre DreamcastRecompiled.exe.
3. Selecciona tu copia original .CDI o .GDI de ChuChu Rocket!.
4. El launcher verifica y extrae localmente IP.BIN / BOOT.BIN / BOOTSTRAP.BIN.
5. Pulsa PLAY.

El ZIP NO incluye CDI, GDI, IP.BIN, BOOT.BIN ni BOOTSTRAP.BIN.
El guest distribuido usa external-image mode: el código SH-4 está precompilado
y los datos originales se leen desde la copia aportada por cada tester.

Opciones disponibles en el launcher:
- Render: Auto / GPU Direct3D 11 / Software CPU
- Audio on/off
- Host clock / Device clock
- Performance mode
- Debug
- PVR profile
- Controller backend: Auto / PS4 native / XInput / WinMM
- Device index
- Deadzone

Online seeds precompiladas:
0x8C036102
0x8C036118
0x8C036160
0x8C036236
0x8C036280
0x8C036526
"@
Set-Content -LiteralPath (Join-Path $Stage "README.txt") -Value $readme -Encoding UTF8

Info "Audit gate: buscando bloques originales de BOOTSTRAP dentro de los DLL distribuibles..."

Add-Type -TypeDefinition @'
using System;
public static class DcrByteAudit {
    public static bool Contains(byte[] hay, byte[] needle) {
        if (hay == null || needle == null || needle.Length == 0 || needle.Length > hay.Length) return false;
        byte first = needle[0];
        int limit = hay.Length - needle.Length;
        for (int i = 0; i <= limit; ++i) {
            if (hay[i] != first) continue;
            int j = 1;
            for (; j < needle.Length; ++j) {
                if (hay[i + j] != needle[j]) break;
            }
            if (j == needle.Length) return true;
        }
        return false;
    }
}
'@

$bootBytes = [System.IO.File]::ReadAllBytes($Bootstrap.FullName)
function Assert-NoBootstrapChunks([string]$DllPath) {
    $dll = [System.IO.File]::ReadAllBytes($DllPath)
    $chunkSize = 4096
    $sampleCount = 16
    for ($i = 0; $i -lt $sampleCount; ++$i) {
        $maxStart = [Math]::Max(0, $bootBytes.Length - $chunkSize)
        $start = [int](($maxStart * $i) / [Math]::Max(1, $sampleCount - 1))
        $chunk = New-Object byte[] $chunkSize
        [Array]::Copy($bootBytes, $start, $chunk, 0, $chunkSize)
        if ([DcrByteAudit]::Contains($dll, $chunk)) {
            throw "AUDIT FAIL: $DllPath contiene un bloque original de 4 KiB de BOOTSTRAP.BIN (offset 0x$($start.ToString('X')))."
        }
    }
}

Assert-NoBootstrapChunks (Join-Path $Stage "dcr_game_module.dll")
Assert-NoBootstrapChunks (Join-Path $Stage "plugins\dcr_guest_patch.dll")

$forbidden = @("*.cdi","*.gdi","IP.BIN","BOOT.BIN","BOOT.DISC.BIN","BOOTSTRAP.BIN","dc_image.cpp")
foreach ($pattern in $forbidden) {
    $found = Get-ChildItem -LiteralPath $Stage -Filter $pattern -File -Recurse -ErrorAction SilentlyContinue
    if ($found) { throw "AUDIT FAIL: archivo comercial/embebido inesperado: $($found.FullName)" }
}

$audit = @"
PORTABLE AUDIT PASS
===================
Guest mode: external image
dc_image.cpp linked: NO
Original game image shipped: NO
4 KiB BOOTSTRAP samples found in dcr_game_module.dll: NO
4 KiB BOOTSTRAP samples found in dcr_guest_patch.dll: NO
Known online seeds: $($KnownSeeds -join ', ')
ABI: existing cpp34x Guest ABI1 (unchanged)
Generated: $(Get-Date -Format o)
"@
Set-Content -LiteralPath (Join-Path $Stage "PORTABLE_AUDIT.txt") -Value $audit -Encoding UTF8

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$Zip = Join-Path $OutputDirectory "DreamcastRecompiled_ChuChu_Online_Preview_v0.1_PORTABLE.zip"
if (Test-Path $Zip) { Remove-Item $Zip -Force }
Compress-Archive -LiteralPath $Stage -DestinationPath $Zip -CompressionLevel Optimal

$sha = (Get-FileHash -LiteralPath $Zip -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath "$Zip.sha256" -Value "$sha  $([IO.Path]::GetFileName($Zip))" -Encoding ASCII

Write-Host ""
Write-Host "[OK] Portable creado:" -ForegroundColor Green
Write-Host "     $Zip"
Write-Host "     SHA-256: $sha"
Write-Host ""
Write-Host "El tester solo necesita DreamcastRecompiled.exe + su propio CDI/GDI."
