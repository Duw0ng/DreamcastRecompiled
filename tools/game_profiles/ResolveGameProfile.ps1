param(
    [Parameter(Mandatory=$true)][AllowEmptyString()][string]$Title,
    [string]$Volume = "",
    [Parameter(Mandatory=$true)][string]$Disc,
    [string]$Root = "games",
    [Parameter(Mandatory=$true)][string]$EmitCmd
)

$ErrorActionPreference = "Stop"

function Normalize-Token([string]$s) {
    if ([string]::IsNullOrWhiteSpace($s)) { return "" }
    return (($s.Trim().ToUpperInvariant()) -replace '\s+', ' ')
}

function Get-SafeSlug([string]$s) {
    if ([string]::IsNullOrWhiteSpace($s)) {
        $s = [IO.Path]::GetFileNameWithoutExtension($Disc)
    }
    foreach ($c in [IO.Path]::GetInvalidFileNameChars()) {
        $s = $s.Replace([string]$c, "_")
    }
    $s = ($s -replace '[^A-Za-z0-9._ -]', '_')
    $s = ($s -replace '\s+', '_').Trim([char[]]' ._')
    if ([string]::IsNullOrWhiteSpace($s)) { $s = "UNKNOWN_GAME" }
    if ($s.Length -gt 72) { $s = $s.Substring(0,72).TrimEnd([char[]]'_.') }
    return $s
}

function New-Defaults {
    $p = [ordered]@{
        profile_version       = "2"
        title                 = $Title.Trim()
        volume                = $Volume.Trim()
        mode                  = "GENERIC"
        renderer              = "dx11"
        commercial_boot       = "1"
        commercial_boot_stage = "auto"
        direct_game_entry     = ""
        ct2_compat             = "0"
        seed_file              = ""
        entry_seed             = "0x8C010000"
        raw_base               = "0x8C008000"
        raw_entry              = "0x8C008300"
        max_functions          = "8192"
        max_closure_entries    = "6144"
        closure_passes         = "64"
        audio_profile          = "full"
        device_clock           = "host"
        special_boot_file      = ""
        controller_profile     = ""
        extra_run_args         = ""
    }

    $t = Normalize-Token $Title
    $v = Normalize-Token $Volume
    $id = "$t $v"

    if ($id -match 'CHUCHU\s*ROCKET') {
        $p.mode = "CHUCHU"
        $p.renderer = "dx11"
        # cpp21 proved cooked 2048/0 media must stay plain.
        $p.commercial_boot_stage = "plain"
    }
    elseif ($id -match 'CRAZY\s*TAXI\s*2') {
        $p.mode = "CT2"
        $p.renderer = "dx11"
        # v0.1.1 historical CT2 path intentionally restaged the executable.
        $p.commercial_boot_stage = "scrambled"
        $p.ct2_compat = "1"
        $p.seed_file = "profiles\crazy_taxi_2_known_seeds.txt"
    }
    elseif ($id -match 'PIZZICATO\s+POLKA|DAYTONA') {
        $p.mode = "DAYTONA"
        $p.renderer = "software"
        # Regression fix: pre-cpp11 Daytona used --commercial-boot without CT2
        # scramble staging. Keep the old plain behavior explicitly.
        $p.commercial_boot_stage = "plain"
        $p.direct_game_entry = "0x8C010000"
        $p.seed_file = "profiles\daytona_usa_known_seeds.txt"
        $p.entry_seed = ""
        $p.audio_profile = "basic"
        $p.device_clock = "device"
    }
    elseif ($id -match 'RECORD\s+OF\s+LODOSS\s+WAR|LODOSS') {
        $p.mode = "LODOSS"
        $p.renderer = "software"
        $p.commercial_boot_stage = "plain"
        $p.direct_game_entry = "0x8C010000"
        $p.seed_file = "profiles\record_of_lodoss_war_known_seeds.txt"
        $p.entry_seed = ""
        $p.raw_base = "0x8C010000"
        $p.raw_entry = "0x8C010000"
        $p.max_functions = "10000"
        $p.max_closure_entries = "9000"
        $p.special_boot_file = "1NOSDC.BIN"
        $p.audio_profile = "basic"
        $p.device_clock = "device"
    }
    elseif ($id -match 'SEGAGAGA|SEGA\s*GAGA|SGGG') {
        $p.mode = "SGGG"
        # Preserve the cpp17/cpp18 validated software/PVR path first; users can
        # opt into DX11 by editing the per-game cfg or with --renderer=dx11.
        $p.renderer = "software"
        $p.commercial_boot_stage = "scrambled"
        $p.seed_file = "profiles\sggg_known_seeds.txt"
        # SGGG has ~6393 observed reachable functions. Allow closure growth while
        # reserving headroom for ProgramAnalysis' CFG fragments.
        $p.max_functions = "10240"
        $p.max_closure_entries = "7680"
    }

    return $p
}

function Read-Cfg([string]$path, $defaults) {
    $p = [ordered]@{}
    foreach ($k in $defaults.Keys) { $p[$k] = [string]$defaults[$k] }
    if (Test-Path -LiteralPath $path) {
        foreach ($line in Get-Content -LiteralPath $path -Encoding UTF8) {
            $trim = $line.Trim()
            if ($trim.Length -eq 0 -or $trim.StartsWith("#") -or $trim.StartsWith(";")) { continue }
            $eq = $trim.IndexOf("=")
            if ($eq -lt 1) { continue }
            $key = $trim.Substring(0,$eq).Trim().ToLowerInvariant()
            $value = $trim.Substring($eq+1).Trim()
            if ($p.Contains($key)) { $p[$key] = $value }
        }
    }
    # Metadata always follows the disc probe, settings remain user-editable.
    $p["title"] = $Title.Trim()
    $p["volume"] = $Volume.Trim()
    return $p
}

