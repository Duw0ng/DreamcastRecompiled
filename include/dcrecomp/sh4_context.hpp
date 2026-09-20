#pragma once

#include <array>
#include <cstdint>

namespace dcrecomp {

// Architectural SH-4 state shared by analysis/codegen. The generated 0.0.27 runtime
// emits an equivalent standalone definition so generated projects do not depend on
// the DreamcastRecomp source tree.
struct SH4Context {
    std::array<std::uint32_t, 16> r{};

    std::uint32_t pc{};
    std::uint32_t pr{};
    std::uint32_t gbr{};
    std::uint32_t vbr{};
    std::uint32_t ssr{};
    std::uint32_t spc{};
    std::uint32_t sgr{};
    std::uint32_t dbr{};
    std::array<std::uint32_t, 8> r_bank{};
    std::uint32_t mach{};
    std::uint32_t macl{};
    std::uint32_t sr{};

    std::uint32_t fpul{};
    std::uint32_t fpscr{};
    std::array<std::uint32_t, 16> fr_bits{};
    std::array<std::uint32_t, 16> xf_bits{};
};

} // namespace dcrecomp
