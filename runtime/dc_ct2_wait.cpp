#include "dc_runtime.hpp"
#include <cstdlib>
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
}

}
