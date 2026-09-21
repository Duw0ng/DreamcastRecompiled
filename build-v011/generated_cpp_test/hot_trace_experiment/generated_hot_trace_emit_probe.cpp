#include "generated_hot_trace_emit_probe.hpp"

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

void recomp_00009000(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    // 0.0.156 adaptive static hot trace: profile once, then elide
    // block-entry PC stores until a real scheduler/external barrier.
    static std::uint32_t htr_profile_count = 0u;
    static bool htr_promoted = false;
    bool htr_block = false;
    std::uint64_t htr_stat_entries=0, htr_stat_elisions=0, htr_stat_syncs=0, htr_stat_full=0;
    auto htr_flush_stats = [&]() {
        runtime.hot_trace_entries += htr_stat_entries; runtime.hot_trace_pc_elisions += htr_stat_elisions;
        runtime.hot_trace_syncs += htr_stat_syncs; runtime.hot_trace_full_ticks += htr_stat_full;
        htr_stat_entries=htr_stat_elisions=htr_stat_syncs=htr_stat_full=0u;
    };
    auto htr_sync = [&](std::uint32_t htr_pc) { ctx.pc = htr_pc; runtime.current_pc = htr_pc; ++htr_stat_syncs; };
    auto htr_tick = [&](std::uint64_t htr_cycles, std::uint32_t htr_pc) -> bool {
        ++runtime.sh4_tick_calls; runtime.sh4_tick_pending_cycles += htr_cycles;
        const std::uint64_t htr_batch = runtime.sh4_tick_batch_cycles != 0u ? runtime.sh4_tick_batch_cycles : 1u;
        if (runtime.sh4_tick_pending_cycles < htr_batch) { ++htr_stat_elisions; return false; }
        htr_sync(htr_pc); const std::uint64_t htr_accumulated = runtime.sh4_tick_pending_cycles; runtime.sh4_tick_pending_cycles = 0u;
        ++runtime.sh4_tick_full_calls; ++htr_stat_full; htr_flush_stats();
        return dc_runtime_tick_full(ctx, runtime, htr_accumulated);
    };
    switch (ctx.pc) {
        case 0x00009000u: goto BB_9000;
        default: goto BB_9000;
    }
BB_9000:
    if (!htr_promoted && runtime.hot_trace_enabled) { if (++htr_profile_count >= runtime.hot_trace_threshold) { htr_promoted = true; ++runtime.hot_trace_promotions; } }
    htr_block = runtime.hot_trace_enabled && htr_promoted;
    if (htr_block) { ++htr_stat_entries;
        if (htr_tick(3u, 0x00009000u)) return;
    } else {
        ctx.pc = 0x00009000u; DCR_SYNC_CURRENT_PC(runtime, ctx);
        if (dc_runtime_tick_fast(ctx, runtime, 3u)) return;
    }
    // SH-4 @ 0x00009000u: ADD_IMM
    ctx.r[2] += static_cast<std::uint32_t>(static_cast<std::int32_t>(1));
    // SH-4 @ 0x00009002u: BRANCH
    goto BB_9000;

    htr_flush_stats();
    return;
}

void register_generated_hot_trace_emit_probe(DCRuntime& runtime) {
    runtime.register_target(0x00009000u, &recomp_00009000);
    runtime.hot_trace_candidates += 1u;
}

} // namespace dcrecomp_generated
