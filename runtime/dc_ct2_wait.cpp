#include "dc_runtime.hpp"
#include <cstdlib>
#include <algorithm>
#include <cstdint>
#include <iostream>
namespace dcrecomp_generated {
bool dc_ct2_wait_coalesce_enabled() noexcept {
  const char* v=std::getenv("DCR_CT2_WAIT_COALESCE");
  return v && v[0]=='1' && v[1]==0;
}

void dc_ct2_wait_coalesce(SH4Context& ctx, DCRuntime& runtime, std::uint32_t target) {
    if (!dc_ct2_wait_coalesce_enabled() || !runtime.commercial_boot_active ||
        target != 0x8C1560B0u || ctx.pr != 0x8C157880u ||
        (ctx.sr & 1u) != 0u || runtime.sh4_async_redirect ||
        !runtime.fast_dynamic_dispatch_ready || runtime.trace_calls ||
        runtime.sh4_abi_audit)
        return;
    if (ctx.r[0] != 0x0C300AF8u || ctx.r[1] != 0x0C300C40u ||
        ctx.r[2] != 0x0C1560B0u || ctx.r[3] != 0x0C300B38u)
        return;
    auto read32 = [&](std::uint32_t a) noexcept {
        const std::size_t i = (a & 0x1FFFFFFFu) - DCRuntime::kMainRamPhysicalBase;
        const auto* p = runtime.main_ram.data() + i;
        return static_cast<std::uint32_t>(p[0]) |
               (static_cast<std::uint32_t>(p[1]) << 8u) |
               (static_cast<std::uint32_t>(p[2]) << 16u) |
               (static_cast<std::uint32_t>(p[3]) << 24u);
    };
    if (read32(0x0C300B38u) != 0x0C1560B0u ||
        ctx.r[4] != read32(0x0C300B3Cu) ||
        read32(0x0C300C44u) == 0u)
        return;
    const std::uint32_t bound =
        read32(0x0C300AF4u) + read32(0x0C300AF8u) + 1u;
    if (read32(0x0C300C40u) > bound) return;
    constexpr std::uint64_t kIterationCycles = 22u;
    const std::uint64_t quantum = runtime.sh4_tick_batch_cycles != 0u
        ? std::min<std::uint64_t>(runtime.sh4_tick_batch_cycles, 256u) : 1u;
    const std::uint64_t pending = runtime.sh4_tick_pending_cycles;
    if (pending >= quantum || quantum - pending <= kIterationCycles + 1u) return;
    const std::uint64_t count = (quantum - pending - 1u) / kIterationCycles;
    if (count == 0u) return;
    runtime.sh4_tick_pending_cycles += count * kIterationCycles;
#if !defined(DCR_DISABLE_HOT_TICK_CALL_COUNTER)
    runtime.sh4_tick_calls += count * 4u;
#endif
    static std::uint64_t batches = 0u;
    static std::uint64_t skipped = 0u;
    ++batches;
    skipped += count;
    if ((batches & (batches - 1u)) == 0u)
        std::cerr << "[DCR CT2 WAIT] batches=" << batches
                  << " skipped-iterations=" << skipped
                  << " cycles=" << skipped * kIterationCycles
                  << " | scheduler-boundary=256\n";
}

}
