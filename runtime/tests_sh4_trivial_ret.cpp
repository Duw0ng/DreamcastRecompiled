// DCR V1.2: precompiled ABI1 return-only fast-path differential regression.
// No game data is used. Test both DCR_SH4_RETFAST=0 and =1 via CTest.
#include "dc_runtime.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace dcrecomp_generated;
namespace {
unsigned original_calls = 0u;
void expect(bool ok, const char* msg) { if (!ok) throw std::runtime_error(msg); }
void original_aot_leaf(SH4Context& ctx, DCRuntime& runtime) {
    ++original_calls;
    if (dc_runtime_tick_fast(ctx, runtime, 3u)) return;
    ctx.pc = ctx.pr;
}
}
int main() {
    try {
        const char* value = std::getenv("DCR_SH4_RETFAST");
        const bool on = value && value[0] == '1' && value[1] == '\0';
        constexpr std::uint32_t kTarget = 0x8C1560B0u;
        constexpr std::uint32_t kReturnPc = 0x8C157880u;
        constexpr std::size_t kOffset =
            (kTarget & 0x1FFFFFFFu) - DCRuntime::kMainRamPhysicalBase;
        DCRuntime runtime;
        runtime.commercial_boot_active = true;
        runtime.sh4_tick_batch_cycles = 0x100000u;
        runtime.targets[kTarget] = &original_aot_leaf;
        // Exact SH-4 opcodes: RTS 000B, NOP 0009.
        runtime.main_ram[kOffset] = 0x0Bu;
        runtime.main_ram[kOffset + 1] = 0x00u;
        runtime.main_ram[kOffset + 2] = 0x09u;
        runtime.main_ram[kOffset + 3] = 0x00u;

        SH4Context ctx{};
        ctx.pc = 0x8C157878u;
        ctx.pr = kReturnPc;
        call_recompiled(ctx, runtime, kTarget); // install cache entry
        const auto& entry = runtime.dispatch_cache[
            (kTarget >> 1u) & (DCRuntime::kDispatchCacheSize - 1u)];
        expect(ctx.pc == kReturnPc, "first return PC changed");
        expect(runtime.sh4_tick_pending_cycles == 3u, "first return lost cycles");
        expect(original_calls == (on ? 0u : 1u), "first dispatch wrong implementation");
        expect(entry.function != nullptr, "cache entry not installed");
        expect((entry.function != &original_aot_leaf) == on, "fastpath toggle not respected");

        // Now use the same inline dispatcher as the original precompiled guest.
        runtime.fast_dynamic_dispatch_ready = true;
        ctx.pc = 0x8C157878u;
        dc_call_dynamic_fast(ctx, runtime, kTarget);
        expect(ctx.pc == kReturnPc, "inline return PC changed");
        expect(runtime.sh4_tick_pending_cycles == 6u, "inline return lost cycles");
        expect(original_calls == (on ? 0u : 2u), "inline dispatch wrong implementation");

        // A code overwrite must revoke the experimental shortcut. The original
        // AOT function remains callable and receives exactly the same arguments.
        runtime.main_ram[kOffset + 2] = 0x08u;
        ctx.pc = 0x8C157878u;
        dc_call_dynamic_fast(ctx, runtime, kTarget);
        expect(ctx.pc == kReturnPc, "overwrite return PC changed");
        expect(runtime.sh4_tick_pending_cycles == 9u, "overwrite return lost cycles");
        expect(original_calls == (on ? 1u : 3u), "code overwrite did not fall back");
        expect(entry.function == &original_aot_leaf, "overwritten code kept cached shortcut");

        std::cout << "[PASS] SH4 trivial-return fastpath on=" << on
                  << " cycles=" << runtime.sh4_tick_pending_cycles
                  << " original-calls=" << original_calls << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << e.what() << "\n";
        return 1;
    }
}
