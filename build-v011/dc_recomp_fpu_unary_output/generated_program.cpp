#include "generated_program.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace dcrecomp_generated {

#if defined(_MSC_VER)
#define DCR_GUEST_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define DCR_GUEST_FORCEINLINE inline __attribute__((always_inline))
#else
#define DCR_GUEST_FORCEINLINE inline
#endif

// 0.0.147: exact hot leaves live in the guest TU so retail SQ/FMOV traffic
// does not cross dc_runtime.cpp for the overwhelmingly common mappings.
// 0.0.201: CT2 validates MMU-off throughout gameplay.  Inline the overwhelmingly
// common SDRAM word path while retaining exact fallback semantics elsewhere.
DCR_GUEST_FORCEINLINE std::uint32_t dc_guest_read32_hot(DCRuntime& runtime, std::uint32_t address) {
#if defined(DCR_ASSUME_MMU_OFF)
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    if (physical >= DCRuntime::kMainRamPhysicalBase && physical < 0x10000000u) {
        const std::uint32_t off = (physical - DCRuntime::kMainRamPhysicalBase) & static_cast<std::uint32_t>(DCRuntime::kMainRamSize - 1u);
        if (off <= DCRuntime::kMainRamSize - 4u) { std::uint32_t value = 0u; std::memcpy(&value, runtime.main_ram.data() + off, sizeof(value)); return value; }
    }
#endif
    return dc_read32_hot(runtime, address);
}

DCR_GUEST_FORCEINLINE void dc_guest_write32_hot(DCRuntime& runtime, std::uint32_t address, std::uint32_t value) {
    if ((address & 0xFC000000u) == 0xE0000000u) {
        const std::size_t base = static_cast<std::size_t>(address & 63u);
        if (base + 4u <= runtime.store_queues.size()) {
            auto* q = runtime.store_queues.data() + base;
            q[0] = static_cast<std::uint8_t>(value); q[1] = static_cast<std::uint8_t>(value >> 8u);
            q[2] = static_cast<std::uint8_t>(value >> 16u); q[3] = static_cast<std::uint8_t>(value >> 24u);
        }
        return;
    }
#if defined(DCR_ASSUME_MMU_OFF)
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    if (physical >= DCRuntime::kMainRamPhysicalBase && physical < 0x10000000u) {
        const std::uint32_t off = (physical - DCRuntime::kMainRamPhysicalBase) & static_cast<std::uint32_t>(DCRuntime::kMainRamSize - 1u);
        if (off <= DCRuntime::kMainRamSize - 4u) { std::memcpy(runtime.main_ram.data() + off, &value, sizeof(value)); runtime.main_ram_literal_dirty[off >> DCRuntime::kMainRamLiteralDirtyPageShift] = 1u; runtime.main_ram_literal_dirty[(off + 3u) >> DCRuntime::kMainRamLiteralDirtyPageShift] = 1u; return; }
    }
#endif
    dc_write32_hot(runtime, address, value);
}

DCR_GUEST_FORCEINLINE std::uint64_t dc_guest_read64_fmov_hot(DCRuntime& runtime, std::uint32_t address) {
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    if (physical >= DCRuntime::kMainRamPhysicalBase && physical < 0x10000000u) {
        const std::uint32_t off = (physical - DCRuntime::kMainRamPhysicalBase) & static_cast<std::uint32_t>(DCRuntime::kMainRamSize - 1u);
        if (off <= DCRuntime::kMainRamSize - 8u) { std::uint64_t bits = 0u; std::memcpy(&bits, runtime.main_ram.data() + off, sizeof(bits)); return bits; }
    }
    return dc_read64_fmov_hot(runtime, address);
}

DCR_GUEST_FORCEINLINE void dc_guest_write64_fmov_hot(DCRuntime& runtime, std::uint32_t address, std::uint64_t bits) {
    if ((address & 0xFC000000u) == 0xE0000000u && (address & 31u) <= 24u) {
        const std::size_t base = static_cast<std::size_t>(address & 63u);
        std::memcpy(runtime.store_queues.data() + base, &bits, sizeof(bits));
        return;
    }
    dc_write64_fmov_hot(runtime, address, bits);
}

DCR_GUEST_FORCEINLINE void dc_guest_pref(DCRuntime& runtime, std::uint32_t address, std::uint32_t source_address) {
    if ((address & 0xFC000000u) != 0xE0000000u) {
#if !defined(DCR_LIGHTWEIGHT_HOT_METRICS)
        ++runtime.pref_calls;
#endif
        return;
    }
    dc_pref(runtime, address, source_address);
}

