#pragma once

#include "dc_runtime.hpp"

namespace dcrecomp_generated {

// DreamcastRecomp reachable-program output (version-independent header).
// _main @ 0x8C010000u
void recomp_8C010000(SH4Context& ctx, DCRuntime& runtime);
// _process_record @ 0x8C010080u
void recomp_8C010080(SH4Context& ctx, DCRuntime& runtime);

void register_recompiled_program(DCRuntime& runtime);

} // namespace dcrecomp_generated
