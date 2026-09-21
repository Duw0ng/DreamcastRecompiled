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
    // SH-4 @ 0x8C010000u: LOAD_LITERAL32 -> _record
    ctx.r[4] = dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C010018u, 0x8C020000u);
    // SH-4 @ 0x8C010002u: LOAD_LITERAL32 -> _out
    ctx.r[5] = dc_load_pc_literal32(runtime, 0x8C010002u, 0x8C01001Cu, 0x8C030000u);
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

// Generated from _process_record @ 0x8C010080u
void recomp_8C010080(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    switch (ctx.pc) {
        case 0x8C010080u: goto BB_8C010080;
        default: goto BB_8C010080;
    }
BB_8C010080:
    ctx.pc = 0x8C010080u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 54u)) return;
    // SH-4 @ 0x8C010080u: LOAD8_DISP
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(dc_read8(runtime, ctx.r[4] + 0u))));
    // SH-4 @ 0x8C010082u: EXTU_B
    ctx.r[2] = ctx.r[0] & 0xFFu;
    // SH-4 @ 0x8C010084u: MOV_REG
    ctx.r[0] = ctx.r[2];
    // SH-4 @ 0x8C010086u: AND_IMM
    ctx.r[0] &= 15u;
    // SH-4 @ 0x8C010088u: MOV_REG
    ctx.r[2] = ctx.r[0];
    // SH-4 @ 0x8C01008Au: LOAD8_DISP
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(dc_read8(runtime, ctx.r[4] + 1u))));
    // SH-4 @ 0x8C01008Cu: EXTS_B
    ctx.r[6] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(ctx.r[0] & 0xFFu)));
    // SH-4 @ 0x8C01008Eu: LOAD16_DISP
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(dc_read16(runtime, ctx.r[4] + 2u))));
    // SH-4 @ 0x8C010090u: EXTU_W
    ctx.r[3] = ctx.r[0] & 0xFFFFu;
    // SH-4 @ 0x8C010092u: MOV_REG
    ctx.r[7] = ctx.r[3];
    // SH-4 @ 0x8C010094u: SHLR8
    ctx.r[7] >>= 8;
    // SH-4 @ 0x8C010096u: LOAD32_DISP
    ctx.r[8] = dc_guest_read32_hot(runtime, ctx.r[4] + 4u);
    // SH-4 @ 0x8C010098u: SHLR16
    ctx.r[8] >>= 16;
    // SH-4 @ 0x8C01009Au: MOV_REG
    ctx.r[0] = ctx.r[8];
    // SH-4 @ 0x8C01009Cu: AND_IMM
    ctx.r[0] &= 255u;
    // SH-4 @ 0x8C01009Eu: MOV_REG
    ctx.r[8] = ctx.r[0];
    // SH-4 @ 0x8C0100A0u: ADD_REG
    ctx.r[0] += ctx.r[2];
    // SH-4 @ 0x8C0100A2u: ADD_REG
    ctx.r[0] += ctx.r[7];
    // SH-4 @ 0x8C0100A4u: ADD_REG
    ctx.r[0] += ctx.r[6];
    // SH-4 @ 0x8C0100A6u: XOR_IMM
    ctx.r[0] ^= 8u;
    // SH-4 @ 0x8C0100A8u: OR_IMM
    ctx.r[0] |= 32u;
    // SH-4 @ 0x8C0100AAu: AND_IMM
    ctx.r[0] &= 42u;
    // SH-4 @ 0x8C0100ACu: STORE8_DISP
    dc_write8(runtime, ctx.r[5] + 0u, static_cast<std::uint8_t>(ctx.r[0] & 0xFFu));
    // SH-4 @ 0x8C0100AEu: STORE16_DISP
    dc_write16(runtime, ctx.r[5] + 2u, static_cast<std::uint16_t>(ctx.r[0] & 0xFFFFu));
    // SH-4 @ 0x8C0100B0u: STORE32_DISP
    dc_guest_write32_hot(runtime, ctx.r[5] + 4u, ctx.r[0]);
    // SH-4 @ 0x8C0100B2u: MOV_REG
    ctx.r[9] = ctx.r[0];
    // SH-4 @ 0x8C0100B4u: MOV_REG
    ctx.r[11] = ctx.r[4];
    // SH-4 @ 0x8C0100B6u: ADD_IMM
    ctx.r[11] += static_cast<std::uint32_t>(static_cast<std::int32_t>(8));
    // SH-4 @ 0x8C0100B8u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(3));
    // SH-4 @ 0x8C0100BAu: MOV_IMM
    ctx.r[12] = static_cast<std::uint32_t>(static_cast<std::int32_t>(127));
    // SH-4 @ 0x8C0100BCu: STORE8_INDEXED
    dc_write8(runtime, ctx.r[11] + ctx.r[0], static_cast<std::uint8_t>(ctx.r[12] & 0xFFu));
    // SH-4 @ 0x8C0100BEu: LOAD8_INDEXED
    ctx.r[13] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(dc_read8(runtime, ctx.r[11] + ctx.r[0]))));
    // SH-4 @ 0x8C0100C0u: EXTU_B
    ctx.r[13] = ctx.r[13] & 0xFFu;
    // SH-4 @ 0x8C0100C2u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(2));
    // SH-4 @ 0x8C0100C4u: LOAD8_INDEXED
    ctx.r[10] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(dc_read8(runtime, ctx.r[11] + ctx.r[0]))));
    // SH-4 @ 0x8C0100C6u: EXTU_B
    ctx.r[10] = ctx.r[10] & 0xFFu;
    // SH-4 @ 0x8C0100C8u: SHLL2
    ctx.r[10] <<= 2;
    // SH-4 @ 0x8C0100CAu: SHLR2
    ctx.r[10] >>= 2;
    // SH-4 @ 0x8C0100CCu: SHLL8
    ctx.r[10] <<= 8;
    // SH-4 @ 0x8C0100CEu: SHLR8
    ctx.r[10] >>= 8;
    // SH-4 @ 0x8C0100D0u: ROTL
    { const std::uint32_t old = ctx.r[10]; const std::uint32_t bit = old >> 31; ctx.r[10] = (old << 1) | bit; ctx.sr = (ctx.sr & ~1u) | bit; }
    // SH-4 @ 0x8C0100D2u: ROTR
    { const std::uint32_t old = ctx.r[10]; const std::uint32_t bit = old & 1u; ctx.r[10] = (old >> 1) | (bit << 31); ctx.sr = (ctx.sr & ~1u) | bit; }
    // SH-4 @ 0x8C0100D4u: AND_REG
    ctx.r[13] &= ctx.r[10];
    // SH-4 @ 0x8C0100D6u: XOR_REG
    ctx.r[13] ^= ctx.r[10];
    // SH-4 @ 0x8C0100D8u: OR_REG
    ctx.r[13] |= ctx.r[10];
    // SH-4 @ 0x8C0100DAu: MOV_REG
    ctx.r[0] = ctx.r[9];
    // SH-4 @ 0x8C0100DCu: LOAD8_DISP
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(dc_read8(runtime, ctx.r[5] + 0u))));
    // SH-4 @ 0x8C0100DEu: EXTU_B
    ctx.r[0] = ctx.r[0] & 0xFFu;
    // SH-4 @ 0x8C0100E0u: LOAD16_DISP
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(dc_read16(runtime, ctx.r[5] + 2u))));
    // SH-4 @ 0x8C0100E2u: EXTU_W
    ctx.r[0] = ctx.r[0] & 0xFFFFu;
    // SH-4 @ 0x8C0100E4u: LOAD32_DISP
    ctx.r[0] = dc_guest_read32_hot(runtime, ctx.r[5] + 4u);
    // SH-4 @ 0x8C0100E6u: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C01000Au, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010080u, &recomp_8C010080); // _process_record block
}

} // namespace dcrecomp_generated
