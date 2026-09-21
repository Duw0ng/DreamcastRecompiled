#pragma once

#include "dc_runtime.hpp"

namespace dcrecomp_generated {

// Loads every SHF_ALLOC section that maps to Dreamcast main RAM, including .text.
// Recompiled instructions execute as native C++, but real SH-4 binaries frequently
// keep literal pools, jump tables and floating-point constants inside executable
// sections and access those bytes as normal data at runtime.
void load_embedded_elf_image(DCRuntime& runtime);

} // namespace dcrecomp_generated
