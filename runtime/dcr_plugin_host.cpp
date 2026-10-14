#include "dc_runtime.hpp"
#include "dcr_plugin_api.hpp"
#include "dcr_plugin_api_v2.hpp"
#include "dcr_gdrom_bridge.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace dcrecomp_generated {
namespace {

struct LoadedPlugin {
#ifdef _WIN32
    HMODULE module{};
#else
    void* module{};
#endif
    const DCRPluginDescriptorV1* api1{};
    const DCRPluginDescriptorV2* api2{};
    void* instance{};
    std::filesystem::path path;
    std::uint32_t kind{};
};

struct Registry {
    std::vector<LoadedPlugin> plugins;
    DCRHostServicesV2 host{};
    bool attempted{};
};

std::mutex g_plugin_mutex;
std::unordered_map<DCRuntime*, Registry> g_plugins;
std::atomic<std::uint64_t> g_plugin_generation{1u};

constexpr std::size_t kFastKinds = 16u;
struct CachedPlugin {
    const DCRPluginDescriptorV1* api1{};
    const DCRPluginDescriptorV2* api2{};
    void* instance{};
};
struct PluginThreadCache {
    DCRuntime* runtime{};
    std::uint64_t generation{};
    std::array<CachedPlugin, kFastKinds> kinds{};
};
thread_local PluginThreadCache g_plugin_tls;

std::atomic<std::uint64_t> g_gdrom_audio_generation{1u};
struct GDROMAudioThreadCache {
    DCRuntime* runtime{};
    std::uint64_t generation{};
    std::array<std::int16_t, 64u * 2u> stereo{};
    std::uint32_t frames{};
    std::uint32_t pos{};
};
thread_local GDROMAudioThreadCache g_gdrom_audio_tls;

void invalidate_gdrom_audio_cache(DCRuntime& runtime) {
    (void)runtime;
    g_gdrom_audio_generation.fetch_add(1u, std::memory_order_release);
}

bool is_plugin_library(const std::filesystem::path& p) {
#ifdef _WIN32
    return p.extension() == ".dll";
#elif defined(__APPLE__)
    return p.extension() == ".dylib";
#else
    return p.extension() == ".so";
#endif
}

void* lookup_symbol(
#ifdef _WIN32
    HMODULE module,
#else
    void* module,
#endif
    const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(module, name));
#else
    return dlsym(module, name);
#endif
}

#ifdef _WIN32
HMODULE open_library(const std::filesystem::path& p) { return LoadLibraryW(p.wstring().c_str()); }
void close_library(HMODULE h) { if (h) FreeLibrary(h); }
#else
void* open_library(const std::filesystem::path& p) { return dlopen(p.string().c_str(), RTLD_NOW | RTLD_LOCAL); }
void close_library(void* h) { if (h) dlclose(h); }
#endif

std::filesystem::path default_plugin_dir() {
    if (const char* env = std::getenv("DCR_PLUGIN_DIR"); env && *env) return std::filesystem::path(env);
    return std::filesystem::path("plugins");
}

LoadedPlugin* find_plugin_locked(Registry& r, std::uint32_t kind) {
    for (auto& p : r.plugins) if (p.kind == kind) return &p;
    return nullptr;
}

CachedPlugin find_plugin_fast(DCRuntime& runtime, std::uint32_t kind) {
    const auto generation = g_plugin_generation.load(std::memory_order_acquire);
    if (kind < kFastKinds && g_plugin_tls.runtime == &runtime && g_plugin_tls.generation == generation)
        return g_plugin_tls.kinds[kind];

    std::lock_guard<std::mutex> lock(g_plugin_mutex);
    const auto it = g_plugins.find(&runtime);
    if (it == g_plugins.end()) return {};
    if (kind < kFastKinds) {
        g_plugin_tls = {};
        g_plugin_tls.runtime = &runtime;
        g_plugin_tls.generation = generation;
        for (const auto& p : it->second.plugins) {
            if (p.kind < kFastKinds) g_plugin_tls.kinds[p.kind] = {p.api1, p.api2, p.instance};
        }
        return g_plugin_tls.kinds[kind];
    }
    if (auto* p = find_plugin_locked(it->second, kind)) return {p->api1, p->api2, p->instance};
    return {};
}

