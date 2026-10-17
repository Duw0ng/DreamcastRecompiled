// DCR v1.2 Rev 1: AICA ARM7 address-bus regressions.
// Exercises the exact RAM mapping helpers used by DreamcastRuntime and an
// interpreted ARM7 firmware fragment as the baseline for native recompilation.
#include "dc_arm7.hpp"
#include "dcr_aica_arm_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* detail) {
    if (!condition) {
        std::cerr << "AICA ARM bus test failed: " << detail << '\n';
        std::exit(1);
    }
}

class TestBus final : public dcrecomp::ARM7Bus {
public:
    TestBus()
        : ram_(dcrecomp_generated::kAicaArmRamBytes, 0u),
          view_(ram_.data(), ram_.size()) {}

    std::uint8_t read8(std::uint32_t address) override {
        std::uint8_t value = 0u;
        if (!view_.read8(address, value)) throw std::out_of_range("AICA test bus non-RAM read8");
        return value;
    }
    std::uint16_t read16(std::uint32_t address) override {
        std::uint16_t value = 0u;
        if (!view_.read16(address, value)) throw std::out_of_range("AICA test bus non-RAM read16");
        return value;
    }
    std::uint32_t read32(std::uint32_t address) override {
        std::uint32_t value = 0u;
        if (!view_.read32(address, value)) throw std::out_of_range("AICA test bus non-RAM read32");
        return value;
    }
    void write8(std::uint32_t address, std::uint8_t value) override {
        if (!view_.write8(address, value)) throw std::out_of_range("AICA test bus non-RAM write8");
    }
    void write16(std::uint32_t address, std::uint16_t value) override {
        if (!view_.write16(address, value)) throw std::out_of_range("AICA test bus non-RAM write16");
    }
    void write32(std::uint32_t address, std::uint32_t value) override {
        if (!view_.write32(address, value)) throw std::out_of_range("AICA test bus non-RAM write32");
    }

private:
    std::vector<std::uint8_t> ram_;
    dcrecomp_generated::AICAArmRamView view_;
};

void test_ram_mirrors_and_widths() {
    TestBus bus;
    bus.write32(0x00001000u, 0x12345678u);
    for (auto address : {0x00001000u, 0x00201000u, 0x00401000u, 0x00601000u})
        require(bus.read32(address) == 0x12345678u, "all four 2 MiB RAM mirrors read the same bytes");

    bus.write16(0x00401001u, 0xBEEFu);
    require(bus.read8(0x00001001u) == 0xEFu && bus.read8(0x00601002u) == 0xBEu,
            "unaligned halfword write aliases RAM and retains little-endian order");

    bus.write32(0x001FFFFEu, 0x89ABCDEFu);
    require(bus.read8(0x001FFFFEu) == 0xEFu &&
            bus.read8(0x001FFFFFu) == 0xCDu &&
            bus.read8(0x00200000u) == 0xABu &&
            bus.read8(0x00200001u) == 0x89u,
            "word access wraps across the 2 MiB RAM boundary");
    require(bus.read32(0x005FFFFEu) == 0x89ABCDEFu,
            "cross-boundary word remains readable through another mirror");
}

void test_register_decode() {
    std::uint32_t offset = 0u;
    require(!dcrecomp_generated::aica_arm_register_offset(0x00200000u, offset),
            "0x200000 must decode to mirrored RAM, not MMIO");
    for (auto address : {0x00802000u, 0x0080A000u, 0x00812000u, 0x01802000u})
        require(dcrecomp_generated::aica_arm_register_offset(address, offset) &&
                    offset == 0x2000u,
                "registers mirror within ARM7 24-bit address space");

    std::vector<std::uint8_t> storage(dcrecomp_generated::kAicaArmRamBytes);
    dcrecomp_generated::AICAArmRamView ram(storage.data(), storage.size());
    std::uint8_t value = 0u;
    require(!ram.read8(0x00800000u, value), "register window never aliases sound RAM");
    require(ram.write8(0x01200000u, 0x5Au) && ram.read8(0x00000000u, value) && value == 0x5Au,
            "24-bit address wrap aliases RAM safely");
}

void test_arm7_executes_from_mirror() {
    TestBus bus;
    // LDR r0,[r1]; STR r0,[r1,#4].
    bus.write32(0x00000000u, 0xE5910000u);
    bus.write32(0x00000004u, 0xE5810004u);
    bus.write32(0x00001000u, 0x31415926u);

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx, 0x00200000u);
    ctx.r[1] = 0x00601000u;
    require(dcrecomp::arm7_run(ctx, bus, 2u) == 2u, "ARM7 runs two mirrored firmware instructions");
    require(ctx.r[0] == 0x31415926u, "LDR reads 2 MiB mirror");
    require(bus.read32(0x00001004u) == 0x31415926u, "STR writes mirrored sound RAM");
    require(ctx.r[15] == 0x00200008u, "ARM7 PC advances within the mirror");
}

} // namespace

int main() {
    test_ram_mirrors_and_widths();
    test_register_decode();
    test_arm7_executes_from_mirror();
    std::cout << "DCR v1.2 Rev 1 AICA ARM7 memory regressions passed\n";
}