// Generated from _main @ 0x8C010000u
void recomp_8C010000(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    // 0.0.155: static function-superblock FR/XF cache. Register
    // membership and writeback masks are compile-time constants: no
    // per-lane valid/dirty branches exist in the generated hot path.
    bool sfc_active = false;
    bool sfc_fr_swapped = false;
    std::uint64_t sfc_stat_regions=0, sfc_stat_ops=0, sfc_stat_reuses=0, sfc_stat_planned_wb=0;
    std::uint64_t sfc_stat_ftrv=0, sfc_stat_fipr=0, sfc_stat_fmac=0, sfc_stat_fsca=0, sfc_stat_fsrra=0, sfc_stat_cross=0, sfc_stat_super_reuses=0, sfc_stat_entries=0, sfc_stat_static_loads=0;
    std::uint32_t sfc_fr0 = 0u;
    std::uint32_t sfc_fr1 = 0u;
    std::uint32_t sfc_fr2 = 0u;
    std::uint32_t sfc_fr3 = 0u;
    std::uint32_t sfc_fr4 = 0u;
    std::uint32_t sfc_fr5 = 0u;
    std::uint32_t sfc_fr6 = 0u;
    std::uint32_t sfc_fr7 = 0u;
    std::uint32_t sfc_xf0 = 0u;
    std::uint32_t sfc_xf1 = 0u;
    std::uint32_t sfc_xf2 = 0u;
    std::uint32_t sfc_xf3 = 0u;
    std::uint32_t sfc_xf4 = 0u;
    std::uint32_t sfc_xf5 = 0u;
    std::uint32_t sfc_xf6 = 0u;
    std::uint32_t sfc_xf7 = 0u;
    std::uint32_t sfc_xf8 = 0u;
    std::uint32_t sfc_xf9 = 0u;
    std::uint32_t sfc_xf10 = 0u;
    std::uint32_t sfc_xf11 = 0u;
    std::uint32_t sfc_xf12 = 0u;
    std::uint32_t sfc_xf13 = 0u;
    std::uint32_t sfc_xf14 = 0u;
    std::uint32_t sfc_xf15 = 0u;
    auto sfc_flush = [&](unsigned sfc_reason) {
        if (!sfc_active) return;
        auto& sfc_fr_bank = sfc_fr_swapped ? ctx.xf_bits : ctx.fr_bits;
        auto& sfc_xf_bank = sfc_fr_swapped ? ctx.fr_bits : ctx.xf_bits;
        sfc_fr_bank[0u] = sfc_fr0;
        sfc_fr_bank[1u] = sfc_fr1;
        sfc_fr_bank[2u] = sfc_fr2;
        sfc_fr_bank[3u] = sfc_fr3;
        sfc_fr_bank[4u] = sfc_fr4;
        sfc_fr_bank[5u] = sfc_fr5;
        sfc_fr_bank[6u] = sfc_fr6;
        sfc_fr_bank[7u] = sfc_fr7;
        runtime.fpu_super_actual_writebacks += 8u;
        runtime.fpu_block_cache_hits += sfc_stat_regions; runtime.fpu_block_cache_ops += sfc_stat_ops; runtime.fpu_block_cache_reuses += sfc_stat_reuses; runtime.fpu_block_cache_writebacks += sfc_stat_planned_wb;
        runtime.fpu_ftrv_ops += sfc_stat_ftrv; if (sfc_fr_swapped) runtime.fpu_ftrv_fr1_ops += sfc_stat_ftrv; runtime.fpu_fipr_ops += sfc_stat_fipr; runtime.fpu_fmac_ops += sfc_stat_fmac; runtime.fpu_fsca_ops += sfc_stat_fsca; runtime.fpu_fsrra_ops += sfc_stat_fsrra;
        runtime.fpu_super_cross_block_keeps += sfc_stat_cross; runtime.fpu_super_reuses += sfc_stat_super_reuses; runtime.fpu_super_entries += sfc_stat_entries; runtime.fpu_super_static_loads += sfc_stat_static_loads;
        sfc_stat_regions=sfc_stat_ops=sfc_stat_reuses=sfc_stat_planned_wb=0; sfc_stat_ftrv=sfc_stat_fipr=sfc_stat_fmac=sfc_stat_fsca=sfc_stat_fsrra=sfc_stat_cross=sfc_stat_super_reuses=sfc_stat_entries=sfc_stat_static_loads=0;
        if (sfc_reason == 1u) ++runtime.fpu_super_tick_flushes;
        else if (sfc_reason == 2u) ++runtime.fpu_super_barrier_flushes;
        else if (sfc_reason == 3u) ++runtime.fpu_super_exit_flushes;
        sfc_active = false;
    };
    auto sfc_activate = [&]() {
        sfc_active = true;
        sfc_fr_swapped = (ctx.fpscr & (1u << 21)) != 0u;
        auto& sfc_fr_bank = sfc_fr_swapped ? ctx.xf_bits : ctx.fr_bits;
        auto& sfc_xf_bank = sfc_fr_swapped ? ctx.fr_bits : ctx.xf_bits;
        sfc_fr0 = sfc_fr_bank[0u];
        sfc_fr1 = sfc_fr_bank[1u];
        sfc_fr2 = sfc_fr_bank[2u];
        sfc_fr3 = sfc_fr_bank[3u];
        sfc_fr4 = sfc_fr_bank[4u];
        sfc_fr5 = sfc_fr_bank[5u];
        sfc_fr6 = sfc_fr_bank[6u];
        sfc_fr7 = sfc_fr_bank[7u];
        sfc_xf0 = sfc_xf_bank[0u];
        sfc_xf1 = sfc_xf_bank[1u];
        sfc_xf2 = sfc_xf_bank[2u];
        sfc_xf3 = sfc_xf_bank[3u];
        sfc_xf4 = sfc_xf_bank[4u];
        sfc_xf5 = sfc_xf_bank[5u];
        sfc_xf6 = sfc_xf_bank[6u];
        sfc_xf7 = sfc_xf_bank[7u];
        sfc_xf8 = sfc_xf_bank[8u];
        sfc_xf9 = sfc_xf_bank[9u];
        sfc_xf10 = sfc_xf_bank[10u];
        sfc_xf11 = sfc_xf_bank[11u];
        sfc_xf12 = sfc_xf_bank[12u];
        sfc_xf13 = sfc_xf_bank[13u];
        sfc_xf14 = sfc_xf_bank[14u];
        sfc_xf15 = sfc_xf_bank[15u];
        ++sfc_stat_entries; sfc_stat_static_loads += 24u;
    };
    auto sfc_tick = [&](std::uint64_t sfc_cycles) -> bool {
#if !defined(DCR_DISABLE_HOT_TICK_CALL_COUNTER)
        ++runtime.sh4_tick_calls;
#endif
        runtime.sh4_tick_pending_cycles += sfc_cycles;
        const std::uint64_t sfc_batch = runtime.sh4_tick_batch_cycles != 0u ? runtime.sh4_tick_batch_cycles : 1u;
        if (runtime.sh4_tick_pending_cycles < sfc_batch) { if (sfc_active) ++sfc_stat_cross; return false; }
        sfc_flush(1u);
        const std::uint64_t sfc_accumulated = runtime.sh4_tick_pending_cycles;
        runtime.sh4_tick_pending_cycles = 0u;
        ++runtime.sh4_tick_full_calls;
        return dc_runtime_tick_full(ctx, runtime, sfc_accumulated);
    };
    switch (ctx.pc) {
        case 0x8C010000u: goto BB_8C010000;
        case 0x8C010012u: goto BB_8C010012;
        case 0x8C010024u: goto BB_8C010024;
        case 0x8C01003Cu: goto BB_8C01003C;
        case 0x8C010056u: goto BB_8C010056;
        case 0x8C010094u: goto BB_8C010094;
        case 0x8C0100A2u: goto BB_8C0100A2;
        case 0x8C0100ACu: goto BB_8C0100AC;
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(9u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 9u)) return;
    // SH-4 @ 0x8C010000u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    // SH-4 @ 0x8C010002u: LDS_FPUL
    ctx.fpul = ctx.r[1];
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010004u: FLOAT
    dc_require_fpu_round_nearest(ctx, 0x8C010004u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 0u, static_cast<double>(static_cast<std::int32_t>(ctx.fpul)));
    else dc_set_fr_float(ctx, 0u, static_cast<float>(static_cast<std::int32_t>(ctx.fpul)));
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010006u: FNEG
    if (dc_fpu_double_precision(ctx)) dc_set_dr_bits(ctx, 0u, dc_get_dr_bits(ctx, 0u) ^ 0x8000000000000000ull);
    else dc_set_fr_bits(ctx, 0u, dc_get_fr_bits(ctx, 0u) ^ 0x80000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010008u: FABS
    if (dc_fpu_double_precision(ctx)) dc_set_dr_bits(ctx, 0u, dc_get_dr_bits(ctx, 0u) & 0x7FFFFFFFFFFFFFFFull);
    else dc_set_fr_bits(ctx, 0u, dc_get_fr_bits(ctx, 0u) & 0x7FFFFFFFu);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01000Au: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01000Au);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 0u) : static_cast<double>(dc_get_fr_float(ctx, 0u)));
    // SH-4 @ 0x8C01000Cu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C01000Eu: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) ? 1u : 0u);
    // SH-4 @ 0x8C010010u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;

