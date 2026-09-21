#pragma once

#include "dc_runtime.hpp"

namespace dcrecomp_generated {

void register_native_overrides(DCRuntime& runtime);
void register_maple_host_overrides(DCRuntime& runtime, bool enabled);
void register_probe_overrides(DCRuntime& runtime, bool skip_audio, bool no_input, bool default_video, bool controller_a_then_start, bool controller_start_burst);

} // namespace dcrecomp_generated