DCRuntime& host_runtime(void* ctx) { return *static_cast<DCRuntime*>(ctx); }
std::uint8_t hs_read8(void* c, std::uint32_t a) { return dc_read8(host_runtime(c), a); }
std::uint16_t hs_read16(void* c, std::uint32_t a) { return dc_read16(host_runtime(c), a); }
std::uint32_t hs_read32(void* c, std::uint32_t a) { return dc_read32(host_runtime(c), a); }
void hs_write8(void* c, std::uint32_t a, std::uint8_t v) { dc_write8(host_runtime(c), a, v); }
void hs_write16(void* c, std::uint32_t a, std::uint16_t v) { dc_write16(host_runtime(c), a, v); }
void hs_write32(void* c, std::uint32_t a, std::uint32_t v) { dc_write32(host_runtime(c), a, v); }

bool hs_get_memory_region(void* c, std::uint32_t address, std::size_t size, DCRMemoryRegionV2* out) {
    if (!out) return false;
    auto& r = host_runtime(c);
    const std::uint32_t p = address & 0x1FFFFFFFu;
    auto match = [&](std::uint32_t base, std::vector<std::uint8_t>& mem, std::uint32_t flags) {
        if (p < base) return false;
        const std::size_t off = static_cast<std::size_t>(p - base);
        if (off > mem.size() || size > mem.size() - off) return false;
        *out = {mem.data() + off, mem.size() - off, base, flags};
        return true;
    };
    if (match(DCRuntime::kMainRamPhysicalBase, r.main_ram, DCR_MEM_READ | DCR_MEM_WRITE | DCR_MEM_EXEC)) return true;
    if (match(0x04000000u, r.vram, DCR_MEM_READ | DCR_MEM_WRITE)) return true;
    if (match(0x00800000u, r.aica_ram, DCR_MEM_READ | DCR_MEM_WRITE)) return true;
    return false;
}
void hs_raise_irq(void* c, std::uint32_t bank, std::uint32_t bits) {
    auto& r = host_runtime(c); if (bank < r.holly_pending.size()) r.holly_pending[bank] |= bits;
}
void hs_clear_irq(void* c, std::uint32_t bank, std::uint32_t bits) {
    auto& r = host_runtime(c); if (bank < r.holly_pending.size()) r.holly_pending[bank] &= ~bits;
}
bool hs_schedule_event(void*, std::uint64_t, std::uint32_t, std::uint64_t) { return false; }
void hs_log(void*, std::uint32_t, const char* component, const char* message) {
    std::cout << "[PLUGIN V2" << (component && *component ? std::string(" ") + component : std::string()) << "] "
              << (message ? message : "") << "\n";
}
const char* hs_get_config(void*, const char* key) { return key ? std::getenv(key) : nullptr; }
bool hs_compat_read32(void* c, std::uint32_t kind, std::uint32_t address, std::uint32_t* value) {
    if (!value) return false;
    if (kind == DCR_PLUGIN_KIND_MAPLE) return dc_builtin_maple_read32(host_runtime(c), address, *value);
    return false;
}
bool hs_compat_write32(void* c, std::uint32_t kind, std::uint32_t address, std::uint32_t value) {
    if (kind == DCR_PLUGIN_KIND_MAPLE) return dc_builtin_maple_write32(host_runtime(c), address, value);
    return false;
}

DCRHostServicesV2 make_host(DCRuntime& runtime) {
    return {DCR_HOST_SERVICES_ABI_V2, sizeof(DCRHostServicesV2), &runtime,
            &hs_read8, &hs_read16, &hs_read32, &hs_write8, &hs_write16, &hs_write32,
            &hs_get_memory_region, &hs_raise_irq, &hs_clear_irq, &hs_schedule_event,
            &hs_log, &hs_get_config, &hs_compat_read32, &hs_compat_write32};
}

} // namespace

