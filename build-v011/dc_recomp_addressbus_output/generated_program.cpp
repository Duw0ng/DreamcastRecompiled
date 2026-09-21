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
        case 0x8C01000Au: goto BB_8C01000A;
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    // SH-4 @ 0x8C010000u: LOAD_LITERAL32
    ctx.r[4] = dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C010010u, 0x8C100000u);
    // SH-4 @ 0x8C010002u: MOV_IMM
    ctx.r[5] = static_cast<std::uint32_t>(static_cast<std::int32_t>(64));
    // SH-4 @ 0x8C010004u: PUSH_PR
    ctx.r[15] -= 4u;
    dc_guest_write32_hot(runtime, ctx.r[15], ctx.pr);
    // SH-4 @ 0x8C010006u: CALL
    ctx.pr = 0x8C01000Au;
    if (runtime.direct_dispatch_ready) {
        DCR_HOT_METRIC_INC(runtime.direct_static_calls);
        dc_trace_record(runtime, runtime.current_pc, 0x8C010080u, ctx.r[8], ctx.r[15]);
        ctx.pc = 0x8C010080u;
        recomp_8C010080(ctx, runtime);
        if (!runtime.sh4_async_redirect && ctx.pc == 0x8C010080u) ctx.pc = 0x8C01000Au;
    } else {
        ++runtime.direct_dispatch_fallbacks;
        call_recompiled(ctx, runtime, 0x8C010080u);
    }
    if (runtime.sh4_async_redirect || ctx.pc != 0x8C01000Au) return;
    ctx.pc = 0x8C01000Au;

BB_8C01000A:
    ctx.pc = 0x8C01000Au;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C01000Au: POP_PR
    ctx.pr = dc_guest_read32_hot(runtime, ctx.r[15]);
    ctx.r[15] += 4u;
    // SH-4 @ 0x8C01000Cu: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

// Generated from _memTestAddressBus @ 0x8C010080u
void recomp_8C010080(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    switch (ctx.pc) {
        case 0x8C010080u: goto BB_8C010080;
        case 0x8C01008Cu: goto BB_8C01008C;
        case 0x8C01009Au: goto BB_8C01009A;
        case 0x8C01009Eu: goto BB_8C01009E;
        case 0x8C0100A8u: goto BB_8C0100A8;
        case 0x8C0100B0u: goto BB_8C0100B0;
        case 0x8C0100B4u: goto BB_8C0100B4;
        case 0x8C0100C0u: goto BB_8C0100C0;
        case 0x8C0100C2u: goto BB_8C0100C2;
        case 0x8C0100CCu: goto BB_8C0100CC;
        case 0x8C0100D0u: goto BB_8C0100D0;
        case 0x8C0100D8u: goto BB_8C0100D8;
        case 0x8C0100E6u: goto BB_8C0100E6;
        case 0x8C0100ECu: goto BB_8C0100EC;
        case 0x8C0100F6u: goto BB_8C0100F6;
        default: goto BB_8C010080;
    }
BB_8C010080:
    ctx.pc = 0x8C010080u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    // SH-4 @ 0x8C010080u: MOV_REG
    ctx.r[6] = ctx.r[5];
    // SH-4 @ 0x8C010082u: SHLR2
    ctx.r[6] >>= 2;
    // SH-4 @ 0x8C010084u: ADD_IMM
    ctx.r[6] += static_cast<std::uint32_t>(static_cast<std::int32_t>(-1));
    // SH-4 @ 0x8C010086u: LOAD_LITERAL32
    ctx.r[9] = dc_load_pc_literal32(runtime, 0x8C010086u, 0x8C010100u, 0xAAAAAAAAu);
    // SH-4 @ 0x8C010088u: LOAD_LITERAL32
    ctx.r[10] = dc_load_pc_literal32(runtime, 0x8C010088u, 0x8C010104u, 0x55555555u);
    // SH-4 @ 0x8C01008Au: MOV_IMM
    ctx.r[7] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));

BB_8C01008C:
    ctx.pc = 0x8C01008Cu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 7u)) return;
    // SH-4 @ 0x8C01008Cu: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C01008Eu: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C010090u: STORE32_INDEXED
    dc_guest_write32_hot(runtime, ctx.r[4] + ctx.r[0], ctx.r[9]);
    // SH-4 @ 0x8C010092u: SHLL
    { const std::uint32_t old = ctx.r[7]; ctx.sr = (ctx.sr & ~1u) | ((old >> 31) & 1u); ctx.r[7] = old << 1; }
    // SH-4 @ 0x8C010094u: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C010096u: TST_REG
    ctx.sr = (ctx.sr & ~1u) | (((ctx.r[0] & ctx.r[6]) == 0u) ? 1u : 0u);
    // SH-4 @ 0x8C010098u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C01008C;

BB_8C01009A:
    ctx.pc = 0x8C01009Au;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C01009Au: STORE32
    dc_guest_write32_hot(runtime, ctx.r[4], ctx.r[10]);
    // SH-4 @ 0x8C01009Cu: MOV_IMM
    ctx.r[7] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));

BB_8C01009E:
    ctx.pc = 0x8C01009Eu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 5u)) return;
    // SH-4 @ 0x8C01009Eu: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C0100A0u: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100A2u: LOAD32_INDEXED
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[4] + ctx.r[0]);
    // SH-4 @ 0x8C0100A4u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[1] == ctx.r[9]) ? 1u : 0u);
    // SH-4 @ 0x8C0100A6u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100EC;

