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
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 43u)) return;
    // SH-4 @ 0x8C010000u: LOAD_LITERAL32 -> _tas_byte
    ctx.r[10] = dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C01004Cu, 0x8C020000u);
    // SH-4 @ 0x8C010002u: TAS_B
    { const std::uint8_t old = dc_read8(runtime, ctx.r[10]); ctx.sr = (ctx.sr & ~1u) | ((old == 0u) ? 1u : 0u); dc_write8(runtime, ctx.r[10], static_cast<std::uint8_t>(old | 0x80u)); }
    // SH-4 @ 0x8C010004u: MOV_T
    ctx.r[11] = ctx.sr & 1u;
    // SH-4 @ 0x8C010006u: TAS_B
    { const std::uint8_t old = dc_read8(runtime, ctx.r[10]); ctx.sr = (ctx.sr & ~1u) | ((old == 0u) ? 1u : 0u); dc_write8(runtime, ctx.r[10], static_cast<std::uint8_t>(old | 0x80u)); }
    // SH-4 @ 0x8C010008u: MOV_T
    ctx.r[12] = ctx.sr & 1u;
    // SH-4 @ 0x8C01000Au: CLEAR_T
    ctx.sr &= ~1u;
    // SH-4 @ 0x8C01000Cu: SET_T
    ctx.sr |= 1u;
    // SH-4 @ 0x8C01000Eu: CLEAR_MAC
    ctx.mach = 0u; ctx.macl = 0u;
    // SH-4 @ 0x8C010010u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(6));
    // SH-4 @ 0x8C010012u: MOV_IMM
    ctx.r[2] = static_cast<std::uint32_t>(static_cast<std::int32_t>(7));
    // SH-4 @ 0x8C010014u: MUL_L
    ctx.macl = static_cast<std::uint32_t>(static_cast<std::uint64_t>(ctx.r[1]) * static_cast<std::uint64_t>(ctx.r[2]));
    // SH-4 @ 0x8C010016u: STS_MACL
    ctx.r[0] = ctx.macl;
    // SH-4 @ 0x8C010018u: MOV_IMM
    ctx.r[3] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));
    // SH-4 @ 0x8C01001Au: SHLD
    { const std::uint32_t amount = ctx.r[3]; const std::uint32_t old = ctx.r[0]; if ((amount & 0x80000000u) == 0u) { ctx.r[0] = old << (amount & 0x1Fu); } else { const std::uint32_t count = ((~amount) & 0x1Fu) + 1u; ctx.r[0] = (count >= 32u) ? 0u : (old >> count); } }
    // SH-4 @ 0x8C01001Cu: MOV_IMM
    ctx.r[3] = static_cast<std::uint32_t>(static_cast<std::int32_t>(-1));
    // SH-4 @ 0x8C01001Eu: SHLD
    { const std::uint32_t amount = ctx.r[3]; const std::uint32_t old = ctx.r[0]; if ((amount & 0x80000000u) == 0u) { ctx.r[0] = old << (amount & 0x1Fu); } else { const std::uint32_t count = ((~amount) & 0x1Fu) + 1u; ctx.r[0] = (count >= 32u) ? 0u : (old >> count); } }
    // SH-4 @ 0x8C010020u: LDS_FPUL
    ctx.fpul = ctx.r[0];
    // SH-4 @ 0x8C010022u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C010024u: STS_FPUL
    ctx.r[0] = ctx.fpul;
    // SH-4 @ 0x8C010026u: MULU_W
    ctx.macl = static_cast<std::uint32_t>(static_cast<std::uint16_t>(ctx.r[1]) * static_cast<std::uint16_t>(ctx.r[2]));
    // SH-4 @ 0x8C010028u: MULS_W
    ctx.macl = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(ctx.r[1])) * static_cast<std::int32_t>(static_cast<std::int16_t>(ctx.r[2])));
    // SH-4 @ 0x8C01002Au: DMULU_L
    { const std::uint64_t product = static_cast<std::uint64_t>(ctx.r[1]) * static_cast<std::uint64_t>(ctx.r[2]); ctx.macl = static_cast<std::uint32_t>(product); ctx.mach = static_cast<std::uint32_t>(product >> 32); }
    // SH-4 @ 0x8C01002Cu: DMULS_L
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.r[1])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.r[2])); const std::uint64_t bits = static_cast<std::uint64_t>(product); ctx.macl = static_cast<std::uint32_t>(bits); ctx.mach = static_cast<std::uint32_t>(bits >> 32); }
    // SH-4 @ 0x8C01002Eu: SWAP_B
    { const std::uint32_t v = ctx.r[1]; ctx.r[4] = (v & 0xFFFF0000u) | ((v & 0x000000FFu) << 8) | ((v & 0x0000FF00u) >> 8); }
    // SH-4 @ 0x8C010030u: SWAP_W
    { const std::uint32_t v = ctx.r[1]; ctx.r[5] = (v << 16) | (v >> 16); }
    // SH-4 @ 0x8C010032u: XTRCT
    { const std::uint32_t old_n = ctx.r[6]; const std::uint32_t old_m = ctx.r[1]; ctx.r[6] = (old_n >> 16) | (old_m << 16); }
    // SH-4 @ 0x8C010034u: LDS_MACH
    ctx.mach = ctx.r[1];
    // SH-4 @ 0x8C010036u: STS_MACH
    ctx.r[7] = ctx.mach;
    // SH-4 @ 0x8C010038u: LDS_MACL
    ctx.macl = ctx.r[2];
    // SH-4 @ 0x8C01003Au: STS_MACL
    ctx.r[8] = ctx.macl;
    // SH-4 @ 0x8C01003Cu: LDS_FPSCR
    dc_write_fpscr(ctx, ctx.r[3]);
    // SH-4 @ 0x8C01003Eu: STS_FPSCR
    ctx.r[9] = ctx.fpscr;
    // SH-4 @ 0x8C010040u: MOV_IMM
    ctx.r[4] = static_cast<std::uint32_t>(static_cast<std::int32_t>(21));
    // SH-4 @ 0x8C010042u: MOV_IMM
    ctx.r[3] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));
    // SH-4 @ 0x8C010044u: SHAD
    { const std::uint32_t amount = ctx.r[3]; const std::uint32_t old = ctx.r[4]; if ((amount & 0x80000000u) == 0u) { ctx.r[4] = old << (amount & 0x1Fu); } else { const std::uint32_t count = ((~amount) & 0x1Fu) + 1u; const std::int32_t signed_old = static_cast<std::int32_t>(old); ctx.r[4] = (count >= 32u) ? (signed_old < 0 ? 0xFFFFFFFFu : 0u) : static_cast<std::uint32_t>(signed_old >> count); } }
    // SH-4 @ 0x8C010046u: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
}

} // namespace dcrecomp_generated