bool dc_plugins_load_default(DCRuntime& runtime, const std::string& requested_dir) {
    std::lock_guard<std::mutex> lock(g_plugin_mutex);
    auto& reg = g_plugins[&runtime];
    if (reg.attempted) return !reg.plugins.empty();
    reg.attempted = true;
    reg.host = make_host(runtime);

    const std::filesystem::path dir = requested_dir.empty() ? default_plugin_dir() : std::filesystem::path(requested_dir);
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
        std::cout << "[PLUGIN] directory not found: " << dir.string() << " | using built-in fallbacks\n";
        return false;
    }

    std::vector<std::filesystem::path> candidates;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        const auto& p = e.path();
        if (!is_plugin_library(p) || p.stem().string().rfind("dcr_", 0u) != 0u) continue;
        candidates.push_back(p);
    }
    std::sort(candidates.begin(), candidates.end());

    for (const auto& path : candidates) {
        auto module = open_library(path);
        if (!module) { std::cout << "[PLUGIN] failed to load " << path.filename().string() << "\n"; continue; }

        if (auto* raw2 = lookup_symbol(module, "dcr_plugin_query_v2")) {
            auto query2 = reinterpret_cast<DCRPluginQueryV2>(raw2);
            const auto* api2 = query2 ? query2() : nullptr;
            if (api2 && api2->abi_version == DCR_PLUGIN_ABI_V2 && api2->struct_size >= sizeof(DCRPluginDescriptorV2)) {
                if (find_plugin_locked(reg, api2->kind)) {
                    std::cout << "[PLUGIN] duplicate kind ignored: " << (api2->name ? api2->name : path.filename().string()) << "\n";
                    close_library(module); continue;
                }
                void* instance = api2->create ? api2->create(&reg.host) : nullptr;
                if (api2->create && !instance) {
                    std::cout << "[PLUGIN] V2 create rejected: " << (api2->name ? api2->name : path.filename().string()) << "\n";
                    close_library(module); continue;
                }
                reg.plugins.push_back(LoadedPlugin{module, nullptr, api2, instance, path, api2->kind});
                std::cout << "[PLUGIN] loaded ABI2 kind=" << api2->kind << " name=" << (api2->name ? api2->name : "?")
                          << " build=" << (api2->build_id ? api2->build_id : "?") << " file=" << path.filename().string() << "\n";
                continue;
            }
        }

        auto* raw1 = lookup_symbol(module, "dcr_plugin_query_v1");
        if (!raw1) { close_library(module); continue; }
        auto query1 = reinterpret_cast<DCRPluginQueryV1>(raw1);
        const auto* api1 = query1 ? query1() : nullptr;
        if (!api1 || api1->abi_version != DCR_PLUGIN_ABI_V1 || api1->struct_size < sizeof(DCRPluginDescriptorV1)) {
            std::cout << "[PLUGIN] ABI mismatch: " << path.filename().string() << "\n"; close_library(module); continue;
        }
        if (find_plugin_locked(reg, api1->kind)) {
            std::cout << "[PLUGIN] duplicate kind ignored: " << (api1->name ? api1->name : path.filename().string()) << "\n";
            close_library(module); continue;
        }
        if (api1->startup && !api1->startup(&runtime)) {
            std::cout << "[PLUGIN] startup rejected: " << (api1->name ? api1->name : path.filename().string()) << "\n";
            close_library(module); continue;
        }
        reg.plugins.push_back(LoadedPlugin{module, api1, nullptr, nullptr, path, api1->kind});
        std::cout << "[PLUGIN] loaded ABI1 kind=" << api1->kind << " name=" << (api1->name ? api1->name : "?")
                  << " build=" << (api1->build_id ? api1->build_id : "?") << " file=" << path.filename().string() << "\n";
    }
    g_plugin_generation.fetch_add(1u, std::memory_order_release);
    return !reg.plugins.empty();
}