BB_8C010012:
    ctx.pc = 0x8C010012u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(9u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 9u)) return;
    if (runtime.fpu_aot_mode == 2u) {
        if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        ++sfc_stat_regions; sfc_stat_ops += 6u; sfc_stat_reuses += 4u; sfc_stat_planned_wb += 2u;
        sfc_stat_ftrv += 0u; sfc_stat_fipr += 0u; sfc_stat_fmac += 0u; sfc_stat_fsca += 0u; sfc_stat_fsrra += 1u;
        if (!sfc_active) sfc_activate(); else ++sfc_stat_super_reuses;
        // SH-4 @ 0x8C010012u: FLDI1 [FPU sfc_]
        sfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010014u: FSQRT [FPU sfc_]
        sfc_fr1 = std::bit_cast<std::uint32_t>(std::sqrt(std::bit_cast<float>(sfc_fr1)));
        // SH-4 @ 0x8C010016u: FSRRA [FPU sfc_]
        sfc_fr1 = std::bit_cast<std::uint32_t>(1.0f / std::sqrt(std::bit_cast<float>(sfc_fr1)));
        // SH-4 @ 0x8C010018u: FLDS [FPU sfc_]
        ctx.fpul = sfc_fr1;
        // SH-4 @ 0x8C01001Au: FSTS [FPU sfc_]
        sfc_fr2 = ctx.fpul;
        // SH-4 @ 0x8C01001Cu: FTRC [FPU sfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(sfc_fr2)));
    // SH-4 @ 0x8C01001Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010020u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010022u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        } else {
            sfc_flush(2u); ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C010012u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010012u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010014u: FSQRT
    dc_require_fpu_round_nearest(ctx, 0x8C010014u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 1u, std::sqrt(dc_get_dr_double(ctx, 1u)));
    else dc_set_fr_float(ctx, 1u, std::sqrt(dc_get_fr_float(ctx, 1u)));
    // SH-4 @ 0x8C010016u: FSRRA
    ++runtime.fpu_fsrra_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C010016u);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C010016u, "FSRRA requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const float x = std::bit_cast<float>(frb[1u]); frb[1u] = std::bit_cast<std::uint32_t>(1.0f / std::sqrt(x)); }
    // SH-4 @ 0x8C010018u: FLDS
    ctx.fpul = dc_get_fr_bits(ctx, 1u);
    // SH-4 @ 0x8C01001Au: FSTS
    dc_set_fr_bits(ctx, 2u, ctx.fpul);
    // SH-4 @ 0x8C01001Cu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01001Cu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 2u) : static_cast<double>(dc_get_fr_float(ctx, 2u)));
    // SH-4 @ 0x8C01001Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010020u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010022u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        }
    } else {
    if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        if (runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics) {
            ++runtime.fpu_block_cache_hits; runtime.fpu_block_cache_ops += 6u; runtime.fpu_block_cache_reuses += 4u; runtime.fpu_block_cache_writebacks += 2u;
            runtime.fpu_ftrv_ops += 0u; if ((ctx.fpscr & (1u << 21)) != 0u) runtime.fpu_ftrv_fr1_ops += 0u;
            runtime.fpu_fipr_ops += 0u; runtime.fpu_fmac_ops += 0u; runtime.fpu_fsca_ops += 0u; runtime.fpu_fsrra_ops += 1u;
        }
        auto& rfc_fr_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits;
        auto& rfc_xf_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits;
        std::uint32_t rfc_fr1 = 0u;
        std::uint32_t rfc_fr2 = 0u;
        // SH-4 @ 0x8C010012u: FLDI1 [FPU rfc_]
        rfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010014u: FSQRT [FPU rfc_]
        rfc_fr1 = std::bit_cast<std::uint32_t>(std::sqrt(std::bit_cast<float>(rfc_fr1)));
        // SH-4 @ 0x8C010016u: FSRRA [FPU rfc_]
        rfc_fr1 = std::bit_cast<std::uint32_t>(1.0f / std::sqrt(std::bit_cast<float>(rfc_fr1)));
        // SH-4 @ 0x8C010018u: FLDS [FPU rfc_]
        ctx.fpul = rfc_fr1;
        // SH-4 @ 0x8C01001Au: FSTS [FPU rfc_]
        rfc_fr2 = ctx.fpul;
        // SH-4 @ 0x8C01001Cu: FTRC [FPU rfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(rfc_fr2)));
    // SH-4 @ 0x8C01001Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010020u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
        rfc_fr_bank[1u] = rfc_fr1;
        rfc_fr_bank[2u] = rfc_fr2;
    // SH-4 @ 0x8C010022u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    } else {
        ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C010012u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010012u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010014u: FSQRT
    dc_require_fpu_round_nearest(ctx, 0x8C010014u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 1u, std::sqrt(dc_get_dr_double(ctx, 1u)));
    else dc_set_fr_float(ctx, 1u, std::sqrt(dc_get_fr_float(ctx, 1u)));
    // SH-4 @ 0x8C010016u: FSRRA
    ++runtime.fpu_fsrra_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C010016u);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C010016u, "FSRRA requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const float x = std::bit_cast<float>(frb[1u]); frb[1u] = std::bit_cast<std::uint32_t>(1.0f / std::sqrt(x)); }
    // SH-4 @ 0x8C010018u: FLDS
    ctx.fpul = dc_get_fr_bits(ctx, 1u);
    // SH-4 @ 0x8C01001Au: FSTS
    dc_set_fr_bits(ctx, 2u, ctx.fpul);
    // SH-4 @ 0x8C01001Cu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01001Cu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 2u) : static_cast<double>(dc_get_fr_float(ctx, 2u)));
    // SH-4 @ 0x8C01001Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010020u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010022u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    }
    }

