#include "dcrecomp/raw_data.hpp"

#include <algorithm>
#include <cstdint>

namespace dcrecomp {
namespace {

bool is_direct_dreamcast_area(std::uint32_t value) {
    const std::uint32_t area = value & 0xE0000000u;
    return area == 0x00000000u || area == 0x80000000u || area == 0xA0000000u;
}

bool aliases_raw_image(std::uint32_t base, std::size_t size, std::uint32_t value) {
    if ((value & 1u) != 0u || !is_direct_dreamcast_area(value)) return false;
    const std::uint32_t base_phys = base & 0x1FFFFFFFu;
    const std::uint32_t value_phys = value & 0x1FFFFFFFu;
    if (value_phys < base_phys) return false;
    return static_cast<std::uint64_t>(value_phys - base_phys) + 2u <= size;
}

bool is_main_ram_pointer(std::uint32_t value) {
    if ((value & 3u) != 0u || !is_direct_dreamcast_area(value)) return false;
    const std::uint32_t physical = value & 0x1FFFFFFFu;
    // Dreamcast system RAM occupies physical 0x0C000000..0x0CFFFFFF.
    return physical >= 0x0C000000u && physical < 0x0D000000u;
}

} // namespace

bool looks_like_dense_dreamcast_ram_pointer_words(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base,
    std::uint32_t address,
    std::size_t window_bytes) {
    if (address < base || window_bytes < 16u) return false;
    const std::uint64_t off64 = static_cast<std::uint64_t>(address) - base;
    if (off64 >= bytes.size()) return false;
    const auto off = static_cast<std::size_t>(off64);
    const std::size_t available = std::min(window_bytes, bytes.size() - off);

    std::size_t words = 0u;
    std::size_t pointer_words = 0u;
    for (std::size_t i = 0u; i + 4u <= available; i += 4u) {
        const std::uint32_t value = static_cast<std::uint32_t>(bytes[off + i]) |
                                    (static_cast<std::uint32_t>(bytes[off + i + 1u]) << 8u) |
                                    (static_cast<std::uint32_t>(bytes[off + i + 2u]) << 16u) |
                                    (static_cast<std::uint32_t>(bytes[off + i + 3u]) << 24u);
        ++words;
        if (aliases_raw_image(base, bytes.size(), value) || is_main_ram_pointer(value)) {
            ++pointer_words;
        }
    }

    // At least 4 words and >=75% pointer density. This is intentionally strict:
    // ordinary SH-4 prologues/literal pools can contain one or two addresses, but
    // dense vtables/object tables are overwhelmingly pointer-shaped.
    return words >= 4u && pointer_words >= 4u && pointer_words * 4u >= words * 3u;
}

} // namespace dcrecomp