void dc_plugins_shutdown(DCRuntime& runtime) {
    std::lock_guard<std::mutex> lock(g_plugin_mutex);
    const auto it = g_plugins.find(&runtime); if (it == g_plugins.end()) return;
    for (auto rit = it->second.plugins.rbegin(); rit != it->second.plugins.rend(); ++rit) {
        if (rit->api2 && rit->api2->destroy) rit->api2->destroy(rit->instance);
        if (rit->api1 && rit->api1->shutdown) rit->api1->shutdown(&runtime);
        close_library(rit->module);
    }
    g_plugins.erase(it);
    g_plugin_generation.fetch_add(1u, std::memory_order_release);
    if (g_plugin_tls.runtime == &runtime) g_plugin_tls = {};
}

bool dc_plugin_kind_loaded(DCRuntime& runtime, std::uint32_t kind) { auto p = find_plugin_fast(runtime, kind); return p.api1 || p.api2; }

bool dc_plugin_read8(DCRuntime& runtime, std::uint32_t kind, std::uint32_t address, std::uint8_t& value) {
    auto p = find_plugin_fast(runtime, kind);
    if (p.api2) { if (p.api2->handles_address && !p.api2->handles_address(p.instance, address)) return false; return p.api2->read8 && p.api2->read8(p.instance, address, &value); }
    if (p.api1) { if (p.api1->handles_address && !p.api1->handles_address(address)) return false; return p.api1->read8 && p.api1->read8(&runtime, address, &value); }
    return false;
}
bool dc_plugin_write8(DCRuntime& runtime, std::uint32_t kind, std::uint32_t address, std::uint8_t value) {
    auto p = find_plugin_fast(runtime, kind);
    if (p.api2) { if (p.api2->handles_address && !p.api2->handles_address(p.instance, address)) return false; return p.api2->write8 && p.api2->write8(p.instance, address, value); }
    if (p.api1) { if (p.api1->handles_address && !p.api1->handles_address(address)) return false; return p.api1->write8 && p.api1->write8(&runtime, address, value); }
    return false;
}
bool dc_plugin_read32(DCRuntime& runtime, std::uint32_t kind, std::uint32_t address, std::uint32_t& value) {
    auto p = find_plugin_fast(runtime, kind); if (!p.api2 || !p.api2->read32) return false;
    if (p.api2->handles_address && !p.api2->handles_address(p.instance, address)) return false;
    return p.api2->read32(p.instance, address, &value);
}
bool dc_plugin_write32(DCRuntime& runtime, std::uint32_t kind, std::uint32_t address, std::uint32_t value) {
    auto p = find_plugin_fast(runtime, kind); if (!p.api2 || !p.api2->write32) return false;
    if (p.api2->handles_address && !p.api2->handles_address(p.instance, address)) return false;
    return p.api2->write32(p.instance, address, value);
}
bool dc_plugin_tick(DCRuntime& runtime, std::uint32_t kind) {
    auto p = find_plugin_fast(runtime, kind);
    if (p.api2 && p.api2->tick) { p.api2->tick(p.instance, 0u); return true; }
    if (p.api1 && p.api1->tick) { p.api1->tick(&runtime); return true; }
    return false;
}

const void* dc_plugin_query_interface(DCRuntime& runtime, std::uint32_t kind, const char* interface_name, std::uint32_t min_version) {
    auto p = find_plugin_fast(runtime, kind);
    if (!p.api2 || !p.api2->query_interface || !interface_name) return nullptr;
    return p.api2->query_interface(p.instance, interface_name, min_version);
}


const DCRMapleControlV2* maple_control_for(const CachedPlugin& p) {
    if (!p.api2 || !p.api2->query_interface) return nullptr;
    return static_cast<const DCRMapleControlV2*>(
        p.api2->query_interface(p.instance, DCR_MAPLE_CONTROL_IFACE_V2, DCR_MAPLE_CONTROL_VERSION_V2));
}

