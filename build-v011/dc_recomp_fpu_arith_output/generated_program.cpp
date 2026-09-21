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
    switch (ctx.pc) {
        case 0x8C010000u: goto BB_8C010000;
        case 0x8C01003Eu: goto BB_8C01003E;
        case 0x8C010042u: goto BB_8C010042;
        case 0x8C01005Cu: goto BB_8C01005C;
        case 0x8C010060u: goto BB_8C010060;
        case 0x8C010078u: goto BB_8C010078;
        case 0x8C010080u: goto BB_8C010080;
        case 0x8C010084u: goto BB_8C010084;
        case 0x8C010094u: goto BB_8C010094;
        case 0x8C01009Cu: goto BB_8C01009C;
        case 0x8C0100A2u: goto BB_8C0100A2;
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 31u)) return;
    // SH-4 @ 0x8C010000u: LOAD_LITERAL32 -> _fpu_data
    ctx.r[4] = dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C0100A8u, 0x8C020000u);
    // SH-4 @ 0x8C010002u: MOV_REG
    ctx.r[6] = ctx.r[4];
    // SH-4 @ 0x8C010004u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(24));
    // SH-4 @ 0x8C010006u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010008u: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 0u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 0u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01000Au: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 1u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 1u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01000Cu: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 2u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01000Eu: FMOV_LOAD
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 3u, dc_guest_read32_hot(runtime, ctx.r[5])); } else {
        dc_set_fmov64_bits(ctx, 3u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); }
    // SH-4 @ 0x8C010010u: FADD
    dc_require_fpu_round_nearest(ctx, 0x8C010010u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 2u, dc_get_dr_double(ctx, 2u) + dc_get_dr_double(ctx, 1u));
    else dc_set_fr_float(ctx, 2u, dc_get_fr_float(ctx, 2u) + dc_get_fr_float(ctx, 1u));
    // SH-4 @ 0x8C010012u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C010014u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010016u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010018u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C01001Au: FMOV_LOAD
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_guest_read32_hot(runtime, ctx.r[5])); } else {
        dc_set_fmov64_bits(ctx, 2u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); }
    // SH-4 @ 0x8C01001Cu: FSUB
    dc_require_fpu_round_nearest(ctx, 0x8C01001Cu);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 2u, dc_get_dr_double(ctx, 2u) - dc_get_dr_double(ctx, 1u));
    else dc_set_fr_float(ctx, 2u, dc_get_fr_float(ctx, 2u) - dc_get_fr_float(ctx, 1u));
    // SH-4 @ 0x8C01001Eu: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C010020u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010022u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010024u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C010026u: FMOV_LOAD
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_guest_read32_hot(runtime, ctx.r[5])); } else {
        dc_set_fmov64_bits(ctx, 2u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); }
    // SH-4 @ 0x8C010028u: FMUL
    dc_require_fpu_round_nearest(ctx, 0x8C010028u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 2u, dc_get_dr_double(ctx, 2u) * dc_get_dr_double(ctx, 1u));
    else dc_set_fr_float(ctx, 2u, dc_get_fr_float(ctx, 2u) * dc_get_fr_float(ctx, 1u));
    // SH-4 @ 0x8C01002Au: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C01002Cu: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C01002Eu: FDIV
    dc_require_fpu_round_nearest(ctx, 0x8C01002Eu);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 3u, dc_get_dr_double(ctx, 3u) / dc_get_dr_double(ctx, 0u));
    else dc_set_fr_float(ctx, 3u, dc_get_fr_float(ctx, 3u) / dc_get_fr_float(ctx, 0u));
    // SH-4 @ 0x8C010030u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 3u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 3u)); }
    // SH-4 @ 0x8C010032u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010034u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010036u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C010038u: FMOV_LOAD
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_guest_read32_hot(runtime, ctx.r[5])); } else {
        dc_set_fmov64_bits(ctx, 2u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); }
    // SH-4 @ 0x8C01003Au: FCMP_EQ
    dc_require_fpu_round_nearest(ctx, 0x8C01003Au);
    { const bool result = dc_fpu_double_precision(ctx) ? (dc_get_dr_double(ctx, 2u) == dc_get_dr_double(ctx, 1u)) : (dc_get_fr_float(ctx, 2u) == dc_get_fr_float(ctx, 1u)); ctx.sr = (ctx.sr & ~1u) | (result ? 1u : 0u); }
    // SH-4 @ 0x8C01003Cu: BRANCH_IF_TRUE
    if ((ctx.sr & 1u) != 0u) goto BB_8C0100A2;

