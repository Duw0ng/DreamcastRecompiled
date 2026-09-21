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
    if (dc_runtime_tick_fast(ctx, runtime, 35u)) return;
    // SH-4 @ 0x8C010000u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    // SH-4 @ 0x8C010002u: LDC_GBR
    ctx.gbr = ctx.r[1];
    // SH-4 @ 0x8C010004u: STC_GBR
    ctx.r[0] = ctx.gbr;
    // SH-4 @ 0x8C010006u: SET_S
    ctx.sr |= 2u;
    // SH-4 @ 0x8C010008u: CLEAR_S
    ctx.sr &= ~2u;
    // SH-4 @ 0x8C01000Au: LDC_VBR
    ctx.vbr = ctx.r[1];
    // SH-4 @ 0x8C01000Cu: STC_VBR
    ctx.r[2] = ctx.vbr;
    // SH-4 @ 0x8C01000Eu: STC_L_GBR
    ctx.r[15] -= 4u;
    dc_guest_write32_hot(runtime, ctx.r[15], ctx.gbr);
    // SH-4 @ 0x8C010010u: MOV_IMM
    ctx.r[1] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C010012u: LDC_GBR
    ctx.gbr = ctx.r[1];
    // SH-4 @ 0x8C010014u: LDC_L_GBR
    { const std::uint32_t dc_tmp = dc_guest_read32_hot(runtime, ctx.r[15]); ctx.r[15] += 4u; ctx.gbr = dc_tmp; }
    // SH-4 @ 0x8C010016u: STC_GBR
    ctx.r[3] = ctx.gbr;
    // SH-4 @ 0x8C010018u: LDS_MACH
    ctx.mach = ctx.r[0];
    // SH-4 @ 0x8C01001Au: STS_L_MACH
    ctx.r[15] -= 4u;
    dc_guest_write32_hot(runtime, ctx.r[15], ctx.mach);
    // SH-4 @ 0x8C01001Cu: CLEAR_MAC
    ctx.mach = 0u; ctx.macl = 0u;
    // SH-4 @ 0x8C01001Eu: LDS_L_MACH
    ctx.mach = dc_guest_read32_hot(runtime, ctx.r[15]);
    ctx.r[15] += 4u;
    // SH-4 @ 0x8C010020u: STS_MACH
    ctx.r[4] = ctx.mach;
    // SH-4 @ 0x8C010022u: LOAD_LITERAL32 -> _system_word
    ctx.r[5] = dc_load_pc_literal32(runtime, 0x8C010022u, 0x8C010030u, 0x8C020000u);
    // SH-4 @ 0x8C010024u: PREF
    dc_guest_pref(runtime, ctx.r[5], 0x8C010024u);
    // SH-4 @ 0x8C010026u: MOVCA_L
    dc_guest_write32_hot(runtime, ctx.r[5], ctx.r[0]);
    // SH-4 @ 0x8C010028u: LOAD32
    ctx.r[6] = dc_guest_read32_hot(runtime, ctx.r[5]);
    // SH-4 @ 0x8C01002Au: MOV_REG
    ctx.r[0] = ctx.r[6];
    // SH-4 @ 0x8C01002Cu: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
}

} // namespace dcrecomp_generated
