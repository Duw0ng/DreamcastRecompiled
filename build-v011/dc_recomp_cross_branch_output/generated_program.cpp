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
        case 0x8C010006u: goto BB_8C010006;
        default: goto BB_8C010000;
    }
BB_8C010000:
    ctx.pc = 0x8C010000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010000u: PUSH_PR
    ctx.r[15] -= 4u;
    dc_guest_write32_hot(runtime, ctx.r[15], ctx.pr);
    // SH-4 @ 0x8C010002u: CALL
    ctx.pr = 0x8C010006u;
    if (runtime.direct_dispatch_ready) {
        DCR_HOT_METRIC_INC(runtime.direct_static_calls);
        dc_trace_record(runtime, runtime.current_pc, 0x8C010080u, ctx.r[8], ctx.r[15]);
        ctx.pc = 0x8C010080u;
        recomp_8C010080(ctx, runtime);
        if (!runtime.sh4_async_redirect && ctx.pc == 0x8C010080u) ctx.pc = 0x8C010006u;
    } else {
        ++runtime.direct_dispatch_fallbacks;
        call_recompiled(ctx, runtime, 0x8C010080u);
    }
    if (runtime.sh4_async_redirect || ctx.pc != 0x8C010006u) return;
    ctx.pc = 0x8C010006u;

BB_8C010006:
    ctx.pc = 0x8C010006u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010006u: POP_PR
    ctx.pr = dc_guest_read32_hot(runtime, ctx.r[15]);
    ctx.r[15] += 4u;
    // SH-4 @ 0x8C010008u: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

// Generated from _shared+0x8 @ 0x8C010048u
void recomp_8C010048(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    switch (ctx.pc) {
        case 0x8C010048u: goto BB_8C010048;
        default: goto BB_8C010048;
    }
BB_8C010048:
    ctx.pc = 0x8C010048u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010048u: LOAD32_POSTINC
    ctx.r[4] = dc_guest_read32_hot(runtime, ctx.r[15]);
    ctx.r[15] += 4u;
    // SH-4 @ 0x8C01004Au: RETURN
    ctx.pc = ctx.pr;
    return;

    return;
}

// Generated from _signed @ 0x8C010080u
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
    if (dc_runtime_tick_fast(ctx, runtime, 4u)) return;
    // SH-4 @ 0x8C010080u: STORE32_PREDEC
    ctx.r[15] -= 4u;
    dc_guest_write32_hot(runtime, ctx.r[15], ctx.r[4]);
    // SH-4 @ 0x8C010082u: MOV_IMM
    ctx.r[0] = static_cast<std::uint32_t>(static_cast<std::int32_t>(42));
    // SH-4 @ 0x8C010084u: BRANCH
    if (runtime.direct_dispatch_ready) {
        DCR_HOT_METRIC_INC(runtime.direct_tail_calls);
        dc_trace_record(runtime, runtime.current_pc, 0x8C010048u, ctx.r[8], ctx.r[15]);
        ctx.pc = 0x8C010048u;
        recomp_8C010048(ctx, runtime);
    } else {
        ++runtime.direct_dispatch_fallbacks;
        call_recompiled(ctx, runtime, 0x8C010048u);
    }
    return;

    return;
}

void register_recompiled_program(DCRuntime& runtime) {
    runtime.register_target(0x8C010000u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010006u, &recomp_8C010000); // _main block
    runtime.register_target(0x8C010048u, &recomp_8C010048); // _shared+0x8 block
    runtime.register_target(0x8C010080u, &recomp_8C010080); // _signed block
}

} // namespace dcrecomp_generated
