// DCR v1.2 Rev 2: SGGG audio retrigger regression.
// A one-shot AICA slot that reaches LEA must clear KYONB; otherwise a
// KYONEX command for any *other* slot restarts the old sound.
#include "dc_runtime.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace dcrecomp_generated;

namespace {
void check(bool ok, const char* description) {
    if (!ok) throw std::runtime_error(description);
}
void set_slot(DCRuntime& rt, unsigned ch, std::uint32_t base, std::uint32_t lea) {
    constexpr std::uint32_t kAica = 0xA0700000u; // SH-4 P2 alias of 0x00700000
    const std::uint32_t addr = kAica + ch * 0x80u;
    dc_write32(rt, addr + 4u, base);
    dc_write32(rt, addr + 12u, lea);
}
void test_one_shot_does_not_restart_on_other_slots_keyonex() {
    auto rt = std::make_unique<DCRuntime>();
    dc_aica_enable_arm7(*rt, true);
    rt->aica_arm7_in_reset = false;
    rt->aica_native_arm_steps_per_frame = 1u;
    rt->aica_arm7_idle_fast_forward = false;

    // ARM7 NOPs let the existing public ARM execution API advance the native
    // AICA mix path deterministically without adding debug-only entrypoints.
    for (unsigned i = 0; i < 1024; i += 4) {
        rt->aica_ram[i] = 0x00u;
        rt->aica_ram[i+1] = 0x00u;
        rt->aica_ram[i+2] = 0xA0u;
        rt->aica_ram[i+3] = 0xE1u; // ARM MOV r0,r0
    }

    constexpr std::uint32_t kAica = 0xA0700000u;
    constexpr std::uint32_t kOneShotPCM8 = 0xC080u; // KYONEX | KYONB | PCMS=1
    set_slot(*rt, 0u, 0x100u, 4u);
    dc_write32(*rt, kAica, kOneShotPCM8);
    check(rt->aica_channel_starts == 1u, "first KYONEX should start voice #0 exactly once");
    check(rt->aica_native_slots[0].playing, "slot #0 must be playing initially");

    const auto executed = dc_aica_run_arm7(*rt, 16u);
    check(executed == 16u, "ARM7 test firmware must execute 16 NOP instructions");
    check(rt->aica_native_frames >= 5u, "native mixer must advance to one-shot end");
    check(!rt->aica_native_slots[0].playing, "one-shot at LEA must stop");
    check((rt->aica_native_slots[0].reg0 & 0x4000u) == 0u,
          "one-shot end must clear hardware KYONB");
    check((dc_read32(*rt, kAica) & 0x4000u) == 0u,
          "ARM/SH-4 visible register shadow must also clear KYONB");

    // Starting a DIFFERENT sound asserts global KYONEX. Slot #0 must not be
    // rearmed just because it was programmed with KYONB in the past.
    set_slot(*rt, 1u, 0x200u, 4u);
    dc_write32(*rt, kAica + 0x80u, kOneShotPCM8);
    check(rt->aica_native_slots[1].playing, "slot #1 must start normally");
    check(rt->aica_native_slots[0].starts == 1u, "old one-shot must not restart");
    check(rt->aica_channel_starts == 2u, "total starts must only include the two intended voices");

    // A deliberate explicit restart of slot #0 still works.
    dc_write32(*rt, kAica, kOneShotPCM8);
    check(rt->aica_native_slots[0].starts == 2u, "explicit KYONB+KYONEX can start finished slot");
    check(rt->aica_channel_starts == 3u, "explicit replay counted once");
}
} // namespace

int main() {
    try {
        test_one_shot_does_not_restart_on_other_slots_keyonex();
        std::cout << "DCR v1.2 Rev 2: SGGG AICA one-shot KYONB regression PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "AICA one-shot regression FAIL: " << e.what() << "\n";
        return 1;
    }
}