BB_8C0100A8:
    ctx.pc = 0x8C0100A8u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C0100A8u: SHLL
    { const std::uint32_t old = ctx.r[7]; ctx.sr = (ctx.sr & ~1u) | ((old >> 31) & 1u); ctx.r[7] = old << 1; }
    // SH-4 @ 0x8C0100AAu: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C0100ACu: TST_REG
    ctx.sr = (ctx.sr & ~1u) | (((ctx.r[0] & ctx.r[6]) == 0u) ? 1u : 0u);
    // SH-4 @ 0x8C0100AEu: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C01009E;

BB_8C0100B0:
    ctx.pc = 0x8C0100B0u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C0100B0u: STORE32
    dc_guest_write32_hot(runtime, ctx.r[4], ctx.r[9]);
    // SH-4 @ 0x8C0100B2u: MOV_IMM
    ctx.r[8] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));

BB_8C0100B4:
    ctx.pc = 0x8C0100B4u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    // SH-4 @ 0x8C0100B4u: MOV_REG
    ctx.r[0] = ctx.r[8];
    // SH-4 @ 0x8C0100B6u: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100B8u: STORE32_INDEXED
    dc_guest_write32_hot(runtime, ctx.r[4] + ctx.r[0], ctx.r[10]);
    // SH-4 @ 0x8C0100BAu: LOAD32
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[4]);
    // SH-4 @ 0x8C0100BCu: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[1] == ctx.r[9]) ? 1u : 0u);
    // SH-4 @ 0x8C0100BEu: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100F6;

BB_8C0100C0:
    ctx.pc = 0x8C0100C0u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 1u)) return;
    // SH-4 @ 0x8C0100C0u: MOV_IMM
    ctx.r[7] = static_cast<std::uint32_t>(static_cast<std::int32_t>(1));

BB_8C0100C2:
    ctx.pc = 0x8C0100C2u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 5u)) return;
    // SH-4 @ 0x8C0100C2u: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C0100C4u: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100C6u: LOAD32_INDEXED
    ctx.r[1] = dc_guest_read32_hot(runtime, ctx.r[4] + ctx.r[0]);
    // SH-4 @ 0x8C0100C8u: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[1] == ctx.r[9]) ? 1u : 0u);
    // SH-4 @ 0x8C0100CAu: BRANCH_IF_TRUE
    if ((ctx.sr & 1u) != 0u) goto BB_8C0100D0;

BB_8C0100CC:
    ctx.pc = 0x8C0100CCu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 2u)) return;
    // SH-4 @ 0x8C0100CCu: CMP_EQ
    ctx.sr = (ctx.sr & ~1u) | ((ctx.r[7] == ctx.r[8]) ? 1u : 0u);
    // SH-4 @ 0x8C0100CEu: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100F6;

BB_8C0100D0:
    ctx.pc = 0x8C0100D0u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C0100D0u: SHLL
    { const std::uint32_t old = ctx.r[7]; ctx.sr = (ctx.sr & ~1u) | ((old >> 31) & 1u); ctx.r[7] = old << 1; }
    // SH-4 @ 0x8C0100D2u: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C0100D4u: TST_REG
    ctx.sr = (ctx.sr & ~1u) | (((ctx.r[0] & ctx.r[6]) == 0u) ? 1u : 0u);
    // SH-4 @ 0x8C0100D6u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100C2;

BB_8C0100D8:
    ctx.pc = 0x8C0100D8u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 7u)) return;
    // SH-4 @ 0x8C0100D8u: MOV_REG
    ctx.r[0] = ctx.r[8];
    // SH-4 @ 0x8C0100DAu: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100DCu: STORE32_INDEXED
    dc_guest_write32_hot(runtime, ctx.r[4] + ctx.r[0], ctx.r[9]);
    // SH-4 @ 0x8C0100DEu: SHLL
    { const std::uint32_t old = ctx.r[8]; ctx.sr = (ctx.sr & ~1u) | ((old >> 31) & 1u); ctx.r[8] = old << 1; }
    // SH-4 @ 0x8C0100E0u: MOV_REG
    ctx.r[0] = ctx.r[8];
    // SH-4 @ 0x8C0100E2u: TST_REG
    ctx.sr = (ctx.sr & ~1u) | (((ctx.r[0] & ctx.r[6]) == 0u) ? 1u : 0u);
    // SH-4 @ 0x8C0100E4u: BRANCH_IF_FALSE
    if ((ctx.sr & 1u) == 0u) goto BB_8C0100B4;

BB_8C0100E6:
    ctx.pc = 0x8C0100E6u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C0100E6u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(0));
    // SH-4 @ 0x8C0100E8u: RETURN
    ctx.pc = ctx.pr;
    return;

BB_8C0100EC:
    ctx.pc = 0x8C0100ECu;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    // SH-4 @ 0x8C0100ECu: MOV_REG
    ctx.r[0] = ctx.r[7];
    // SH-4 @ 0x8C0100EEu: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100F0u: ADD_REG
    ctx.r[0] += ctx.r[4];
    // SH-4 @ 0x8C0100F2u: RETURN
    ctx.pc = ctx.pr;
    return;

BB_8C0100F6:
    ctx.pc = 0x8C0100F6u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 6u)) return;
    // SH-4 @ 0x8C0100F6u: MOV_REG
    ctx.r[0] = ctx.r[8];
    // SH-4 @ 0x8C0100F8u: SHLL2
    ctx.r[0] <<= 2;
    // SH-4 @ 0x8C0100FAu: ADD_REG
    ctx.r[0] += ctx.r[4];
    // SH-4 @ 0x8C0100FCu: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01000Au, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010080u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C01008Cu, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C01009Au, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C01009Eu, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100A8u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100B0u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100B4u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100C0u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100C2u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100CCu, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100D0u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100D8u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100E6u, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100ECu, &recomp_8C010080); // _memTestAddressBus block
    runtime.register_target(0x8C0100F6u, &recomp_8C010080); // _memTestAddressBus block
}

} // namespace dcrecomp_generated
