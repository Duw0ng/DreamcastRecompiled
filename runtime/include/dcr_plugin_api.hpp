#pragma once

#include <cstdint>

namespace dcrecomp_generated {

struct DCRuntime;

inline constexpr std::uint32_t DCR_PLUGIN_ABI_V1 = 0x00010000u;

enum DCRPluginKindV1 : std::uint32_t {
    DCR_PLUGIN_KIND_UNKNOWN = 0u,
    DCR_PLUGIN_KIND_NETWORK = 1u,
    DCR_PLUGIN_KIND_PVR = 2u,
    DCR_PLUGIN_KIND_AICA = 3u,
    DCR_PLUGIN_KIND_GDROM = 4u,
    DCR_PLUGIN_KIND_MAPLE = 5u,
};

struct DCRPluginDescriptorV1 {
    std::uint32_t abi_version;
    std::uint32_t struct_size;
    std::uint32_t kind;
    std::uint32_t flags;
    const char* name;
    const char* build_id;

    // Lifecycle. State lives inside the plugin; DCRuntime remains ABI-stable.
    bool (*startup)(DCRuntime* runtime);
    void (*shutdown)(DCRuntime* runtime);
    void (*tick)(DCRuntime* runtime);

    // Optional MMIO hooks. Returning false means "not handled" and lets the
    // stable runtime fall back to its built-in compatibility implementation.
    bool (*handles_address)(std::uint32_t address);
    bool (*read8)(DCRuntime* runtime, std::uint32_t address, std::uint8_t* value);
    bool (*write8)(DCRuntime* runtime, std::uint32_t address, std::uint8_t value);
};

using DCRPluginQueryV1 = const DCRPluginDescriptorV1* (*)();

} // namespace dcrecomp_generated

#if defined(_WIN32)
#  if defined(DCR_PLUGIN_BUILD)
#    define DCR_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#  else
#    define DCR_PLUGIN_EXPORT extern "C"
#  endif
#else
#  define DCR_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif
