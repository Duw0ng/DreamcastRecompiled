param(
    [string]$Root = (Get-Location).Path,
    [string]$GameCacheRoot = "$env:LOCALAPPDATA\DreamcastRecompiled\games"
)

$ErrorActionPreference = 'Stop'

function Patch-TextFile {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        Write-Host "[SKIP] No existe: $Path"
        return $false
    }

    $text = [IO.File]::ReadAllText($Path)
    if ($text.Contains('struct CommercialIpBinShadow')) {
        Write-Host "[OK] Ya contiene cpp34d: $Path"
        return $true
    }

    $a1 = 'thread_local SH4Context* g_last_sh4_context = nullptr;'
    $i1 = @'
thread_local SH4Context* g_last_sh4_context = nullptr;

// cpp34d: preserve the immutable Sega IP.BIN image for a later chain-load
// back into the bootstrap. Retail titles can reuse 0x8C008000-0x8C00FFFF after
// initial boot, but Homepage/browser paths may later jump back into IP.BIN.
// The AOT instructions remain the original bootstrap, so its data/table bytes
// must be restored as a unit before re-entry.
struct CommercialIpBinShadow {
    DCRuntime* owner{};
    std::array<std::uint8_t, 0x8000u> bytes{};
    bool valid{};
    bool seen_game{};
    bool reentry_latched{};
    std::uint64_t restore_count{};
};
CommercialIpBinShadow g_ipbin_shadow{};
'@

    $a2 = @'
    std::fill_n(runtime.main_ram.begin(), 0x8000u, std::uint8_t{0xFFu});
    dc_zero_bytes(runtime, 0x8C000068u, 24u);
'@
    $i2 = @'
    std::fill_n(runtime.main_ram.begin(), 0x8000u, std::uint8_t{0xFFu});

    // Capture the original 32 KiB IP.BIN image after the generated disc image
    // has populated main RAM. Keep this runtime-private so Guest ABI v1 stays
    // unchanged.
    g_ipbin_shadow = {};
    g_ipbin_shadow.owner = &runtime;
    std::memcpy(g_ipbin_shadow.bytes.data(),
                runtime.main_ram.data() + 0x8000u,
                g_ipbin_shadow.bytes.size());
    g_ipbin_shadow.valid = true;
    std::cout << "[DCR IPBIN] captured 32 KiB bootstrap shadow for chain-load reentry\n";

    dc_zero_bytes(runtime, 0x8C000068u, 24u);
'@

    $a3 = @'
bool dc_runtime_tick_full(SH4Context& ctx, DCRuntime& runtime, std::uint64_t sh4_cycles) {
    g_last_sh4_context = &ctx;
    runtime.current_pc = ctx.pc;
'@
    $i3 = @'
bool dc_runtime_tick_full(SH4Context& ctx, DCRuntime& runtime, std::uint64_t sh4_cycles) {
    g_last_sh4_context = &ctx;
    runtime.current_pc = ctx.pc;

    // cpp34d: restore the original bootstrap when the title chain-loads back
    // into IP.BIN. This mirrors the real console path where IP.BIN is reloaded
    // instead of executing AOT instructions against game-overwritten tables.
    if (g_ipbin_shadow.owner == &runtime && g_ipbin_shadow.valid) {
        const std::uint32_t physical_pc = physical29(ctx.pc);
        const bool in_ipbin =
            physical_pc >= 0x0C008000u && physical_pc < 0x0C010000u;
        const bool in_game_main_ram =
            physical_pc >= 0x0C010000u && physical_pc < 0x0D000000u;

        if (in_game_main_ram) {
            g_ipbin_shadow.seen_game = true;
            g_ipbin_shadow.reentry_latched = false;
        } else if (g_ipbin_shadow.seen_game && in_ipbin &&
                   !g_ipbin_shadow.reentry_latched) {
            std::memcpy(runtime.main_ram.data() + 0x8000u,
                        g_ipbin_shadow.bytes.data(),
                        g_ipbin_shadow.bytes.size());

            constexpr std::size_t first_page =
                0x8000u >> DCRuntime::kMainRamLiteralDirtyPageShift;
            constexpr std::size_t last_page =
                0xFFFFu >> DCRuntime::kMainRamLiteralDirtyPageShift;
            for (std::size_t page = first_page; page <= last_page; ++page)
                runtime.main_ram_literal_dirty[page] = 0u;

            g_ipbin_shadow.reentry_latched = true;
            ++g_ipbin_shadow.restore_count;
            std::cout << "[DCR IPBIN] chain-load reentry | pc=0x"
                      << std::hex << std::uppercase << ctx.pc << std::dec
                      << " | restored=32768 | count="
                      << g_ipbin_shadow.restore_count << "\n";
        }
    }
'@

    foreach ($pair in @(@($a1,$i1), @($a2,$i2), @($a3,$i3))) {
        if (-not $text.Contains($pair[0])) {
            throw "No se encontro un anchor cpp34c esperado en: $Path"
        }
        $text = $text.Replace($pair[0], $pair[1])
    }

    Copy-Item -LiteralPath $Path -Destination ($Path + '.cpp34d.bak') -Force
    [IO.File]::WriteAllText($Path, $text, [Text.UTF8Encoding]::new($false))
    Write-Host "[PATCHED] $Path"
    return $true
}

$patched = 0

$emitter = Join-Path $Root 'src\codegen\cpp_emitter.cpp'
if (Test-Path -LiteralPath $emitter) {
    if (Patch-TextFile $emitter) { $patched++ }
}

if (Test-Path -LiteralPath $GameCacheRoot) {
    $runtimeFiles = Get-ChildItem -LiteralPath $GameCacheRoot -Recurse -Filter 'dc_runtime.cpp' -File -ErrorAction SilentlyContinue
    foreach ($file in $runtimeFiles) {
        $body = [IO.File]::ReadAllText($file.FullName)
        if ($body.Contains('v1.2-cpp34c-flash-base0') -or
            ($body.Contains('dc_setup_commercial_boot') -and $body.Contains('g_last_sh4_context'))) {
            if (Patch-TextFile $file.FullName) { $patched++ }
        }
    }
}

if ($patched -eq 0) {
    throw 'No se encontro cpp_emitter.cpp ni un dc_runtime.cpp compatible para parchear.'
}

Write-Host ''
Write-Host '[OK] cpp34d IPBIN_REENTRY_SHADOW aplicado.'
Write-Host 'Ahora ejecuta:'
Write-Host '  run_game.bat "C:\ruta\ccr.cdi" --rebuild-runtime'
Write-Host ''
Write-Host 'NO uses --clean ni --recompile-game.'