const DCRMapleControlV3* maple_control_v3_for(const CachedPlugin& p) {
    if (!p.api2 || !p.api2->query_interface) return nullptr;
    return static_cast<const DCRMapleControlV3*>(
        p.api2->query_interface(p.instance, DCR_MAPLE_CONTROL_IFACE_V3, DCR_MAPLE_CONTROL_VERSION_V3));
}

const DCRGDROMControlV1* gdrom_control_for(const CachedPlugin& p) {
    if (!p.api2 || !p.api2->query_interface) return nullptr;
    return static_cast<const DCRGDROMControlV1*>(
        p.api2->query_interface(p.instance, DCR_GDROM_CONTROL_IFACE_V1, DCR_GDROM_CONTROL_VERSION_V1));
}

const DCRGDROMControlV2* gdrom_control_v2_for(const CachedPlugin& p) {
    if (!p.api2 || !p.api2->query_interface) return nullptr;
    return static_cast<const DCRGDROMControlV2*>(
        p.api2->query_interface(p.instance, DCR_GDROM_CONTROL_IFACE_V2, DCR_GDROM_CONTROL_VERSION_V2));
}

const DCRGDROMControlV3* gdrom_control_v3_for(const CachedPlugin& p) {
    if (!p.api2 || !p.api2->query_interface) return nullptr;
    return static_cast<const DCRGDROMControlV3*>(
        p.api2->query_interface(p.instance, DCR_GDROM_CONTROL_IFACE_V3, DCR_GDROM_CONTROL_VERSION_V3));
}

bool dc_plugin_maple_load_controller_profile(DCRuntime& runtime, const std::string& path) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    return ctl && ctl->load_controller_profile ? ctl->load_controller_profile(p.instance, path.c_str()) : false;
}
bool dc_plugin_maple_enable_host_input(DCRuntime& runtime, bool enabled) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->enable_host_input) return false; ctl->enable_host_input(p.instance, enabled); return true;
}
bool dc_plugin_maple_set_controller_present(DCRuntime& runtime, bool present) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->set_controller_present) return false; ctl->set_controller_present(p.instance, present); return true;
}
bool dc_plugin_maple_set_controller_state(DCRuntime& runtime, const MapleHostController& state, bool present) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->set_controller_state) return false;
    DCRMapleControllerStateV2 v{}; v.struct_size=sizeof(v); v.present=present; v.connected=state.connected;
    v.buttons=static_cast<std::uint16_t>(state.buttons & 0xffffu); v.ltrig=state.ltrig; v.rtrig=state.rtrig;
    v.joyx=state.joyx; v.joyy=state.joyy; v.joy2x=state.joy2x; v.joy2y=state.joy2y;
    ctl->set_controller_state(p.instance, &v); return true;
}
bool dc_plugin_maple_poll_host(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->poll_host) return false; ctl->poll_host(p.instance); return true;
}
bool dc_plugin_maple_get_controller_state(DCRuntime& runtime, MapleHostController& state, bool& present) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->get_controller_state) return false;
    DCRMapleControllerStateV2 v{}; v.struct_size=sizeof(v); if (!ctl->get_controller_state(p.instance,&v)) return false;
    present=v.present; state.connected=v.connected; state.buttons=v.buttons; state.ltrig=v.ltrig; state.rtrig=v.rtrig;
    state.joyx=v.joyx; state.joyy=v.joyy; state.joy2x=v.joy2x; state.joy2y=v.joy2y; return true;
}
bool dc_plugin_maple_get_controller_state_port(DCRuntime& runtime, std::uint32_t port, MapleHostController& state, bool& present) {
    auto p=find_plugin_fast(runtime,DCR_PLUGIN_KIND_MAPLE); const auto* ctl=maple_control_v3_for(p);
    if (!ctl || !ctl->get_port_controller_state || port>=4u) { if (port==0u) return dc_plugin_maple_get_controller_state(runtime,state,present); present=false; return false; }
    DCRMapleControllerStateV2 v{}; v.struct_size=sizeof(v); if(!ctl->get_port_controller_state(p.instance,port,&v)) return false;
    present=v.present; state.connected=v.connected; state.buttons=v.buttons; state.ltrig=v.ltrig; state.rtrig=v.rtrig; state.joyx=v.joyx; state.joyy=v.joyy; state.joy2x=v.joy2x; state.joy2y=v.joy2y; return true;
}
std::uint32_t dc_plugin_maple_controller_count(DCRuntime& runtime) {
    std::uint32_t count=0; for(std::uint32_t port=0;port<4u;++port){ MapleHostController s{}; bool present=false; if(dc_plugin_maple_get_controller_state_port(runtime,port,s,present)&&present&&s.connected) ++count; } return count;
}
bool dc_plugin_maple_print_stats(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_MAPLE); const auto* ctl = maple_control_for(p);
    if (!ctl || !ctl->print_stats) return false; ctl->print_stats(p.instance); return true;
}

