#include "dcrecomp/raw_data.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void write32(std::vector<std::uint8_t>& bytes, std::size_t off, std::uint32_t v) {
    bytes[off + 0u] = static_cast<std::uint8_t>(v & 0xFFu);
    bytes[off + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xFFu);
    bytes[off + 2u] = static_cast<std::uint8_t>((v >> 16u) & 0xFFu);
    bytes[off + 3u] = static_cast<std::uint8_t>((v >> 24u) & 0xFFu);
}

bool expect(bool cond, const char* what) {
    if (!cond) std::cerr << "[FAIL] " << what << "\n";
    return cond;
}

} // namespace

int main() {
    constexpr std::uint32_t base = 0x8C008000u;
    std::vector<std::uint8_t> bytes(0x4000u, 0u);
    bool ok = true;

    // P0 aliases of addresses inside the raw image. The old detector missed
    // these because it compared only exact 0x8C... virtual addresses.
    for (std::size_t i = 0u; i < 8u; ++i)
        write32(bytes, 0x100u + i * 4u, 0x0C008400u + static_cast<std::uint32_t>(i * 0x20u));
    ok &= expect(dcrecomp::looks_like_dense_dreamcast_ram_pointer_words(bytes, base, base + 0x100u),
                 "P0 alias pointer table must be classified as data");

    // P2 aliases of the same image are equally valid Dreamcast pointers.
    for (std::size_t i = 0u; i < 8u; ++i)
        write32(bytes, 0x180u + i * 4u, 0xAC008400u + static_cast<std::uint32_t>(i * 0x20u));
    ok &= expect(dcrecomp::looks_like_dense_dreamcast_ram_pointer_words(bytes, base, base + 0x180u),
                 "P2 alias pointer table must be classified as data");

    // Runtime-loaded/dynamic objects can live elsewhere in the 16 MiB main RAM
    // and are not aliases of bytes present in the static raw image.
    for (std::size_t i = 0u; i < 8u; ++i)
        write32(bytes, 0x200u + i * 4u, 0x0C420000u + static_cast<std::uint32_t>(i * 0x100u));
    ok &= expect(dcrecomp::looks_like_dense_dreamcast_ram_pointer_words(bytes, base, base + 0x200u),
                 "external main-RAM pointer table must be classified as data");

    // A mixed block with only two pointer-looking words is not dense enough.
    for (std::size_t i = 0u; i < 8u; ++i)
        write32(bytes, 0x280u + i * 4u, 0x12340000u + static_cast<std::uint32_t>(i * 0x111u));
    write32(bytes, 0x280u, 0x0C420000u);
    write32(bytes, 0x284u, 0x8C008800u);
    ok &= expect(!dcrecomp::looks_like_dense_dreamcast_ram_pointer_words(bytes, base, base + 0x280u),
                 "sparse pointers must not classify ordinary bytes as a table");

    if (!ok) return 1;
    std::cout << "raw_data_tests: PASS\n";
    return 0;
}
