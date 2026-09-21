#include "generated_gpr_static_cache_emit_probe.hpp"

#include <bit>
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

void recomp_00008800(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    // 0.0.163: multi-entry-safe static SH-4 integer register cache.
    bool gpc_active = false;
    std::uint32_t gpc_r2 = 0u;
    std::uint32_t gpc_r3 = 0u;
    std::uint32_t gpc_pr = 0u;
    auto gpc_reload = [&]() {
        gpc_r2 = ctx.r[2u];
        gpc_r3 = ctx.r[3u];
        gpc_pr = ctx.pr;
        gpc_active = true;
        if (runtime.perf_profile_enabled) { ++runtime.gpr_cache_entries; runtime.gpr_cache_static_loads += 3u; }
    };
    auto gpc_flush = [&](unsigned gpc_reason) {
        if (!gpc_active) return;
        ctx.r[2u] = gpc_r2;
        gpc_active = false;
        if (runtime.perf_profile_enabled) { runtime.gpr_cache_writebacks += 1u; if (gpc_reason == 1u) ++runtime.gpr_cache_tick_flushes; else if (gpc_reason == 2u) ++runtime.gpr_cache_barrier_flushes; else if (gpc_reason == 3u) ++runtime.gpr_cache_exit_flushes; }
    };
    auto gpc_tick = [&](std::uint64_t gpc_cycles) -> bool {
        ++runtime.sh4_tick_calls; runtime.sh4_tick_pending_cycles += gpc_cycles;
        const std::uint64_t gpc_batch = runtime.sh4_tick_batch_cycles != 0u ? runtime.sh4_tick_batch_cycles : 1u;
        if (runtime.sh4_tick_pending_cycles < gpc_batch) return false;
        gpc_flush(1u);
        const std::uint64_t gpc_accumulated = runtime.sh4_tick_pending_cycles; runtime.sh4_tick_pending_cycles = 0u;
        ++runtime.sh4_tick_full_calls; const bool gpc_stop = dc_runtime_tick_full(ctx, runtime, gpc_accumulated);
        if (!gpc_stop) gpc_reload();
        return gpc_stop;
    };
    gpc_reload();
    switch (ctx.pc) {
        case 0x00008800u: goto BB_8800;
        default: goto BB_8800;
    }
BB_8800:
    ctx.pc = 0x00008800u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (gpc_tick(11u)) return;
    // SH-4 @ 0x00008800u: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x00008802u: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x00008804u: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x00008806u: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x00008808u: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x0000880Au: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x0000880Cu: ADD_REG
    gpc_r2 += gpc_r3;
    // SH-4 @ 0x0000880Eu: ADD_REG
    gpc_r2 += gpc_r3;
    gpc_flush(3u);
    // SH-4 @ 0x00008810u: RETURN
    ctx.pc = ctx.pr;
    return;

    gpc_flush(3u);
    return;
}

void register_generated_gpr_static_cache_emit_probe(DCRuntime& runtime) {
    runtime.register_target(0x00008800u, &recomp_00008800);
}

} // namespace dcrecomp_generated