// cpp37c GD-ROM phase 3. The plugin owns physical media geometry/I/O plus command/multi/DMA state;
// DCRuntime receives a compatibility snapshot so not-yet-migrated CDDA/status
// paths continue to behave exactly as before.
bool dc_plugin_gdrom_load_disc_map(DCRuntime& runtime, const std::string& path) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM);
    const auto* ctl = gdrom_control_for(p);
    if (!ctl || !ctl->load_disc_map || !ctl->get_summary || !ctl->get_track) return false;
    if (!ctl->load_disc_map(p.instance, path.c_str())) return false;
    DCRGDROMSummaryV1 summary{}; summary.struct_size = sizeof(summary);
    if (!ctl->get_summary(p.instance, &summary) || summary.logical_sector_size != 2048u || summary.track_count == 0u) return false;
    runtime.gd_tracks.clear();
    runtime.gd_tracks.reserve(summary.track_count);
    for (std::uint32_t i = 0u; i < summary.track_count; ++i) {
        DCRGDROMTrackV1 t{}; t.struct_size = sizeof(t);
        if (!ctl->get_track(p.instance, i, &t)) return false;
        GDDiscTrack out{};
        out.session=t.session; out.number=t.number; out.mode=t.mode; out.start_fad=t.start_fad; out.end_fad=t.end_fad;
        out.file_base=t.file_base; out.sector_size=t.sector_size; out.user_offset=t.user_offset; out.user_size=t.user_size;
        runtime.gd_tracks.push_back(out);
    }
    if (summary.boot_track >= runtime.gd_tracks.size()) return false;
    runtime.gd_boot_track = summary.boot_track;
    runtime.gd_sector_size = summary.logical_sector_size;
    runtime.gd_fad_base = summary.boot_fad;
    runtime.gd_iso_base = runtime.gd_tracks[summary.boot_track].file_base;
    runtime.gd_disc_path = (ctl->image_path && ctl->image_path(p.instance)) ? ctl->image_path(p.instance) : std::string{};
    runtime.gd_disc_stream.close(); runtime.gd_disc_stream.clear();
    // cpp37c phase 3 owns raw Red-Book/audio image I/O in dcr_gdrom.dll. Keep
    // the Runtime stream closed when V3 is present; it is opened only for the
    // built-in CDDA fallback used with older/no GD-ROM plugins.
    if (!gdrom_control_v3_for(p) && !runtime.gd_disc_path.empty())
        runtime.gd_disc_stream.open(runtime.gd_disc_path, std::ios::binary);
    runtime.gd_disc_ready = true;
    runtime.gd.current_fad = runtime.gd_fad_base;
    std::cout << "[GDROM PLUGIN] cpp37c media snapshot active | tracks=" << summary.track_count
              << " | sessions=" << summary.session_count << " | boot-fad=" << summary.boot_fad
              << " | logical-sector=" << summary.logical_sector_size << "\n";
    return true;
}

