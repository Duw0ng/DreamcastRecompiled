#pragma once

#include <cstddef>
#include <cstdint>

namespace dcrecomp_generated {

// ARM7TDMI AICA bus: the ARM sees a 24-bit address space. Its 2 MiB
// sound RAM repeats over 0x000000-0x7fffff; registers occupy the other
// 8 MiB and are decoded in a 0x8000-byte mirrored window.
inline constexpr std::uint32_t kAicaArmAddressMask = 0x00FFFFFFu;
inline constexpr std::uint32_t kAicaArmRamLimit = 0x00800000u;
inline constexpr std::uint32_t kAicaArmRamBytes = 0x00200000u;
inline constexpr std::uint32_t kAicaArmRamMask = kAicaArmRamBytes - 1u;
inline constexpr std::uint32_t kAicaArmRegMask = 0x00007FFFu;

inline bool aica_arm_register_offset(std::uint32_t address, std::uint32_t& offset) noexcept {
    const auto mapped = address & kAicaArmAddressMask;
    if (mapped < kAicaArmRamLimit) return false;
    offset = (mapped - kAicaArmRamLimit) & kAicaArmRegMask;
    return true;
}

// Shared RAM accessors for interpreted ARM7 and a future native ARM7 backend.
// Keeping both engines on this path avoids mismatched bus aliasing. This view
// does not intercept writes to executable RAM: the native block cache must
// separately invalidate translated pages on ARM and SH-4 writes.
class AICAArmRamView final {
public:
    AICAArmRamView(std::uint8_t* data, std::size_t bytes) noexcept : data_(data), bytes_(bytes) {}

    bool read8(std::uint32_t address, std::uint8_t& value) const noexcept {
        if (!in_ram(address)) return false;
        value = data_[address & kAicaArmRamMask];
        return true;
    }
    bool read16(std::uint32_t address, std::uint16_t& value) const noexcept {
        if (!in_ram(address)) return false;
        const auto off = address & kAicaArmRamMask;
        value = static_cast<std::uint16_t>(data_[off]) |
                static_cast<std::uint16_t>(data_[(off + 1u) & kAicaArmRamMask]) << 8u;
        return true;
    }
    bool read32(std::uint32_t address, std::uint32_t& value) const noexcept {
        if (!in_ram(address)) return false;
        const auto off = address & kAicaArmRamMask;
        value = static_cast<std::uint32_t>(data_[off]) |
                static_cast<std::uint32_t>(data_[(off + 1u) & kAicaArmRamMask]) << 8u |
                static_cast<std::uint32_t>(data_[(off + 2u) & kAicaArmRamMask]) << 16u |
                static_cast<std::uint32_t>(data_[(off + 3u) & kAicaArmRamMask]) << 24u;
        return true;
    }
    bool write8(std::uint32_t address, std::uint8_t value) noexcept {
        if (!in_ram(address)) return false;
        data_[address & kAicaArmRamMask] = value;
        return true;
    }
    bool write16(std::uint32_t address, std::uint16_t value) noexcept {
        if (!in_ram(address)) return false;
        const auto off = address & kAicaArmRamMask;
        data_[off] = static_cast<std::uint8_t>(value);
        data_[(off + 1u) & kAicaArmRamMask] = static_cast<std::uint8_t>(value >> 8u);
        return true;
    }
    bool write32(std::uint32_t address, std::uint32_t value) noexcept {
        if (!in_ram(address)) return false;
        const auto off = address & kAicaArmRamMask;
        for (unsigned i = 0; i < 4; ++i)
            data_[(off + i) & kAicaArmRamMask] = static_cast<std::uint8_t>(value >> (8u * i));
        return true;
    }

private:
    bool in_ram(std::uint32_t address) const noexcept {
        return data_ != nullptr && bytes_ == kAicaArmRamBytes &&
               (address & kAicaArmAddressMask) < kAicaArmRamLimit;
    }
    std::uint8_t* data_;
    std::size_t bytes_;
};

} // namespace dcrecomp_generated
