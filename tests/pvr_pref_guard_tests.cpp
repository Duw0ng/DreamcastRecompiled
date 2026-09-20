#include <cstdint>
#include <iostream>

namespace {
constexpr std::uint32_t physical29(std::uint32_t a) { return a & 0x1FFFFFFFu; }
constexpr bool generic_ta(std::uint32_t address, std::uint32_t qacr0, std::uint32_t qacr1) {
    if ((address & 0xFC000000u) != 0xE0000000u) return false;
    const std::uint32_t qacr = ((address >> 5u) & 1u) ? qacr1 : qacr0;
    const std::uint32_t target = ((qacr & 0x1Cu) << 24u) | (address & 0x03FFFFE0u);
    const auto p = physical29(target);
    return p >= 0x10000000u && p < 0x10800000u;
}
constexpr bool exact_ta(std::uint32_t address, std::uint32_t qacr0, std::uint32_t qacr1) {
    if ((address & 0xFC000000u) != 0xE0000000u) return false;
    const std::uint32_t qacr = ((address >> 5u) & 1u) ? qacr1 : qacr0;
    const std::uint32_t low = address & 0x03FFFFE0u;
    return (qacr & 0x1Cu) == 0x10u && low < 0x00800000u;
}
[[noreturn]] void fail(std::uint32_t a, std::uint32_t q0, std::uint32_t q1) {
    std::cerr << "TA guard mismatch address=0x" << std::hex << a
              << " q0=0x" << q0 << " q1=0x" << q1 << std::dec << "\n";
    std::exit(1);
}
}

int main() {
    constexpr std::uint32_t boundaries[] = {
        0xE0000000u, 0xE0000020u, 0xE07FFFE0u, 0xE0800000u,
        0xE0FFFFE0u, 0xE1000000u, 0xE3FFFFE0u,
        0xDFFFFFFFu, 0xE4000000u, 0xFFFFFFFFu
    };
    for (std::uint32_t q0 = 0; q0 < 32u; ++q0)
        for (std::uint32_t q1 = 0; q1 < 32u; ++q1)
            for (const auto a : boundaries)
                if (generic_ta(a, q0, q1) != exact_ta(a, q0, q1)) fail(a, q0, q1);

    // Deterministic broad sample over address/QACR combinations.
    std::uint32_t x = 0x13579BDFu;
    for (std::uint32_t n = 0; n < 1000000u; ++n) {
        x ^= x << 13u; x ^= x >> 17u; x ^= x << 5u;
        const std::uint32_t a = (n & 3u) ? (0xE0000000u | (x & 0x03FFFFE0u)) : x;
        const std::uint32_t q0 = (x >> 7u) & 31u;
        const std::uint32_t q1 = (x >> 17u) & 31u;
        if (generic_ta(a, q0, q1) != exact_ta(a, q0, q1)) fail(a, q0, q1);
    }
    std::cout << "PVR native PREF TA guard equivalence: PASS\n";
    return 0;
}