BB_8C01003E:
    ctx.pc = 0x8C01003Eu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C01003Eu: FCMP_GT
    dc_require_fpu_round_nearest(ctx, 0x8C01003Eu);
    { const bool result = dc_fpu_double_precision(ctx) ? (dc_get_dr_double(ctx, 2u) > dc_get_dr_double(ctx, 1u)) : (dc_get_fr_float(ctx, 2u) > dc_get_fr_float(ctx, 1u)); ctx.sr = (ctx.sr & ~1u) | (result ? 1u : 0u); }
    // SH-4 @ 0x8C010040u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C010042:
    ctx.pc = 0x8C010042u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 13u)) return;
    if ((ctx.fpscr & ((1u << 19) | (1u << 20) | 3u)) == 0u) {
        if (runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics) {
            ++runtime.fpu_block_cache_hits; runtime.fpu_block_cache_ops += 6u; runtime.fpu_block_cache_reuses += 1u; runtime.fpu_block_cache_writebacks += 4u;
            runtime.fpu_ftrv_ops += 0u; if ((ctx.fpscr & (1u << 21)) != 0u) runtime.fpu_ftrv_fr1_ops += 0u;
            runtime.fpu_fipr_ops += 0u; runtime.fpu_fmac_ops += 1u; runtime.fpu_fsca_ops += 0u; runtime.fpu_fsrra_ops += 0u;
        }
        auto& rfc_fr_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits;
        auto& rfc_xf_bank = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits;
        std::uint32_t rfc_fr0 = rfc_fr_bank[0u];
        std::uint32_t rfc_fr1 = rfc_fr_bank[1u];
        std::uint32_t rfc_fr2 = rfc_fr_bank[2u];
        std::uint32_t rfc_fr3 = 0u;
        std::uint32_t rfc_fr4 = 0u;
        std::uint32_t rfc_fr5 = 0u;
        // SH-4 @ 0x8C010042u: FMAC [FPU rfc_]
        rfc_fr2 = std::bit_cast<std::uint32_t>(dc_host_fma(std::bit_cast<float>(rfc_fr0), std::bit_cast<float>(rfc_fr1), std::bit_cast<float>(rfc_fr2)));
        // SH-4 @ 0x8C010044u: FMOV_STORE [FPU rfc_]
        dc_guest_write32_hot(runtime, ctx.r[6], rfc_fr2);
    // SH-4 @ 0x8C010046u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010048u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(64));
        // SH-4 @ 0x8C01004Au: FMOV_LOAD_POSTINC [FPU rfc_]
        rfc_fr3 = dc_guest_read32_hot(runtime, ctx.r[5]); ctx.r[5] += 4u;
        // SH-4 @ 0x8C01004Cu: FMOV_LOAD_POSTINC [FPU rfc_]
        rfc_fr2 = dc_guest_read32_hot(runtime, ctx.r[5]); ctx.r[5] += 4u;
        // SH-4 @ 0x8C01004Eu: FMOV_LOAD_POSTINC [FPU rfc_]
        rfc_fr5 = dc_guest_read32_hot(runtime, ctx.r[5]); ctx.r[5] += 4u;
        // SH-4 @ 0x8C010050u: FMOV_LOAD_POSTINC [FPU rfc_]
        rfc_fr4 = dc_guest_read32_hot(runtime, ctx.r[5]); ctx.r[5] += 4u;
    // SH-4 @ 0x8C010052u: LOAD_LITERAL32
    ctx.r[1] = dc_load_pc_literal32(runtime, 0x8C010052u, 0x8C0100ACu, 0x00080000u);
        rfc_fr_bank[2u] = rfc_fr2;
        rfc_fr_bank[3u] = rfc_fr3;
        rfc_fr_bank[4u] = rfc_fr4;
        rfc_fr_bank[5u] = rfc_fr5;
    } else {
        ++runtime.fpu_block_cache_fallbacks;
    // SH-4 @ 0x8C010042u: FMAC
    ++runtime.fpu_fmac_ops;
    dc_require_fpu_round_nearest(ctx, 0x8C010042u);
    if ((ctx.fpscr & (1u << 19)) != 0u) dc_unimplemented(0x8C010042u, "FMAC is only available when FPSCR.PR=0");
    { auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits; const float f0 = std::bit_cast<float>(frb[0u]); const float fs = std::bit_cast<float>(frb[1u]); const float fd = std::bit_cast<float>(frb[2u]); frb[2u] = std::bit_cast<std::uint32_t>(dc_host_fma(f0, fs, fd)); }
    // SH-4 @ 0x8C010044u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C010046u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010048u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(64));
    // SH-4 @ 0x8C01004Au: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 3u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 3u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01004Cu: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 2u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01004Eu: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 5u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 5u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C010050u: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 4u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 4u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C010052u: LOAD_LITERAL32
    ctx.r[1] = dc_load_pc_literal32(runtime, 0x8C010052u, 0x8C0100ACu, 0x00080000u);
    }
    // SH-4 @ 0x8C010054u: LDS_FPSCR
    dc_write_fpscr(ctx, ctx.r[1]);
    // SH-4 @ 0x8C010056u: FADD
    dc_require_fpu_round_nearest(ctx, 0x8C010056u);
    if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, 2u, dc_get_dr_double(ctx, 2u) + dc_get_dr_double(ctx, 4u));
    else dc_set_fr_float(ctx, 2u, dc_get_fr_float(ctx, 2u) + dc_get_fr_float(ctx, 4u));
    // SH-4 @ 0x8C010058u: FCMP_GT
    dc_require_fpu_round_nearest(ctx, 0x8C010058u);
    { const bool result = dc_fpu_double_precision(ctx) ? (dc_get_dr_double(ctx, 2u) > dc_get_dr_double(ctx, 4u)) : (dc_get_fr_float(ctx, 2u) > dc_get_fr_float(ctx, 4u)); ctx.sr = (ctx.sr & ~1u) | (result ? 1u : 0u); }
    // SH-4 @ 0x8C01005Au: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C01005C:
    ctx.pc = 0x8C01005Cu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C01005Cu: FCMP_EQ
    dc_require_fpu_round_nearest(ctx, 0x8C01005Cu);
    { const bool result = dc_fpu_double_precision(ctx) ? (dc_get_dr_double(ctx, 2u) == dc_get_dr_double(ctx, 2u)) : (dc_get_fr_float(ctx, 2u) == dc_get_fr_float(ctx, 2u)); ctx.sr = (ctx.sr & ~1u) | (result ? 1u : 0u); }
    // SH-4 @ 0x8C01005Eu: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C010060:
    ctx.pc = 0x8C010060u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 12u)) return;
    // SH-4 @ 0x8C010060u: MOV_REG
    ctx.r[6] = ctx.r[4];
    // SH-4 @ 0x8C010062u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(48));
    // SH-4 @ 0x8C010064u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 3u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 3u)); }
    // SH-4 @ 0x8C010066u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010068u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C01006Au: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C01006Cu: LDS_FPSCR
    dc_write_fpscr(ctx, ctx.r[1]);
    // SH-4 @ 0x8C01006Eu: MOV_REG
    ctx.r[6] = ctx.r[4];
    // SH-4 @ 0x8C010070u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(24));
    // SH-4 @ 0x8C010072u: MOV_REG
    ctx.r[7] = ctx.r[4];
    // SH-4 @ 0x8C010074u: ADD_IMM
    ctx.r[7] += static_cast<std::uint32_t>(static_cast<std::int32_t>(80));
    // SH-4 @ 0x8C010076u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(5));

