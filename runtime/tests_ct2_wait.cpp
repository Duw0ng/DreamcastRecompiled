// Regression of the CT2 runtime-only scheduler-bound wait coalescer.
#include "dc_runtime.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace dcrecomp_generated;
namespace {
int original_calls = 0;
void orig(SH4Context& ctx, DCRuntime& rt) {
    ++original_calls;
    if (dc_runtime_tick_fast(ctx, rt, 3u)) return;
    ctx.pc = ctx.pr;
}
void put32(DCRuntime& rt, std::uint32_t a, std::uint32_t v) {
    const std::size_t i = (a & 0x1FFFFFFFu) - DCRuntime::kMainRamPhysicalBase;
    for (unsigned j=0; j<4; ++j) rt.main_ram[i+j] = static_cast<std::uint8_t>(v >> (j*8u));
}
void must(bool ok, const char* m) { if (!ok) throw std::runtime_error(m); }
}
int main() {
 try {
    const char* opt=std::getenv("DCR_CT2_WAIT_COALESCE");
    const bool enabled=opt && opt[0]=='1' && opt[1]=='\0';
    DCRuntime rt{};
    rt.commercial_boot_active=true;
    rt.fast_dynamic_dispatch_ready=true;
    rt.sh4_tick_batch_cycles=256u;
    constexpr auto target=0x8C1560B0u;
    rt.targets[target]=&orig;
    put32(rt,target,0x0009000Bu);
    put32(rt,0x0C300B38u,0x0C1560B0u);
    put32(rt,0x0C300B3Cu,0x12345678u);
    put32(rt,0x0C300AF4u,0u);
    put32(rt,0x0C300AF8u,0u);
    put32(rt,0x0C300C40u,0u);
    put32(rt,0x0C300C44u,1u);
    SH4Context ctx{};
    ctx.pc=0x8C157878u;
    ctx.pr=0x8C157880u;
    ctx.r[0]=0x0C300AF8u;
    ctx.r[1]=0x0C300C40u;
    ctx.r[2]=0x0C1560B0u;
    ctx.r[3]=0x0C300B38u;
    ctx.r[4]=0x12345678u;
    rt.sh4_tick_pending_cycles=20u;
    call_recompiled(ctx,rt,target);
    const std::uint64_t want=enabled ? 243u : 23u;
    must(ctx.pc==0x8C157880u,"callback PR/PC changed");
    must(rt.sh4_tick_pending_cycles==want,"incorrect event-boundary cycle accounting");
    must(original_calls==(enabled ? 0 : 1),"unexpected callback implementation");
    // Transitioning the guard counter must prevent any further coalescing.
    put32(rt,0x0C300C44u,0u);
    rt.sh4_tick_pending_cycles=20u;
    ctx.pc=0x8C157878u;
    call_recompiled(ctx,rt,target);
    must(rt.sh4_tick_pending_cycles==23u,"coalesced after wait condition changed");
    must(ctx.r[0]==0x0C300AF8u && ctx.r[1]==0x0C300C40u &&
         ctx.r[2]==0x0C1560B0u && ctx.r[3]==0x0C300B38u,
         "coalescer modified guest general registers");
    std::cout << "[PASS] CT2 wait coalesce on=" << enabled << "\n";
    return 0;
 } catch (const std::exception& e) {
    std::cerr << "[FAIL] " << e.what() << "\n";
    return 1;
 }
}
