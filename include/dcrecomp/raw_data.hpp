#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dcrecomp {

// Detect a dense table of Dreamcast system-RAM pointers at an exact candidate
// address. P0/P1/P2 aliases of the current raw image count, as do aligned
// pointers into the 16 MiB main-RAM physical window even when the pointed data
// is loaded dynamically and therefore is not present in the static image.
//
// This deliberately does not try to decide whether a short callable thunk sits
// before a pointer pool. Callers that have architectural code evidence should
// apply those exemptions before invoking this data-density predicate.
bool looks_like_dense_dreamcast_ram_pointer_words(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base,
    std::uint32_t address,
    std::size_t window_bytes = 32u);

} // namespace dcrecomp