BB_8C010024:
    ctx.pc = 0x8C010024u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(12u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 12u)) return;
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010024u: FLDS
    ctx.fpul = dc_get_fr_bits(ctx, 2u);
    // SH-4 @ 0x8C010026u: LOAD_LITERAL32
    ctx.r[3] = dc_load_pc_literal32(runtime, 0x8C010026u, 0x8C0100B4u, 0x00080000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010028u: LDS_FPSCR
    dc_write_fpscr(ctx, ctx.r[3]);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01002Au: FCNVSD
    dc_require_fpu_round_nearest(ctx, 0x8C01002Au);
    if (!dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01002Au, "FCNVSD requires FPSCR.PR=1");
    dc_set_dr_double(ctx, 4u, static_cast<double>(std::bit_cast<float>(ctx.fpul)));
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01002Cu: FCNVDS
    dc_require_fpu_round_nearest(ctx, 0x8C01002Cu);
    if (!dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01002Cu, "FCNVDS requires FPSCR.PR=1");
    ctx.fpul = std::bit_cast<std::uint32_t>(static_cast<float>(dc_get_dr_double(ctx, 4u)));
    // SH-4 @ 0x8C01002Eu: MOV_IMM
    ctx.r[3] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010030u: LDS_FPSCR
    dc_write_fpscr(ctx, ctx.r[3]);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010032u: FSTS
    dc_set_fr_bits(ctx, 10u, ctx.fpul);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010034u: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C010034u);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 10u) : static_cast<double>(dc_get_fr_float(ctx, 10u)));
    // SH-4 @ 0x8C010036u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010038u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C01003Au: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;

