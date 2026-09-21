#include "dc_image.hpp"

#include <array>
#include <cstdint>

namespace dcrecomp_generated {
namespace {

// .text @ 0x8C010000u (8 bytes)
static constexpr std::array<std::uint8_t, 8> kSection1 = {
    0xD5, 0xE1, 0x17, 0x60, 0x0B, 0x00, 0x09, 0x00
};

} // namespace

void load_embedded_elf_image(DCRuntime& runtime) {
    dc_load_bytes(runtime, 0x8C010000u, kSection1.data(), kSection1.size()); // .text
}

} // namespace dcrecomp_generated
