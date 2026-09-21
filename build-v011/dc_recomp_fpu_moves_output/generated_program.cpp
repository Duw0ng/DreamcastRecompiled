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
        case 0x8C01002Au: goto BB_8C01002A;
        case 0x8C01003Au: goto BB_8C01003A;
        case 0x8C010042u: goto BB_8C010042;
        case 0x8C01004Au: goto BB_8C01004A;
        case 0x8C010052u: goto BB_8C010052;
        case 0x8C010058u: goto BB_8C010058;
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 21u)) return;
    // SH-4 @ 0x8C010000u: LOAD_LITERAL32 -> _fpu_data
    ctx.r[4] = dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C010060u, 0x8C020000u);
    // SH-4 @ 0x8C010002u: MOV_REG
    ctx.r[5] = ctx.r[4];
    // SH-4 @ 0x8C010004u: ADD_IMM
    ctx.r[5] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010006u: MOV_REG
    ctx.r[6] = ctx.r[4];
    // SH-4 @ 0x8C010008u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(16));
    // SH-4 @ 0x8C01000Au: MOV_REG
    ctx.r[7] = ctx.r[6];
    // SH-4 @ 0x8C01000Cu: ADD_IMM
    ctx.r[7] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C01000Eu: MOV_REG
    ctx.r[8] = ctx.r[6];
    // SH-4 @ 0x8C010010u: ADD_IMM
    ctx.r[8] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C010012u: FMOV_LOAD
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 1u, dc_guest_read32_hot(runtime, ctx.r[4])); } else {
        dc_set_fmov64_bits(ctx, 1u, dc_guest_read64_fmov_hot(runtime, ctx.r[4])); }
    // SH-4 @ 0x8C010014u: FMOV_REG
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 2u, dc_get_fr_bits(ctx, 1u)); } else {
        dc_set_fmov64_bits(ctx, 2u, dc_get_fmov64_bits(ctx, 1u)); }
    // SH-4 @ 0x8C010016u: FMOV_STORE
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_guest_write32_hot(runtime, ctx.r[6], dc_get_fr_bits(ctx, 2u)); } else {
        const std::uint32_t address = ctx.r[6]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 2u)); }
    // SH-4 @ 0x8C010018u: FMOV_LOAD_POSTINC
    if ((ctx.fpscr & (1u << 20)) == 0u) { dc_set_fr_bits(ctx, 3u, dc_guest_read32_hot(runtime, ctx.r[5])); ctx.r[5] += 4u; } else {
        dc_set_fmov64_bits(ctx, 3u, dc_guest_read64_fmov_hot(runtime, ctx.r[5])); ctx.r[5] += 8u; }
    // SH-4 @ 0x8C01001Au: FMOV_STORE_PREDEC
    if ((ctx.fpscr & (1u << 20)) == 0u) { ctx.r[7] -= 4u; dc_guest_write32_hot(runtime, ctx.r[7], dc_get_fr_bits(ctx, 3u)); } else {
        ctx.r[7] -= 8u; const std::uint32_t address = ctx.r[7]; dc_guest_write64_fmov_hot(runtime, address, dc_get_fmov64_bits(ctx, 3u)); }
    // SH-4 @ 0x8C01001Cu: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C01001Eu: FMOV_INDEXED_LOAD
    { const std::uint32_t address = ctx.r[0] + ctx.r[4]; if ((ctx.fpscr & (1u << 20)) == 0u) dc_set_fr_bits(ctx, 4u, dc_guest_read32_hot(runtime, address)); else {
        dc_set_fmov64_bits(ctx,4u,dc_guest_read64_fmov_hot(runtime,address)); } }
    // SH-4 @ 0x8C010020u: FMOV_INDEXED_STORE
    { const std::uint32_t address = ctx.r[8] + ctx.r[0]; if ((ctx.fpscr & (1u << 20)) == 0u) dc_guest_write32_hot(runtime, address, dc_get_fr_bits(ctx, 4u)); else {
        dc_guest_write64_fmov_hot(runtime,address,dc_get_fmov64_bits(ctx,4u)); } }
    // SH-4 @ 0x8C010022u: LOAD32
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[4]);
    // SH-4 @ 0x8C010024u: LOAD32
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[6]);
    // SH-4 @ 0x8C010026u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[1]) ? 1u : 0u);
    // SH-4 @ 0x8C010028u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010058;

BB_8C01002A:
    ctx.pc = 0x8C01002Au;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 8u)) return;
    // SH-4 @ 0x8C01002Au: MOV_REG
    ctx.r[9] = ctx.r[4];
    // SH-4 @ 0x8C01002Cu: ADD_IMM
    ctx.r[9] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C01002Eu: LOAD32
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[9]);
    // SH-4 @ 0x8C010030u: MOV_REG
    ctx.r[10] = ctx.r[6];
    // SH-4 @ 0x8C010032u: ADD_IMM
    ctx.r[10] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C010034u: LOAD32
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[10]);
    // SH-4 @ 0x8C010036u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[1]) ? 1u : 0u);
    // SH-4 @ 0x8C010038u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010058;

BB_8C01003A:
    ctx.pc = 0x8C01003Au;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C01003Au: LOAD32
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[4]);
    // SH-4 @ 0x8C01003Cu: LOAD32
    ctx.r[2] = dc_guest_read32_hot(runtime, ctx.r[8]);
    // SH-4 @ 0x8C01003Eu: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[2] == ctx.r[1]) ? 1u : 0u);
    // SH-4 @ 0x8C010040u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010058;

BB_8C010042:
    ctx.pc = 0x8C010042u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010042u: MOV_REG
    ctx.r[10] = ctx.r[4];
    // SH-4 @ 0x8C010044u: ADD_IMM
    ctx.r[10] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C010046u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[5] == ctx.r[10]) ? 1u : 0u);
    // SH-4 @ 0x8C010048u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010058;

BB_8C01004A:
    ctx.pc = 0x8C01004Au;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C01004Au: MOV_REG
    ctx.r[10] = ctx.r[6];
    // SH-4 @ 0x8C01004Cu: ADD_IMM
    ctx.r[10] += static_cast<std::uint32_t>(static_cast<std::int32_t>(4));
    // SH-4 @ 0x8C01004Eu: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[7] == ctx.r[10]) ? 1u : 0u);
    // SH-4 @ 0x8C010050u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C010058;

BB_8C010052:
    ctx.pc = 0x8C010052u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010052u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    // SH-4 @ 0x8C010054u: RETURN
    ctx.pc = ctx.pr;
    return;

BB_8C010058:
    ctx.pc = 0x8C010058u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010058u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(-1));
    // SH-4 @ 0x8C01005Au: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01002Au, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01003Au, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010042u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01004Au, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010052u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010058u, &recomp_8C010000); // _main block
}

} // namespace dcrecomp_generated