BB_8C01003C:
    ctx.pc = 0x8C01003Cu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(13u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 13u)) return;
    if (runtime.fpu_aot_mode == 2u) {
        if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        ++sfc_stat_regions; sfc_stat_ops += 10u; sfc_stat_reuses += 9u; sfc_stat_planned_wb += 8u;
        sfc_stat_ftrv += 0u; sfc_stat_fipr += 1u; sfc_stat_fmac += 0u; sfc_stat_fsca += 0u; sfc_stat_fsrra += 0u;
        if (!sfc_active) sfc_activate(); else ++sfc_stat_super_reuses;
        // SH-4 @ 0x8C01003Cu: FLDI1 [FPU sfc_]
        sfc_fr0 = 0x3F800000u;
        // SH-4 @ 0x8C01003Eu: FLDI1 [FPU sfc_]
        sfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010040u: FLDI1 [FPU sfc_]
        sfc_fr2 = 0x3F800000u;
        // SH-4 @ 0x8C010042u: FLDI1 [FPU sfc_]
        sfc_fr3 = 0x3F800000u;
        // SH-4 @ 0x8C010044u: FLDI1 [FPU sfc_]
        sfc_fr4 = 0x3F800000u;
        // SH-4 @ 0x8C010046u: FLDI1 [FPU sfc_]
        sfc_fr5 = 0x3F800000u;
        // SH-4 @ 0x8C010048u: FLDI1 [FPU sfc_]
        sfc_fr6 = 0x3F800000u;
        // SH-4 @ 0x8C01004Au: FLDI1 [FPU sfc_]
        sfc_fr7 = 0x3F800000u;
        // SH-4 @ 0x8C01004Cu: FIPR [FPU sfc_]
        { const double fc_sum = static_cast<double>(std::bit_cast<float>(sfc_fr0)) * std::bit_cast<float>(sfc_fr4) + static_cast<double>(std::bit_cast<float>(sfc_fr1)) * std::bit_cast<float>(sfc_fr5) + static_cast<double>(std::bit_cast<float>(sfc_fr2)) * std::bit_cast<float>(sfc_fr6) + static_cast<double>(std::bit_cast<float>(sfc_fr3)) * std::bit_cast<float>(sfc_fr7); sfc_fr7 = std::bit_cast<std::uint32_t>(static_cast<float>(fc_sum)); }
        // SH-4 @ 0x8C01004Eu: FTRC [FPU sfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(sfc_fr7)));
    // SH-4 @ 0x8C010050u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010052u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) ? 1u : 0u);
    // SH-4 @ 0x8C010054u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        } else {
            sfc_flush(2u); ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C01003Cu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01003Cu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x3F800000u);
    // SH-4 @ 0x8C01003Eu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01003Eu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010040u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010040u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 2u, 0x3F800000u);
    // SH-4 @ 0x8C010042u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010042u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 3u, 0x3F800000u);
    // SH-4 @ 0x8C010044u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010044u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 4u, 0x3F800000u);
    // SH-4 @ 0x8C010046u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010046u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 5u, 0x3F800000u);
    // SH-4 @ 0x8C010048u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010048u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 6u, 0x3F800000u);
    // SH-4 @ 0x8C01004Au: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01004Au, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 7u, 0x3F800000u);
    // SH-4 @ 0x8C01004Cu: FIPR
    ++runtime.fpu_fipr_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C01004Cu);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C01004Cu, "FIPR requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const float a0 = std::bit_cast<float>(frb[0u]); const float a1 = std::bit_cast<float>(frb[1u]); const float a2 = std::bit_cast<float>(frb[2u]); const float a3 = std::bit_cast<float>(frb[3u]); const float b0 = std::bit_cast<float>(frb[4u]); const float b1 = std::bit_cast<float>(frb[5u]); const float b2 = std::bit_cast<float>(frb[6u]); const float b3 = std::bit_cast<float>(frb[7u]); const double sum = static_cast<double>(a0) * b0 + static_cast<double>(a1) * b1 + static_cast<double>(a2) * b2 + static_cast<double>(a3) * b3; frb[7u] = std::bit_cast<std::uint32_t>(static_cast<float>(sum)); }
    // SH-4 @ 0x8C01004Eu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01004Eu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 7u) : static_cast<double>(dc_get_fr_float(ctx, 7u)));
    // SH-4 @ 0x8C010050u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010052u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) ? 1u : 0u);
    // SH-4 @ 0x8C010054u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        }
    } else {
    if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        if (runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics) {
            ++runtime.fpu_block_cache_hits; runtime.fpu_block_cache_ops += 10u; runtime.fpu_block_cache_reuses += 9u; runtime.fpu_block_cache_writebacks += 8u;
            runtime.fpu_ftrv_ops += 0u; if ((ctx.fpscr & (1u << 21)) != 0u) runtime.fpu_ftrv_fr1_ops += 0u;
            runtime.fpu_fipr_ops += 1u; runtime.fpu_fmac_ops += 0u; runtime.fpu_fsca_ops += 0u; runtime.fpu_fsrra_ops += 0u;
        }
        auto& rfc_fr_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits;
        auto& rfc_xf_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits;
        std::uint32_t rfc_fr0 = 0u;
        std::uint32_t rfc_fr1 = 0u;
        std::uint32_t rfc_fr2 = 0u;
        std::uint32_t rfc_fr3 = 0u;
        std::uint32_t rfc_fr4 = 0u;
        std::uint32_t rfc_fr5 = 0u;
        std::uint32_t rfc_fr6 = 0u;
        std::uint32_t rfc_fr7 = 0u;
        // SH-4 @ 0x8C01003Cu: FLDI1 [FPU rfc_]
        rfc_fr0 = 0x3F800000u;
        // SH-4 @ 0x8C01003Eu: FLDI1 [FPU rfc_]
        rfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010040u: FLDI1 [FPU rfc_]
        rfc_fr2 = 0x3F800000u;
        // SH-4 @ 0x8C010042u: FLDI1 [FPU rfc_]
        rfc_fr3 = 0x3F800000u;
        // SH-4 @ 0x8C010044u: FLDI1 [FPU rfc_]
        rfc_fr4 = 0x3F800000u;
        // SH-4 @ 0x8C010046u: FLDI1 [FPU rfc_]
        rfc_fr5 = 0x3F800000u;
        // SH-4 @ 0x8C010048u: FLDI1 [FPU rfc_]
        rfc_fr6 = 0x3F800000u;
        // SH-4 @ 0x8C01004Au: FLDI1 [FPU rfc_]
        rfc_fr7 = 0x3F800000u;
        // SH-4 @ 0x8C01004Cu: FIPR [FPU rfc_]
        { const double fc_sum = static_cast<double>(std::bit_cast<float>(rfc_fr0)) * std::bit_cast<float>(rfc_fr4) + static_cast<double>(std::bit_cast<float>(rfc_fr1)) * std::bit_cast<float>(rfc_fr5) + static_cast<double>(std::bit_cast<float>(rfc_fr2)) * std::bit_cast<float>(rfc_fr6) + static_cast<double>(std::bit_cast<float>(rfc_fr3)) * std::bit_cast<float>(rfc_fr7); rfc_fr7 = std::bit_cast<std::uint32_t>(static_cast<float>(fc_sum)); }
        // SH-4 @ 0x8C01004Eu: FTRC [FPU rfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(rfc_fr7)));
    // SH-4 @ 0x8C010050u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010052u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) ? 1u : 0u);
        rfc_fr_bank[0u] = rfc_fr0;
        rfc_fr_bank[1u] = rfc_fr1;
        rfc_fr_bank[2u] = rfc_fr2;
        rfc_fr_bank[3u] = rfc_fr3;
        rfc_fr_bank[4u] = rfc_fr4;
        rfc_fr_bank[5u] = rfc_fr5;
        rfc_fr_bank[6u] = rfc_fr6;
        rfc_fr_bank[7u] = rfc_fr7;
    // SH-4 @ 0x8C010054u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    } else {
        ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C01003Cu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01003Cu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x3F800000u);
    // SH-4 @ 0x8C01003Eu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01003Eu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010040u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010040u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 2u, 0x3F800000u);
    // SH-4 @ 0x8C010042u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010042u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 3u, 0x3F800000u);
    // SH-4 @ 0x8C010044u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010044u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 4u, 0x3F800000u);
    // SH-4 @ 0x8C010046u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010046u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 5u, 0x3F800000u);
    // SH-4 @ 0x8C010048u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010048u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 6u, 0x3F800000u);
    // SH-4 @ 0x8C01004Au: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01004Au, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 7u, 0x3F800000u);
    // SH-4 @ 0x8C01004Cu: FIPR
    ++runtime.fpu_fipr_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C01004Cu);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C01004Cu, "FIPR requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const float a0 = std::bit_cast<float>(frb[0u]); const float a1 = std::bit_cast<float>(frb[1u]); const float a2 = std::bit_cast<float>(frb[2u]); const float a3 = std::bit_cast<float>(frb[3u]); const float b0 = std::bit_cast<float>(frb[4u]); const float b1 = std::bit_cast<float>(frb[5u]); const float b2 = std::bit_cast<float>(frb[6u]); const float b3 = std::bit_cast<float>(frb[7u]); const double sum = static_cast<double>(a0) * b0 + static_cast<double>(a1) * b1 + static_cast<double>(a2) * b2 + static_cast<double>(a3) * b3; frb[7u] = std::bit_cast<std::uint32_t>(static_cast<float>(sum)); }
    // SH-4 @ 0x8C01004Eu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01004Eu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 7u) : static_cast<double>(dc_get_fr_float(ctx, 7u)));
    // SH-4 @ 0x8C010050u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010052u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) ? 1u : 0u);
    // SH-4 @ 0x8C010054u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    }
    }