BB_8C010078:
    ctx.pc = 0x8C010078u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010078u: LOAD32_POSTINC
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[6]);
    ctx.r[6] += 4u;
    // SH-4 @ 0x8C01007Au: LOAD32_POSTINC
    ctx.r[3] = dc_guest_read32_hot(runtime, ctx.r[7]);
    ctx.r[7] += 4u;
    // SH-4 @ 0x8C01007Cu: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[3]) ? 1u : 0u);
    // SH-4 @ 0x8C01007Eu: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C010080:
    ctx.pc = 0x8C010080u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C010080u: DT
    ctx.r[1] -= 1u;
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[1] == 0u) ? 1u : 0u);
    // SH-4 @ 0x8C010082u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010078;

BB_8C010084:
    ctx.pc = 0x8C010084u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 8u)) return;
    // SH-4 @ 0x8C010084u: MOV_REG
    ctx.r[6] = ctx.r[4];
    // SH-4 @ 0x8C010086u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(48));
    // SH-4 @ 0x8C010088u: MOV_REG
    ctx.r[7] = ctx.r[4];
    // SH-4 @ 0x8C01008Au: ADD_IMM
    ctx.r[7] += static_cast<std::uint32_t>(static_cast<std::int32_t>(100));
    // SH-4 @ 0x8C01008Cu: LOAD32_POSTINC
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[6]);
    ctx.r[6] += 4u;
    // SH-4 @ 0x8C01008Eu: LOAD32_POSTINC
    ctx.r[3] = dc_guest_read32_hot(runtime, ctx.r[7]);
    ctx.r[7] += 4u;
    // SH-4 @ 0x8C010090u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[3]) ? 1u : 0u);
    // SH-4 @ 0x8C010092u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C010094:
    ctx.pc = 0x8C010094u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010094u: LOAD32
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[6]);
    // SH-4 @ 0x8C010096u: LOAD32
    ctx.r[3] = dc_guest_read32_hot(runtime, ctx.r[7]);
    // SH-4 @ 0x8C010098u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[3]) ? 1u : 0u);
    // SH-4 @ 0x8C01009Au: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100A2;

BB_8C01009C:
    ctx.pc = 0x8C01009Cu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C01009Cu: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    // SH-4 @ 0x8C01009Eu: RETURN
    ctx.pc = ctx.pr;
    return;

BB_8C0100A2:
    ctx.pc = 0x8C0100A2u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C0100A2u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(-1));
    // SH-4 @ 0x8C0100A4u: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01003Eu, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010042u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01005Cu, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010060u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010078u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010080u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010084u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010094u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01009Cu, &recomp_8C010000); // _main block
    runtime.register_target(0x8C0100A2u, &recomp_8C010000); // _main block
}

} // namespace dcrecomp_generated