bool dc_plugin_gdrom_read_bytes(DCRuntime& runtime, std::uint32_t fad, std::uint32_t byte_offset,
                                std::uint32_t bytes, std::uint8_t* out, std::size_t out_size) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_for(p);
    return ctl && ctl->read_bytes && ctl->read_bytes(p.instance, fad, byte_offset, bytes, out, out_size);
}

bool dc_plugin_gdrom_build_toc(DCRuntime& runtime, std::uint32_t area, std::uint32_t* words, std::size_t word_count) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_for(p);
    return ctl && ctl->build_toc && ctl->build_toc(p.instance, area, words, word_count);
}

bool dc_plugin_gdrom_print_stats(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_for(p);
    if (!ctl || !ctl->print_stats) return false; ctl->print_stats(p.instance); return true;
}


bool dc_plugin_gdrom_phase2_available(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM);
    const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->version >= DCR_GDROM_CONTROL_VERSION_V2 && ctl->struct_size >= sizeof(DCRGDROMControlV2);
}

bool dc_plugin_gdrom_reset_command_state(DCRuntime& runtime, std::uint32_t current_fad) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->reset_command_state && ctl->reset_command_state(p.instance, current_fad);
}

bool dc_plugin_gdrom_request_command(DCRuntime& runtime, std::uint32_t command,
                                     const std::uint32_t* params4, std::uint32_t& request_id) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->request_command && ctl->request_command(p.instance, command, params4, &request_id);
}

bool dc_plugin_gdrom_get_command_state(DCRuntime& runtime, DCRGDROMCommandStateV2& state) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    state = {}; state.struct_size = sizeof(state);
    return ctl && ctl->get_command_state && ctl->get_command_state(p.instance, &state);
}

bool dc_plugin_gdrom_publish_command_state(DCRuntime& runtime, const DCRGDROMCommandStateV2& state,
                                           bool delegated_to_runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->publish_command_state && ctl->publish_command_state(p.instance, &state, delegated_to_runtime);
}

bool dc_plugin_gdrom_begin_multi(DCRuntime& runtime, std::uint32_t fad, std::uint32_t count) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->begin_multi && ctl->begin_multi(p.instance, fad, count);
}

bool dc_plugin_gdrom_multi_transfer(DCRuntime& runtime, std::uint32_t dest, std::uint32_t bytes, bool dma) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->multi_transfer && ctl->multi_transfer(p.instance, dest, bytes, dma);
}

bool dc_plugin_gdrom_abort_multi(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->abort_multi && ctl->abort_multi(p.instance);
}

bool dc_plugin_gdrom_set_callback(DCRuntime& runtime, std::uint32_t callback, std::uint32_t callback_arg) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->set_callback && ctl->set_callback(p.instance, callback, callback_arg);
}

bool dc_plugin_gdrom_dma_end_ack(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    return ctl && ctl->dma_end_ack && ctl->dma_end_ack(p.instance);
}

bool dc_plugin_gdrom_get_stats_v2(DCRuntime& runtime, DCRGDROMStatsV2& stats) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v2_for(p);
    stats = {}; stats.struct_size = sizeof(stats);
    return ctl && ctl->get_stats && ctl->get_stats(p.instance, &stats);
}

bool dc_plugin_gdrom_phase3_available(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM);
    const auto* ctl = gdrom_control_v3_for(p);
    return ctl && ctl->version >= DCR_GDROM_CONTROL_VERSION_V3 && ctl->struct_size >= sizeof(DCRGDROMControlV3);
}

