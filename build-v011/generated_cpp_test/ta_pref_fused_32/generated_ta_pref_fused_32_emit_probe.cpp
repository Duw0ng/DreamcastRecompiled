#include "generated_ta_pref_fused_32_emit_probe.hpp"

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

void recomp_00008000(SH4Context& ctx, DCRuntime& runtime) {
    bool delayed_t = false;
    std::uint32_t delayed_target = 0u;
    std::array<SQTAFusedCapture, 8u> sqf_0{};
    bool sqf_0_ok = true;
    switch (ctx.pc) {
        case 0x00008000u: goto BB_8000;
        default: goto BB_8000;
    }
BB_8000:
    sqf_0_ok = true;
    ctx.pc = 0x00008000u;
    DCR_SYNC_CURRENT_PC(runtime, ctx);
    if (dc_runtime_tick_fast(ctx, runtime, 9u)) return;
    // SH-4 @ 0x00008000u: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 0u; const std::uint32_t sqf_value = ctx.r[2];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[0u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x00008002u: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 4u; const std::uint32_t sqf_value = ctx.r[3];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[1u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 1u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x00008004u: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 8u; const std::uint32_t sqf_value = ctx.r[4];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[2u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 2u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x00008006u: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 12u; const std::uint32_t sqf_value = ctx.r[5];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[3u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 3u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x00008008u: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 16u; const std::uint32_t sqf_value = ctx.r[2];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[4u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 4u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x0000800Au: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 20u; const std::uint32_t sqf_value = ctx.r[3];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[5u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 5u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x0000800Cu: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 24u; const std::uint32_t sqf_value = ctx.r[4];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[6u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 6u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x0000800Eu: STORE32_DISP [SQ->TA fused capture]
    { const std::uint32_t sqf_address = ctx.r[4] + 28u; const std::uint32_t sqf_value = ctx.r[5];
        if (sqf_0_ok && (sqf_address & 0xFC000000u) == 0xE0000000u && (sqf_address & 31u) <= 28u) {
            sqf_0[7u] = SQTAFusedCapture{sqf_address, sqf_value, 4u};
        } else {
            if (sqf_0_ok) {
                dc_sq_ta_replay_captures(runtime, sqf_0.data(), 7u);
                sqf_0_ok = false;
            }
            dc_guest_write32_hot(runtime, sqf_address, sqf_value);
        } }
    // SH-4 @ 0x00008010u: PREF [SQ->TA complete packet fusion]
    if (sqf_0_ok) dc_pref_ta_fused_packet(runtime, ctx.r[4], 0x00008010u, sqf_0.data(), 8u);
    else dc_pref_ta_native(runtime, ctx.r[4], 0x00008010u);
    sqf_0_ok = true;

    return;
}

void register_generated_ta_pref_fused_32_emit_probe(DCRuntime& runtime) {
    runtime.register_target(0x00008000u, &recomp_00008000);
    runtime.sq_ta_fusion_sites += 1u;
}

} // namespace dcrecomp_generated