BB_8C010056:
    ctx.pc = 0x8C010056u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(31u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 31u)) return;
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010056u: FRCHG
    ++runtime.fpu_frchg_ops; ctx.fpscr ^= (1u << 21);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010058u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010058u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01005Au: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01005Au, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01005Cu: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01005Cu, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 2u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01005Eu: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01005Eu, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 3u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010060u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010060u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 4u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010062u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010062u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 5u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010064u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010064u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 6u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010066u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010066u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 7u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010068u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010068u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 8u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01006Au: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01006Au, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 9u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01006Cu: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01006Cu, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 10u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01006Eu: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01006Eu, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 11u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010070u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010070u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 12u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010072u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010072u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 13u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010074u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010074u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 14u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010076u: FLDI0
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010076u, "FLDI0 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 15u, 0x00000000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010078u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010078u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x3F800000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01007Au: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01007Au, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 5u, 0x3F800000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01007Cu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01007Cu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 10u, 0x3F800000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01007Eu: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C01007Eu, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 15u, 0x3F800000u);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010080u: FRCHG
    ++runtime.fpu_frchg_ops; ctx.fpscr ^= (1u << 21);
    if (runtime.fpu_aot_mode == 2u) {
        if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        ++sfc_stat_regions; sfc_stat_ops += 6u; sfc_stat_reuses += 5u; sfc_stat_planned_wb += 4u;
        sfc_stat_ftrv += 1u; sfc_stat_fipr += 0u; sfc_stat_fmac += 0u; sfc_stat_fsca += 0u; sfc_stat_fsrra += 0u;
        if (!sfc_active) sfc_activate(); else ++sfc_stat_super_reuses;
        // SH-4 @ 0x8C010082u: FLDI1 [FPU sfc_]
        sfc_fr0 = 0x3F800000u;
        // SH-4 @ 0x8C010084u: FLDI1 [FPU sfc_]
        sfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010086u: FLDI1 [FPU sfc_]
        sfc_fr2 = 0x3F800000u;
        // SH-4 @ 0x8C010088u: FLDI1 [FPU sfc_]
        sfc_fr3 = 0x3F800000u;
        // SH-4 @ 0x8C01008Au: FTRV [FPU sfc_]
        { const float fc_v0=std::bit_cast<float>(sfc_fr0),fc_v1=std::bit_cast<float>(sfc_fr1),fc_v2=std::bit_cast<float>(sfc_fr2),fc_v3=std::bit_cast<float>(sfc_fr3); const double fc_o0=static_cast<double>(std::bit_cast<float>(sfc_xf0))*fc_v0+static_cast<double>(std::bit_cast<float>(sfc_xf4))*fc_v1+static_cast<double>(std::bit_cast<float>(sfc_xf8))*fc_v2+static_cast<double>(std::bit_cast<float>(sfc_xf12))*fc_v3; const double fc_o1=static_cast<double>(std::bit_cast<float>(sfc_xf1))*fc_v0+static_cast<double>(std::bit_cast<float>(sfc_xf5))*fc_v1+static_cast<double>(std::bit_cast<float>(sfc_xf9))*fc_v2+static_cast<double>(std::bit_cast<float>(sfc_xf13))*fc_v3; const double fc_o2=static_cast<double>(std::bit_cast<float>(sfc_xf2))*fc_v0+static_cast<double>(std::bit_cast<float>(sfc_xf6))*fc_v1+static_cast<double>(std::bit_cast<float>(sfc_xf10))*fc_v2+static_cast<double>(std::bit_cast<float>(sfc_xf14))*fc_v3; const double fc_o3=static_cast<double>(std::bit_cast<float>(sfc_xf3))*fc_v0+static_cast<double>(std::bit_cast<float>(sfc_xf7))*fc_v1+static_cast<double>(std::bit_cast<float>(sfc_xf11))*fc_v2+static_cast<double>(std::bit_cast<float>(sfc_xf15))*fc_v3; sfc_fr0=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o0)); sfc_fr1=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o1)); sfc_fr2=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o2)); sfc_fr3=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o3)); }
        // SH-4 @ 0x8C01008Cu: FTRC [FPU sfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(sfc_fr3)));
    // SH-4 @ 0x8C01008Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010090u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010092u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        } else {
            sfc_flush(2u); ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C010082u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010082u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x3F800000u);
    // SH-4 @ 0x8C010084u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010084u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010086u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010086u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 2u, 0x3F800000u);
    // SH-4 @ 0x8C010088u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010088u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 3u, 0x3F800000u);
    // SH-4 @ 0x8C01008Au: FTRV
    ++runtime.fpu_ftrv_ops; if ((ctx.fpscr & (1u << 21)) != 0u) ++runtime.fpu_ftrv_fr1_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C01008Au);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C01008Au, "FTRV requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const auto& xfb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits; const float v0 = std::bit_cast<float>(frb[0u]); const float v1 = std::bit_cast<float>(frb[1u]); const float v2 = std::bit_cast<float>(frb[2u]); const float v3 = std::bit_cast<float>(frb[3u]); const double o0 = static_cast<double>(std::bit_cast<float>(xfb[0u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[4u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[8u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[12u])) * v3; const double o1 = static_cast<double>(std::bit_cast<float>(xfb[1u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[5u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[9u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[13u])) * v3; const double o2 = static_cast<double>(std::bit_cast<float>(xfb[2u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[6u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[10u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[14u])) * v3; const double o3 = static_cast<double>(std::bit_cast<float>(xfb[3u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[7u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[11u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[15u])) * v3; frb[0u] = std::bit_cast<std::uint32_t>(static_cast<float>(o0)); frb[1u] = std::bit_cast<std::uint32_t>(static_cast<float>(o1)); frb[2u] = std::bit_cast<std::uint32_t>(static_cast<float>(o2)); frb[3u] = std::bit_cast<std::uint32_t>(static_cast<float>(o3)); }
    // SH-4 @ 0x8C01008Cu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01008Cu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 3u) : static_cast<double>(dc_get_fr_float(ctx, 3u)));
    // SH-4 @ 0x8C01008Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010090u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010092u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
        }
    } else {
    if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        if (runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics) {
            ++runtime.fpu_block_cache_hits; runtime.fpu_block_cache_ops += 6u; runtime.fpu_block_cache_reuses += 5u; runtime.fpu_block_cache_writebacks += 4u;
            runtime.fpu_ftrv_ops += 1u; if ((ctx.fpscr & (1u << 21)) != 0u) runtime.fpu_ftrv_fr1_ops += 1u;
            runtime.fpu_fipr_ops += 0u; runtime.fpu_fmac_ops += 0u; runtime.fpu_fsca_ops += 0u; runtime.fpu_fsrra_ops += 0u;
        }
        auto& rfc_fr_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits;
        auto& rfc_xf_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits;
        std::uint32_t rfc_fr0 = 0u;
        std::uint32_t rfc_fr1 = 0u;
        std::uint32_t rfc_fr2 = 0u;
        std::uint32_t rfc_fr3 = 0u;
        std::uint32_t rfc_xf0 = rfc_xf_bank[0u];
        std::uint32_t rfc_xf1 = rfc_xf_bank[1u];
        std::uint32_t rfc_xf2 = rfc_xf_bank[2u];
        std::uint32_t rfc_xf3 = rfc_xf_bank[3u];
        std::uint32_t rfc_xf4 = rfc_xf_bank[4u];
        std::uint32_t rfc_xf5 = rfc_xf_bank[5u];
        std::uint32_t rfc_xf6 = rfc_xf_bank[6u];
        std::uint32_t rfc_xf7 = rfc_xf_bank[7u];
        std::uint32_t rfc_xf8 = rfc_xf_bank[8u];
        std::uint32_t rfc_xf9 = rfc_xf_bank[9u];
        std::uint32_t rfc_xf10 = rfc_xf_bank[10u];
        std::uint32_t rfc_xf11 = rfc_xf_bank[11u];
        std::uint32_t rfc_xf12 = rfc_xf_bank[12u];
        std::uint32_t rfc_xf13 = rfc_xf_bank[13u];
        std::uint32_t rfc_xf14 = rfc_xf_bank[14u];
        std::uint32_t rfc_xf15 = rfc_xf_bank[15u];
        // SH-4 @ 0x8C010082u: FLDI1 [FPU rfc_]
        rfc_fr0 = 0x3F800000u;
        // SH-4 @ 0x8C010084u: FLDI1 [FPU rfc_]
        rfc_fr1 = 0x3F800000u;
        // SH-4 @ 0x8C010086u: FLDI1 [FPU rfc_]
        rfc_fr2 = 0x3F800000u;
        // SH-4 @ 0x8C010088u: FLDI1 [FPU rfc_]
        rfc_fr3 = 0x3F800000u;
        // SH-4 @ 0x8C01008Au: FTRV [FPU rfc_]
        { const float fc_v0=std::bit_cast<float>(rfc_fr0),fc_v1=std::bit_cast<float>(rfc_fr1),fc_v2=std::bit_cast<float>(rfc_fr2),fc_v3=std::bit_cast<float>(rfc_fr3); const double fc_o0=static_cast<double>(std::bit_cast<float>(rfc_xf0))*fc_v0+static_cast<double>(std::bit_cast<float>(rfc_xf4))*fc_v1+static_cast<double>(std::bit_cast<float>(rfc_xf8))*fc_v2+static_cast<double>(std::bit_cast<float>(rfc_xf12))*fc_v3; const double fc_o1=static_cast<double>(std::bit_cast<float>(rfc_xf1))*fc_v0+static_cast<double>(std::bit_cast<float>(rfc_xf5))*fc_v1+static_cast<double>(std::bit_cast<float>(rfc_xf9))*fc_v2+static_cast<double>(std::bit_cast<float>(rfc_xf13))*fc_v3; const double fc_o2=static_cast<double>(std::bit_cast<float>(rfc_xf2))*fc_v0+static_cast<double>(std::bit_cast<float>(rfc_xf6))*fc_v1+static_cast<double>(std::bit_cast<float>(rfc_xf10))*fc_v2+static_cast<double>(std::bit_cast<float>(rfc_xf14))*fc_v3; const double fc_o3=static_cast<double>(std::bit_cast<float>(rfc_xf3))*fc_v0+static_cast<double>(std::bit_cast<float>(rfc_xf7))*fc_v1+static_cast<double>(std::bit_cast<float>(rfc_xf11))*fc_v2+static_cast<double>(std::bit_cast<float>(rfc_xf15))*fc_v3; rfc_fr0=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o0)); rfc_fr1=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o1)); rfc_fr2=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o2)); rfc_fr3=std::bit_cast<std::uint32_t>(static_cast<float>(fc_o3)); }
        // SH-4 @ 0x8C01008Cu: FTRC [FPU rfc_]
        ctx.fpul = dc_ftrc_to_u32(static_cast<double>(std::bit_cast<float>(rfc_fr3)));
    // SH-4 @ 0x8C01008Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010090u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
        rfc_fr_bank[0u] = rfc_fr0;
        rfc_fr_bank[1u] = rfc_fr1;
        rfc_fr_bank[2u] = rfc_fr2;
        rfc_fr_bank[3u] = rfc_fr3;
    // SH-4 @ 0x8C010092u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    } else {
        ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C010082u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010082u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 0u, 0x3F800000u);
    // SH-4 @ 0x8C010084u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010084u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 1u, 0x3F800000u);
    // SH-4 @ 0x8C010086u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010086u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 2u, 0x3F800000u);
    // SH-4 @ 0x8C010088u: FLDI1
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010088u, "FLDI1 requires FPSCR.PR=0");
    dc_set_fr_bits(ctx, 3u, 0x3F800000u);
    // SH-4 @ 0x8C01008Au: FTRV
    ++runtime.fpu_ftrv_ops; if ((ctx.fpscr & (1u << 21)) != 0u) ++runtime.fpu_ftrv_fr1_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C01008Au);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C01008Au, "FTRV requires FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const auto& xfb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits; const float v0 = std::bit_cast<float>(frb[0u]); const float v1 = std::bit_cast<float>(frb[1u]); const float v2 = std::bit_cast<float>(frb[2u]); const float v3 = std::bit_cast<float>(frb[3u]); const double o0 = static_cast<double>(std::bit_cast<float>(xfb[0u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[4u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[8u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[12u])) * v3; const double o1 = static_cast<double>(std::bit_cast<float>(xfb[1u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[5u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[9u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[13u])) * v3; const double o2 = static_cast<double>(std::bit_cast<float>(xfb[2u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[6u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[10u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[14u])) * v3; const double o3 = static_cast<double>(std::bit_cast<float>(xfb[3u])) * v0 + static_cast<double>(std::bit_cast<float>(xfb[7u])) * v1 + static_cast<double>(std::bit_cast<float>(xfb[11u])) * v2 + static_cast<double>(std::bit_cast<float>(xfb[15u])) * v3; frb[0u] = std::bit_cast<std::uint32_t>(static_cast<float>(o0)); frb[1u] = std::bit_cast<std::uint32_t>(static_cast<float>(o1)); frb[2u] = std::bit_cast<std::uint32_t>(static_cast<float>(o2)); frb[3u] = std::bit_cast<std::uint32_t>(static_cast<float>(o3)); }
    // SH-4 @ 0x8C01008Cu: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01008Cu);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 3u) : static_cast<double>(dc_get_fr_float(ctx, 3u)));
    // SH-4 @ 0x8C01008Eu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010090u: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C010092u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;
    }
    }