bool dc_plugin_gdrom_cdda_reset(DCRuntime& runtime, std::uint32_t current_fad) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->reset_cdda || !ctl->reset_cdda(p.instance, current_fad)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_play_tracks(DCRuntime& runtime, std::uint32_t first_track,
                                       std::uint32_t last_track, std::uint32_t repeat) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->play_tracks || !ctl->play_tracks(p.instance, first_track, last_track, repeat)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_play_sectors(DCRuntime& runtime, std::uint32_t start_fad,
                                        std::uint32_t end_fad, std::uint32_t repeat) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->play_sectors || !ctl->play_sectors(p.instance, start_fad, end_fad, repeat)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_pause(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->pause || !ctl->pause(p.instance)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_release(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->release || !ctl->release(p.instance)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_stop(DCRuntime& runtime) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->stop || !ctl->stop(p.instance)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_seek(DCRuntime& runtime, std::uint32_t fad) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->seek || !ctl->seek(p.instance, fad)) return false;
    invalidate_gdrom_audio_cache(runtime); return true;
}
bool dc_plugin_gdrom_cdda_advance(DCRuntime& runtime, std::uint32_t sectors) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    return ctl && ctl->advance_sectors && ctl->advance_sectors(p.instance, sectors);
}

bool dc_plugin_gdrom_cdda_mix_frame(DCRuntime& runtime, bool host_clock,
                                     std::int16_t& left, std::int16_t& right) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    if (!ctl || !ctl->render_frames) return false;
    const auto gen = g_gdrom_audio_generation.load(std::memory_order_acquire);
    auto& c = g_gdrom_audio_tls;
    if (c.runtime != &runtime || c.generation != gen) {
        c = {};
        c.runtime = &runtime;
        c.generation = gen;
    }
    if (c.pos >= c.frames) {
        c.pos = 0u;
        c.frames = ctl->render_frames(p.instance, c.stereo.data(), 64u, host_clock);
        if (c.frames == 0u) return false;
    }
    left = c.stereo[c.pos * 2u + 0u];
    right = c.stereo[c.pos * 2u + 1u];
    ++c.pos;
    return true;
}

bool dc_plugin_gdrom_get_cdda_state_v3(DCRuntime& runtime, DCRGDROMCDDAStateV3& state) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    state = {}; state.struct_size = sizeof(state);
    return ctl && ctl->get_cdda_state && ctl->get_cdda_state(p.instance, &state);
}
bool dc_plugin_gdrom_get_stats_v3(DCRuntime& runtime, DCRGDROMStatsV3& stats) {
    auto p = find_plugin_fast(runtime, DCR_PLUGIN_KIND_GDROM); const auto* ctl = gdrom_control_v3_for(p);
    stats = {}; stats.struct_size = sizeof(stats);
    return ctl && ctl->get_stats && ctl->get_stats(p.instance, &stats);
}

// ABI1 network compatibility remains unchanged in cpp35a.
void dc_modem_tick(DCRuntime& runtime) { if (dc_plugin_tick(runtime, DCR_PLUGIN_KIND_NETWORK)) return; dc_builtin_modem_tick(runtime); }
std::uint8_t dc_modem_read8(DCRuntime& runtime, std::uint32_t address) { std::uint8_t v{}; if (dc_plugin_read8(runtime, DCR_PLUGIN_KIND_NETWORK, address, v)) return v; return dc_builtin_modem_read8(runtime, address); }
void dc_modem_write8(DCRuntime& runtime, std::uint32_t address, std::uint8_t value) { if (dc_plugin_write8(runtime, DCR_PLUGIN_KIND_NETWORK, address, value)) return; dc_builtin_modem_write8(runtime, address, value); }

bool dc_maple_read32(DCRuntime& runtime, std::uint32_t address, std::uint32_t& value) {
    if (dc_plugin_read32(runtime, DCR_PLUGIN_KIND_MAPLE, address, value)) return true;
    return dc_builtin_maple_read32(runtime, address, value);
}
bool dc_maple_write32(DCRuntime& runtime, std::uint32_t address, std::uint32_t value) {
    if (dc_plugin_write32(runtime, DCR_PLUGIN_KIND_MAPLE, address, value)) return true;
    return dc_builtin_maple_write32(runtime, address, value);
}

} // namespace dcrecomp_generated