function Write-Cfg([string]$path, $p) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# DreamcastRecompiled per-game profile")
    $lines.Add("# Generated from disc metadata. Settings are preserved on later runs.")
    $lines.Add("# commercial_boot_stage: auto | plain | scrambled")
    $lines.Add("# renderer: auto | dx11 | software")
    $lines.Add("# max_functions: final analysis upper bound; max_closure_entries: 0=auto")
    $lines.Add("# Update limits through launcher > Perfiles por juego, or edit manually + --recompile-game")
    foreach ($k in $p.Keys) {
        $lines.Add(("{0}={1}" -f $k, [string]$p[$k]))
    }
    [IO.File]::WriteAllLines($path, $lines, [Text.UTF8Encoding]::new($false))
}

function Cmd-Escape([string]$s) {
    if ($null -eq $s) { return "" }
    # Values emitted here are consumed as SET "K=V". Escape cmd expansion tokens.
    return $s.Replace("%","%%").Replace('"','""')
}

$defaults = New-Defaults
$slug = Get-SafeSlug $(if ([string]::IsNullOrWhiteSpace($Title)) { [IO.Path]::GetFileNameWithoutExtension($Disc) } else { $Title })
$gameDir = Join-Path $Root $slug
$generatedDir = Join-Path $gameDir "generated"
$buildDir = Join-Path $gameDir "build"
$logsDir = Join-Path $gameDir "logs"
$capturesDir = Join-Path $gameDir "captures"

New-Item -ItemType Directory -Force -Path $gameDir,$generatedDir,$buildDir,$logsDir,$capturesDir | Out-Null
$cfg = Join-Path $gameDir "game.cfg"
$p = Read-Cfg $cfg $defaults
# Migrate ONLY the historical untouched SGGG default. User-tuned limits
# and all other games retain their previous numbers.
$upgradedSggg = $false
if ($defaults.mode -eq "SGGG" -and $p.profile_version -eq "1" -and
    $p.max_functions -eq "8192" -and $p.max_closure_entries -eq "6144") {
    $p.max_functions = "10240"
    $p.max_closure_entries = "7680"
    $upgradedSggg = $true
}
$p.profile_version = "2"
# Reject invalid per-game numeric values before they reach the BAT command line.
foreach ($limit in @("max_functions", "max_closure_entries", "closure_passes")) {
    if ([string]$p[$limit] -cnotmatch '^[0-9]{1,6}$') {
        throw "Valor numerico invalido en $cfg : $limit=$($p[$limit])"
    }
}
$maxFunctions = [int]$p.max_functions
$maxClosure = [int]$p.max_closure_entries
$closurePasses = [int]$p.closure_passes
if ($maxFunctions -lt 64 -or $maxFunctions -gt 100000) {
    throw "max_functions debe estar entre 64 y 100000 en $cfg"
}
if ($maxClosure -lt 0 -or $maxClosure -ge $maxFunctions) {
    throw "max_closure_entries debe ser 0 (auto) o menor que max_functions en $cfg"
}
if ($closurePasses -lt 1 -or $closurePasses -gt 256) {
    throw "closure_passes debe estar entre 1 y 256 en $cfg"
}
Write-Cfg $cfg $p
if ($upgradedSggg) {
    # The next normal launch must rebuild SGGG rather than reuse the old closure.
    [IO.File]::WriteAllText((Join-Path $gameDir ".dcr_limits_recompile_required"), "SGGG default closure migration`n")
}

$vars = [ordered]@{
    DCR_GAME_SLUG            = $slug
    DCR_GAME_DIR             = $gameDir
    DCR_PROFILE_CFG          = $cfg
    DCR_MODE                 = $p.mode
    DCR_PROFILE_RENDERER     = $p.renderer
    DCR_COMMERCIAL_BOOT      = $p.commercial_boot
    DCR_BOOT_STAGE           = $p.commercial_boot_stage
    DCR_DIRECT_GAME_ENTRY    = $p.direct_game_entry
    DCR_CT2_COMPAT           = $p.ct2_compat
    DCR_SEED_FILE            = $p.seed_file
    DCR_ENTRY_SEED           = $p.entry_seed
    DCR_RAW_BASE             = $p.raw_base
    DCR_RAW_ENTRY            = $p.raw_entry
    DCR_MAX_FUNCTIONS        = $p.max_functions
    DCR_MAX_CLOSURE_ENTRIES  = $p.max_closure_entries
    DCR_CLOSURE_PASSES       = $p.closure_passes
    DCR_AUDIO_PROFILE        = $p.audio_profile
    DCR_DEVICE_CLOCK         = $p.device_clock
    DCR_SPECIAL_BOOT_FILE    = $p.special_boot_file
    DCR_CONTROLLER_PROFILE   = $p.controller_profile
    DCR_PROFILE_EXTRA_RUN    = $p.extra_run_args
}

$cmdLines = New-Object System.Collections.Generic.List[string]
foreach ($kv in $vars.GetEnumerator()) {
    $cmdLines.Add(('set "{0}={1}"' -f $kv.Key, (Cmd-Escape ([string]$kv.Value))))
}
[IO.File]::WriteAllLines($EmitCmd, $cmdLines, [Text.ASCIIEncoding]::new())

Write-Host ("[PROFILE] title={0}" -f $Title)
Write-Host ("[PROFILE] mode={0} slug={1}" -f $p.mode, $slug)
Write-Host ("[PROFILE] cfg={0}" -f $cfg)
Write-Host ("[PROFILE] renderer={0} boot-stage={1}" -f $p.renderer, $p.commercial_boot_stage)