BB_8C010094:
    ctx.pc = 0x8C010094u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(7u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 7u)) return;
    // SH-4 @ 0x8C010094u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C010096u: LDS_FPUL
    ctx.fpul = ctx.r[1];
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C010098u: FSCA
    ++runtime.fpu_fsca_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C010098u);
    if (dc_fpu_double_precision(ctx)) dc_unimplemented(0x8C010098u, "FSCA requires FPSCR.PR=0");
    { constexpr double tau = 6.283185307179586476925286766559; const double angle = static_cast<double>(ctx.fpul & 0xFFFFu) * tau / 65536.0; dc_set_fr_float(ctx, 8u, static_cast<float>(std::sin(angle))); dc_set_fr_float(ctx, 9u, static_cast<float>(std::cos(angle))); }
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C01009Au: FTRC
    dc_require_fpu_round_nearest(ctx, 0x8C01009Au);
    ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, 9u) : static_cast<double>(dc_get_fr_float(ctx, 9u)));
    // SH-4 @ 0x8C01009Cu: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C01009Eu: CMP_EQ_IMM
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[0] == static_cast<std::uint32_t>(static_cast<std::int32_t>(1))) ? 1u : 0u);
    // SH-4 @ 0x8C0100A0u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100AC;

BB_8C0100A2:
    ctx.pc = 0x8C0100A2u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(6u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C0100A2u: FSCHG
    ctx.fpscr ^= (1u << 20);
    if (runtime.fpu_aot_mode == 2u) sfc_flush(2u);
    // SH-4 @ 0x8C0100A4u: FSCHG
    ctx.fpscr ^= (1u << 20);
    // SH-4 @ 0x8C0100A6u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    if (runtime.fpu_aot_mode == 2u) sfc_flush(3u);
    // SH-4 @ 0x8C0100A8u: RETURN
    ctx.pc = ctx.pr;
    return;

BB_8C0100AC:
    ctx.pc = 0x8C0100ACu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (runtime.fpu_aot_mode == 2u) { if (sfc_tick(4u)) return; } else if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C0100ACu: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(-1));
    if (runtime.fpu_aot_mode == 2u) sfc_flush(3u);
    // SH-4 @ 0x8C0100AEu: RETURN
    ctx.pc = ctx.pr;
    return;

    if (runtime.fpu_aot_mode == 2u) sfc_flush(3u);
    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010012u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010024u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01003Cu, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010056u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010094u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C0100A2u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C0100ACu, &recomp_8C010000); // _main block
}

} // namespace dcrecomp_generated
