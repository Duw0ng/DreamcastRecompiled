#include "dcr_plugin_api.hpp"
#include "dcr_plugin_api_v2.hpp"
#include "dcr_input_focus.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <Xinput.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#endif

namespace dcrecomp_generated {
namespace {

constexpr std::uint32_t kMapleBasePhysical = 0x005F6C00u;
constexpr std::uint32_t kMapleDmaAddrPhysical = kMapleBasePhysical + 0x04u;
constexpr std::uint32_t kMapleDmaTselPhysical = kMapleBasePhysical + 0x10u;
constexpr std::uint32_t kMapleEnablePhysical = kMapleBasePhysical + 0x14u;
constexpr std::uint32_t kMapleStatePhysical = kMapleBasePhysical + 0x18u;
constexpr std::uint32_t kMapleSpeedPhysical = kMapleBasePhysical + 0x80u;
constexpr std::uint32_t kMapleDmaProtPhysical = kMapleBasePhysical + 0x8Cu;
constexpr std::uint32_t kAsicEventMapleDmaBit = 1u << 12;
constexpr std::uint32_t kMapleFuncController = 0x01000000u;
constexpr std::uint32_t kMapleFuncMemcard    = 0x02000000u;
constexpr std::uint32_t kMapleFuncLcd        = 0x04000000u;
constexpr std::uint32_t kMapleFuncClock      = 0x08000000u;
constexpr std::uint32_t kMapleFuncPuruPuru   = 0x00010000u;
constexpr std::uint32_t kMapleControllerCaps = 0xFE060F00u;
constexpr std::size_t kMapleVmuSize = 128u * 1024u;
constexpr std::size_t kMapleVmuBlockSize = 512u;
constexpr std::size_t kMapleVmuBlockCount = 256u;

struct MapleHostController {
    bool connected{true};
    std::uint32_t buttons{};
    std::uint8_t ltrig{};
    std::uint8_t rtrig{};
    std::int8_t joyx{};
    std::int8_t joyy{};
    std::int8_t joy2x{};
    std::int8_t joy2y{};
};

enum class MapleDeviceKind : std::uint32_t {
    Empty = DCR_MAPLE_DEVICE_EMPTY,
    Controller = DCR_MAPLE_DEVICE_CONTROLLER,
    Vmu = DCR_MAPLE_DEVICE_VMU,
    PuruPuru = DCR_MAPLE_DEVICE_PURUPURU,
};

struct MaplePortState {
    bool controller_present{};
    MapleHostController controller{};
    std::array<MapleDeviceKind, 2> expansions{MapleDeviceKind::Empty, MapleDeviceKind::Empty};
    std::uint32_t host_device{};
    std::string host_backend{"none"};
    std::string input_spec{"none"};
    bool mouse_initialized{};
    int mouse_x{};
    int mouse_y{};
};

struct MapleRumbleState {
    std::uint16_t ast{19u};
    std::uint32_t vibset{};
    std::uint16_t low_motor{};
    std::uint16_t high_motor{};
    std::uint64_t stop_at_ms{};
    std::uint64_t commands{};
};

struct MapleStateV2 {
    DCRHostServicesV2 host{};
    bool maple_host_input{};
    bool maple_controller_present{true};
    MapleHostController maple_controller{};
    bool maple_controller_profile_loaded{};
    std::string maple_controller_profile_path;
    std::string maple_controller_backend{"auto"};
    std::uint32_t maple_controller_device{};
    float maple_controller_deadzone{0.18f};
    std::unordered_map<std::string,std::string> maple_controller_bindings;

    // cpp36a Maple Device Tree. Port A mirrors the legacy V2 compatibility
    // fields above; B/C/D are independent host controllers. Expansion slots
    // are explicit even when empty so new MapleDevice implementations can be
    // attached without changing the bus/DMA dispatcher again.
    std::array<MaplePortState, 4> maple_ports{};
    std::array<std::array<MapleRumbleState, 2>, 4> maple_rumble{};
    bool maple_trace{};
    std::uint64_t maple_trace_seq{};

    bool maple_vmu_present{true};
    bool maple_vmu_initialized{};
    std::array<std::uint8_t,kMapleVmuSize> maple_vmu_flash{};
    std::string maple_vmu_path{"dreamcast_vmu_a1.bin"};
    std::uint64_t maple_vmu_devinfo{}, maple_vmu_minfo{}, maple_vmu_reads{}, maple_vmu_writes{}, maple_vmu_syncs{}, maple_vmu_persist_failures{};

    std::uint32_t maple_dma_address{}, maple_enable{}, maple_state{};
    std::uint64_t maple_dma_runs{}, maple_dma_frames{}, maple_desc_start{}, maple_desc_nop{}, maple_desc_other{}, maple_recv_fixups{}, maple_dma_irqs{};
    std::array<std::uint32_t,3> holly_pending{};
    std::unordered_map<std::uint32_t,std::uint32_t> mmio32;

    std::uint64_t maple_devinfo_responses{}, maple_getcond_responses{};
    std::uint64_t maple_host_polls{}, maple_xinput_polls{}, maple_winmm_polls{}, maple_keyboard_polls{}, maple_unfocused_polls{};
    bool maple_input_focused{true};
    std::uint16_t maple_keyboard_buttons{}, maple_xinput_buttons{}, maple_winmm_buttons{}, maple_buttons_seen_interval{};
    std::string maple_host_backend{"none"};
    std::uint32_t maple_host_device{};
    std::uint64_t maple_input_changes{};
    std::uint8_t maple_last_cmd{}, maple_last_dst{}, maple_last_src{};
    std::int8_t maple_last_unit{-1};
    std::uint32_t maple_last_function{};
    std::uint64_t maple_noncontroller_frames{};

    // Legacy opt-in probe fields remain private and disabled by default.
    bool probe_controller_a_then_start_lowlevel{};
    bool probe_controller_start_burst_lowlevel{};
    std::uint32_t probe_controller_poll{};
    std::uint64_t probe_start_burst_presses{};
    std::uint64_t pvr_frames{};
    void* pvr_window_handle{};
    std::uint64_t ticks{};
};

std::uint8_t dc_read8(MapleStateV2& s, std::uint32_t a) {
    return s.host.read8 ? s.host.read8(s.host.host_context, a) : 0u;
}
std::uint16_t dc_read16(MapleStateV2& s, std::uint32_t a) {
    return s.host.read16 ? s.host.read16(s.host.host_context, a) : 0u;
}
std::uint32_t dc_read32(MapleStateV2& s, std::uint32_t a) {
    return s.host.read32 ? s.host.read32(s.host.host_context, a) : 0u;
}
void dc_write8(MapleStateV2& s, std::uint32_t a, std::uint8_t v) {
    if (s.host.write8) s.host.write8(s.host.host_context, a, v);
}
void dc_write16(MapleStateV2& s, std::uint32_t a, std::uint16_t v) {
    if (s.host.write16) s.host.write16(s.host.host_context, a, v);
}
void dc_write32(MapleStateV2& s, std::uint32_t a, std::uint32_t v) {
    if (s.host.write32) s.host.write32(s.host.host_context, a, v);
}
void dc_load_bytes(MapleStateV2& s, std::uint32_t a, const std::uint8_t* data, std::size_t n) {
    if (!data) return;
    DCRMemoryRegionV2 region{};
    if (s.host.get_memory_region && s.host.get_memory_region(s.host.host_context, a, n, &region) && region.data && region.size >= n) {
        std::memcpy(region.data, data, n);
        return;
    }
    for (std::size_t i=0;i<n;++i) dc_write8(s,a+static_cast<std::uint32_t>(i),data[i]);
}

void maple_poll_windows_controller(MapleStateV2& s);
void maple_poll_windows_controller_port(MapleStateV2& s, std::uint32_t port);

const char* maple_kind_name(MapleDeviceKind kind) {
    switch (kind) {
        case MapleDeviceKind::Controller: return "controller";
        case MapleDeviceKind::Vmu: return "vmu";
        case MapleDeviceKind::PuruPuru: return "purupuru";
        default: return "empty";
    }
}

bool config_truthy(const char* v) {
    if (!v || !*v) return false;
    return v[0]=='1' || v[0]=='y' || v[0]=='Y' || v[0]=='t' || v[0]=='T' || v[0]=='o' || v[0]=='O';
}

std::string config_string(MapleStateV2& s, const std::string& key, const std::string& fallback) {
    const char* v = s.host.get_config ? s.host.get_config(s.host.host_context, key.c_str()) : nullptr;
    return v && *v ? std::string(v) : fallback;
}

std::string lower_copy(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return v;
}

MapleDeviceKind parse_expansion_kind(const std::string& raw) {
    const auto v = lower_copy(raw);
    if (v == "vmu") return MapleDeviceKind::Vmu;
    if (v == "purupuru" || v == "rumble") return MapleDeviceKind::PuruPuru;
    return MapleDeviceKind::Empty;
}

std::uint64_t monotonic_ms() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void maple_load_port_config(MapleStateV2& s) {
    for (std::uint32_t port=0; port<4u; ++port) {
        const char letter = static_cast<char>('A' + port);
        const std::string prefix = std::string("DCR_MAPLE_PORT_") + letter;
        const std::string def = port == 0u ? "auto" : "none";
        s.maple_ports[port].input_spec = lower_copy(config_string(s, prefix + "_DEVICE", def));
        const std::string slot1def = port == 0u ? "vmu" : "empty";
        const std::string slot2def = port == 0u ? "purupuru" : "empty";
        s.maple_ports[port].expansions[0] = parse_expansion_kind(config_string(s, prefix + "_SLOT1", slot1def));
        s.maple_ports[port].expansions[1] = parse_expansion_kind(config_string(s, prefix + "_SLOT2", slot2def));
        // cpp36b still has one persistent VMU image, so only A1 may expose it.
        if (s.maple_ports[port].expansions[0] == MapleDeviceKind::Vmu && !(port == 0u))
            s.maple_ports[port].expansions[0] = MapleDeviceKind::Empty;
        if (s.maple_ports[port].expansions[1] == MapleDeviceKind::Vmu)
            s.maple_ports[port].expansions[1] = MapleDeviceKind::Empty;
    }
    s.maple_vmu_present = s.maple_ports[0].expansions[0] == MapleDeviceKind::Vmu;
}

MapleHostController& maple_port_controller(MapleStateV2& s, std::uint32_t port) {
    return port == 0u ? s.maple_controller : s.maple_ports[port & 3u].controller;
}
const MapleHostController& maple_port_controller(const MapleStateV2& s, std::uint32_t port) {
    return port == 0u ? s.maple_controller : s.maple_ports[port & 3u].controller;
}
bool maple_port_controller_present(const MapleStateV2& s, std::uint32_t port) {
    return port == 0u ? s.maple_controller_present : s.maple_ports[port & 3u].controller_present;
}
void maple_set_port_controller_present(MapleStateV2& s, std::uint32_t port, bool present) {
    if (port == 0u) { s.maple_controller_present = present; s.maple_controller.connected = present; }
    s.maple_ports[port & 3u].controller_present = present;
    s.maple_ports[port & 3u].controller.connected = present;
}
void maple_sync_port_a_tree(MapleStateV2& s) {
    s.maple_ports[0].controller_present = s.maple_controller_present;
    s.maple_ports[0].controller = s.maple_controller;
    s.maple_ports[0].host_device = s.maple_controller_device;
    s.maple_ports[0].host_backend = s.maple_host_backend;
    s.maple_ports[0].expansions[0] = s.maple_vmu_present ? MapleDeviceKind::Vmu : MapleDeviceKind::Empty;
}
std::uint8_t maple_expansion_mask(const MapleStateV2& s, std::uint32_t port) {
    std::uint8_t mask = 0u;
    if (port >= s.maple_ports.size()) return mask;
    for (std::uint32_t slot=0; slot<2u; ++slot)
        if (s.maple_ports[port].expansions[slot] != MapleDeviceKind::Empty) mask |= static_cast<std::uint8_t>(1u << slot);
    return mask;
}
MapleDeviceKind maple_device_kind(const MapleStateV2& s, std::uint32_t port, int unit) {
    if (port >= s.maple_ports.size()) return MapleDeviceKind::Empty;
    if (unit == 0) return maple_port_controller_present(s, port) ? MapleDeviceKind::Controller : MapleDeviceKind::Empty;
    if (unit >= 1 && unit <= 2) return s.maple_ports[port].expansions[static_cast<std::size_t>(unit-1)];
    return MapleDeviceKind::Empty;
}
void maple_trace_line(MapleStateV2& s, std::uint8_t cmd, std::uint8_t dst, std::uint8_t src, int port, int unit, std::uint32_t send_words, MapleDeviceKind kind) {
    if (!s.maple_trace) return;
    std::cout << "[MAPLE TRACE #" << ++s.maple_trace_seq << "] port=" << static_cast<char>('A'+std::clamp(port,0,3))
              << " unit=" << unit << " device=" << maple_kind_name(kind)
              << " cmd=" << static_cast<unsigned>(cmd) << " dst=0x" << std::hex << static_cast<unsigned>(dst)
              << " src=0x" << static_cast<unsigned>(src) << std::dec << " words=" << send_words << "\n";
}

std::uint8_t maple_bus_addr(int port, int unit) {
    std::uint8_t addr = static_cast<std::uint8_t>((port & 3) << 6);
    if (unit == 0) addr |= 0x20u;
    else if (unit > 0 && unit < 6) addr |= static_cast<std::uint8_t>(1u << (unit - 1));
    return addr;
}

void maple_write_response_header(MapleStateV2& s, std::uint32_t recv,
                                 std::int8_t response, std::uint8_t dst,
                                 std::uint8_t src, std::uint8_t words) {
    const std::uint32_t header = static_cast<std::uint8_t>(response)
                               | (static_cast<std::uint32_t>(dst) << 8)
                               | (static_cast<std::uint32_t>(src) << 16)
                               | (static_cast<std::uint32_t>(words) << 24);
    dc_write32(s, recv, header);
}

void maple_write_padded_text(MapleStateV2& s, std::uint32_t address,
                             const char* text, std::size_t width) {
    std::vector<std::uint8_t> tmp(width, static_cast<std::uint8_t>(' '));
    const std::size_t n = std::min(width, std::strlen(text));
    std::memcpy(tmp.data(), text, n);
    dc_load_bytes(s, address, tmp.data(), tmp.size());
}

void maple_write_devinfo_response(MapleStateV2& s, std::uint32_t recv,
                                  std::uint8_t request_src, std::uint8_t device_addr) {
    // maple_devinfo_t is exactly 112 bytes / 28 response words.
    maple_write_response_header(s, recv, 5, request_src, device_addr, 28u);
    dc_write32(s, recv + 4u, kMapleFuncController);
    dc_write32(s, recv + 8u, kMapleControllerCaps);
    dc_write32(s, recv + 12u, 0u);
    dc_write32(s, recv + 16u, 0u);
    dc_write8(s, recv + 20u, 0xFFu); // region-free
    dc_write8(s, recv + 21u, 0u);    // connector points up
    maple_write_padded_text(s, recv + 22u, "DreamcastRecomp Controller", 30u);
    maple_write_padded_text(s, recv + 52u,
                            "Produced By or Under License From SEGA ENTERPRISES,LTD.", 60u);
    dc_write16(s, recv + 112u, 0x01AEu);
    dc_write16(s, recv + 114u, 0x01F4u);
    ++s.maple_devinfo_responses;
}

void maple_write_getcond_response(MapleStateV2& s, std::uint32_t recv,
                                  std::uint8_t request_src, std::uint8_t device_addr, std::uint32_t port) {
    if (s.maple_host_input) maple_poll_windows_controller_port(s, port);
    auto c = maple_port_controller(s, port);
    const bool allow_probes = port == 0u;

    // When both legacy controller probes are enabled we treat that combination
    // as the dedicated SGGG New Game autopilot. This keeps the s ABI
    // unchanged while giving SGGG an explicit opt-in route that uses the real
    // low-level Maple GETCOND path.
    const bool sggg_newgame_probe = allow_probes && s.probe_controller_a_then_start_lowlevel
                                 && s.probe_controller_start_burst_lowlevel;

    if (sggg_newgame_probe) {
        const std::uint32_t poll = s.probe_controller_poll++;
        const std::uint64_t frame = s.pvr_frames;
        const bool start = frame >= 1800u && frame <= 5000u && ((poll % 90u) < 4u);
        const bool press_a = frame >= 2750u && frame <= 20000u && ((poll % 48u) < 10u);
        if (start) {
            c.buttons = static_cast<std::uint16_t>(c.buttons | (1u << 3));
            if ((poll % 90u) == 0u)
                std::cout << "[DreamcastRecomp SGGG autopilot] START at GETCOND=" << poll
                          << " pvr-frame=" << frame << "\n";
        }
        if (press_a) {
            c.buttons = static_cast<std::uint16_t>(c.buttons | (1u << 2));
            if ((poll % 48u) == 0u)
                std::cout << "[DreamcastRecomp SGGG autopilot] A at GETCOND=" << poll
                          << " pvr-frame=" << frame << "\n";
        }
    }

    // v0.1.1 CT2 regression harness: inject discrete buttons through the real
    // low-level Maple GETCOND path. It is opt-in via --probe-controller-a-then-start.
    if (allow_probes && s.probe_controller_a_then_start_lowlevel && !sggg_newgame_probe) {
        const std::uint32_t poll = s.probe_controller_poll++;
        std::uint32_t buttons = 0u;
        // Keep the stages non-overlapping so an A pulse cannot cancel a START
        // press while CT2 is transitioning from VMU setup to the title screen.
        if (poll >= 80u && poll <= 760u && ((poll - 80u) % 80u) < 8u) {
            buttons = 1u << 2;
            if (((poll - 80u) % 80u) == 0u) std::cout << "[DreamcastRecomp CT2 autopilot] VMU/A at GETCOND=" << poll << "\n";
        }
        if (poll >= 800u && poll <= 1700u && ((poll - 800u) % 80u) < 16u) {
            buttons = 1u << 3;
            if (((poll - 800u) % 80u) == 0u) std::cout << "[DreamcastRecomp CT2 autopilot] START at GETCOND=" << poll << "\n";
        }
        if (poll >= 1750u && poll <= 4100u && ((poll - 1750u) % 80u) < 12u) {
            buttons = 1u << 2;
            if (((poll - 1750u) % 80u) == 0u) std::cout << "[DreamcastRecomp CT2 autopilot] MENU/A at GETCOND=" << poll << "\n";
        }
        if (poll >= 4200u && ((poll - 4200u) % 40u) < 28u) {
            buttons = 1u << 2;
            if (poll == 4200u) std::cout << "[DreamcastRecomp CT2 autopilot] gameplay gas phase at GETCOND=" << poll << "\n";
        }
        c.buttons = static_cast<std::uint16_t>(buttons);
    }

    // 0.1.0 Lodoss START fix: retail/Katana titles can bypass the high-level
    // KOS maple_dev_status override and consume raw Maple GETCOND packets.
    // Keep the probe opt-in, but inject its finite START burst here so both
    // SDK and custom/low-level Maple paths observe the same button pulses.
    if (allow_probes && s.probe_controller_start_burst_lowlevel && !sggg_newgame_probe) {
        // SGGG/long-intro diagnostic mode: do not perturb logos or early boot.
        // Once real presentation has reached the late intro/title window, pulse
        // START through the actual low-level GETCOND response. This remains
        // strictly opt-in and never affects normal/manual input.
        const std::uint64_t frame = s.pvr_frames;
        const std::uint64_t poll = s.maple_getcond_responses;
        const bool in_window = frame >= 1800u && frame <= 5000u;
        const bool start = in_window && ((poll % 90u) < 4u);
        if (start) {
            c.buttons = static_cast<std::uint16_t>(c.buttons | (1u << 3));
            if ((poll % 90u) == 0u) {
                ++s.probe_start_burst_presses;
                std::cout << "[DreamcastRecomp smoke] late low-level Maple START pulse "
                          << s.probe_start_burst_presses << " at GETCOND #"
                          << poll << " pvr-frame=" << frame << "\n";
            }
        }
    }

    maple_write_response_header(s, recv, 8, request_src, device_addr, 3u);
    dc_write32(s, recv + 4u, kMapleFuncController);
    const std::uint16_t raw_buttons = static_cast<std::uint16_t>((~c.buttons) & 0xFFFFu);
    const std::uint32_t cond0 = static_cast<std::uint32_t>(raw_buttons)
                              | (static_cast<std::uint32_t>(c.rtrig) << 16)
                              | (static_cast<std::uint32_t>(c.ltrig) << 24);
    const auto axis = [](std::int8_t v) -> std::uint8_t {
        return static_cast<std::uint8_t>(static_cast<int>(v) + 128);
    };
    const std::uint32_t cond1 = static_cast<std::uint32_t>(axis(c.joyx))
                              | (static_cast<std::uint32_t>(axis(c.joyy)) << 8)
                              | (static_cast<std::uint32_t>(axis(c.joy2x)) << 16)
                              | (static_cast<std::uint32_t>(axis(c.joy2y)) << 24);
    dc_write32(s, recv + 8u, cond0);
    dc_write32(s, recv + 12u, cond1);
    ++s.maple_getcond_responses;
}



#ifdef _WIN32

enum class SonyPadKind { None, DualShock4, DualSense };

struct SonyHidPad {
    HANDLE handle{INVALID_HANDLE_VALUE};
    SonyPadKind kind{SonyPadKind::None};
    std::uint16_t product_id{};
    std::uint16_t output_report_len{};
    bool bluetooth{};
    std::uint8_t sequence{};
    std::string label;
};

std::mutex g_sony_hid_mutex;
std::vector<SonyHidPad> g_sony_hid_pads;
bool g_sony_hid_scanned = false;

SonyPadKind sony_kind_from_pid(std::uint16_t pid) {
    // Official Sony USB product IDs used by DualShock 4 / DualSense family.
    if (pid == 0x05C4u || pid == 0x09CCu || pid == 0x0BA0u) return SonyPadKind::DualShock4;
    if (pid == 0x0CE6u || pid == 0x0DF2u) return SonyPadKind::DualSense;
    return SonyPadKind::None;
}

const char* sony_kind_name(SonyPadKind kind) {
    switch (kind) {
        case SonyPadKind::DualShock4: return "DualShock 4";
        case SonyPadKind::DualSense: return "DualSense / PS5";
        default: return "Sony HID";
    }
}

void close_sony_hid_pads() {
    for (auto& pad : g_sony_hid_pads) {
        if (pad.handle != INVALID_HANDLE_VALUE) CloseHandle(pad.handle);
        pad.handle = INVALID_HANDLE_VALUE;
    }
    g_sony_hid_pads.clear();
}

void scan_sony_hid_pads_locked(bool force = false) {
    if (g_sony_hid_scanned && !force) return;
    close_sony_hid_pads();
    g_sony_hid_scanned = true;

    GUID hid_guid{};
    HidD_GetHidGuid(&hid_guid);
    HDEVINFO devs = SetupDiGetClassDevsW(&hid_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) return;

    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA iface{};
        iface.cbSize = sizeof(iface);
        if (!SetupDiEnumDeviceInterfaces(devs, nullptr, &hid_guid, index, &iface)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(devs, &iface, nullptr, 0, &required, nullptr);
        if (!required) continue;
        std::vector<std::uint8_t> storage(required);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devs, &iface, detail, required, nullptr, nullptr)) continue;

        HANDLE h = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            h = CreateFileW(detail->DevicePath, GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        }
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attr{};
        attr.Size = sizeof(attr);
        if (!HidD_GetAttributes(h, &attr) || attr.VendorID != 0x054Cu) {
            CloseHandle(h);
            continue;
        }
        const auto kind = sony_kind_from_pid(attr.ProductID);
        if (kind == SonyPadKind::None) {
            CloseHandle(h);
            continue;
        }

        USHORT output_len = 0;
        USHORT feature_len = 0;
        PHIDP_PREPARSED_DATA ppd = nullptr;
        if (HidD_GetPreparsedData(h, &ppd)) {
            HIDP_CAPS caps{};
            if (HidP_GetCaps(ppd, &caps) == HIDP_STATUS_SUCCESS) {
                output_len = caps.OutputReportByteLength;
                feature_len = caps.FeatureReportByteLength;
            }
            HidD_FreePreparsedData(ppd);
        }
        // Windows descriptors vary slightly by transport/driver. SDL uses a 48-byte
        // DualSense USB effects report, while other stacks expose the full 63-byte
        // report. Bluetooth is the signed 78-byte report.
        const bool ds_usb = output_len >= 48u && output_len <= 64u;
        const bool valid_len = (kind == SonyPadKind::DualSense && (ds_usb || output_len == 78u)) ||
                               (kind == SonyPadKind::DualShock4 && (output_len == 32u || output_len == 78u));
        if (!valid_len) {
            CloseHandle(h);
            continue;
        }
        SonyHidPad pad{};
        pad.handle = h;
        pad.kind = kind;
        pad.product_id = attr.ProductID;
        pad.output_report_len = output_len;
        pad.bluetooth = output_len == 78u;
        pad.label = std::string(sony_kind_name(kind)) + (pad.bluetooth ? " Bluetooth HID" : " USB HID");
        if (kind == SonyPadKind::DualSense && pad.bluetooth && feature_len >= 20u) {
            // Reading pairing info switches native DualSense Bluetooth into the full
            // enhanced-report path on Windows/SDL-compatible stacks. Best effort only.
            std::vector<std::uint8_t> feature(feature_len, 0u);
            feature[0] = 0x09u;
            HidD_GetFeature(h, feature.data(), static_cast<ULONG>(feature.size()));
        }
        g_sony_hid_pads.push_back(std::move(pad));
    }
    SetupDiDestroyDeviceInfoList(devs);
}

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1u) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

void store_le32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8u);
    p[2] = static_cast<std::uint8_t>(v >> 16u);
    p[3] = static_cast<std::uint8_t>(v >> 24u);
}

bool send_sony_hid_report(SonyHidPad& pad, std::uint16_t low, std::uint16_t high) {
    if (pad.handle == INVALID_HANDLE_VALUE) return false;
    const auto left = static_cast<std::uint8_t>((static_cast<std::uint32_t>(low) + 128u) / 257u);
    const auto right = static_cast<std::uint8_t>((static_cast<std::uint32_t>(high) + 128u) / 257u);
    std::vector<std::uint8_t> report(pad.output_report_len, 0u);

    if (pad.kind == SonyPadKind::DualSense) {
        if (pad.bluetooth) {
            report[0] = 0x31u;
            report[1] = static_cast<std::uint8_t>((pad.sequence++ & 0x0Fu) << 4u);
            report[2] = 0x10u;
            report[3] = 0x03u; // compatible vibration + haptics select
            report[5] = right;
            report[6] = left;
            std::vector<std::uint8_t> signed_data;
            signed_data.reserve(1u + report.size() - 4u);
            signed_data.push_back(0xA2u); // Sony output CRC seed
            signed_data.insert(signed_data.end(), report.begin(), report.end() - 4);
            store_le32(report.data() + report.size() - 4u, crc32_ieee(signed_data.data(), signed_data.size()));
        } else {
            report[0] = 0x02u;
            report[1] = 0x03u; // compatible vibration + haptics select
            report[3] = right;
            report[4] = left;
        }
    } else if (pad.kind == SonyPadKind::DualShock4) {
        if (pad.bluetooth) {
            report[0] = 0x11u;
            report[1] = 0xC4u; // HID + CRC + 4ms poll interval
            report[3] = 0x01u; // motor enable
            report[6] = right;
            report[7] = left;
            std::vector<std::uint8_t> signed_data;
            signed_data.reserve(1u + report.size() - 4u);
            signed_data.push_back(0xA2u);
            signed_data.insert(signed_data.end(), report.begin(), report.end() - 4);
            store_le32(report.data() + report.size() - 4u, crc32_ieee(signed_data.data(), signed_data.size()));
        } else {
            report[0] = 0x05u;
            report[1] = 0x01u; // motor enable
            report[4] = right;
            report[5] = left;
        }
    } else {
        return false;
    }

    DWORD written = 0;
    if (WriteFile(pad.handle, report.data(), static_cast<DWORD>(report.size()), &written, nullptr) && written == report.size()) return true;
    // Some Windows HID stacks prefer the HID control path over WriteFile.
    return HidD_SetOutputReport(pad.handle, report.data(), static_cast<ULONG>(report.size())) == TRUE;
}

bool sony_hid_rumble(std::uint32_t preferred_index, std::uint16_t low, std::uint16_t high, std::string* label = nullptr) {
    std::lock_guard<std::mutex> lock(g_sony_hid_mutex);
    scan_sony_hid_pads_locked(false);
    if (g_sony_hid_pads.empty()) return false;
    std::size_t index = g_sony_hid_pads.size() == 1u ? 0u : std::min<std::size_t>(preferred_index, g_sony_hid_pads.size() - 1u);
    auto& pad = g_sony_hid_pads[index];
    if (label) *label = pad.label;
    if (send_sony_hid_report(pad, low, high)) return true;
    // Hotplug or Windows re-enumeration: refresh once and retry.
    scan_sony_hid_pads_locked(true);
    if (g_sony_hid_pads.empty()) return false;
    index = g_sony_hid_pads.size() == 1u ? 0u : std::min<std::size_t>(preferred_index, g_sony_hid_pads.size() - 1u);
    auto& retry = g_sony_hid_pads[index];
    if (label) *label = retry.label;
    return send_sony_hid_report(retry, low, high);
}

bool xinput_rumble_device(std::uint32_t device, std::uint16_t low, std::uint16_t high) {
    using XInputSetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_VIBRATION*);
    static HMODULE module = nullptr;
    static XInputSetStateFn set_state = nullptr;
    static bool checked = false;
    if (!checked) {
        checked = true;
        const char* dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
        for (const char* dll : dlls) {
            module = LoadLibraryA(dll);
            if (!module) continue;
            set_state = reinterpret_cast<XInputSetStateFn>(GetProcAddress(module, "XInputSetState"));
            if (set_state) break;
            FreeLibrary(module);
            module = nullptr;
        }
    }
    if (!set_state) return false;
    XINPUT_VIBRATION v{};
    v.wLeftMotorSpeed = low;
    v.wRightMotorSpeed = high;
    return set_state(device, &v) == ERROR_SUCCESS;
}

int maple_test_host_rumble_impl(const char* input_spec, std::uint16_t low, std::uint16_t high, char* status, std::size_t status_size) {
    const std::string spec = input_spec ? lower_copy(input_spec) : std::string("auto");
    std::uint32_t device = 0u;
    const auto colon = spec.find(':');
    std::string kind = spec;
    if (colon != std::string::npos) {
        kind = spec.substr(0u, colon);
        try { device = static_cast<std::uint32_t>(std::stoul(spec.substr(colon + 1u))); } catch (...) { device = 0u; }
    }
    std::string message;
    bool ok = false;
    if (kind == "xinput") {
        ok = xinput_rumble_device(device, low, high);
        message = ok ? "XInput rumble active" : "XInput rumble unavailable";
    } else if (kind == "none" || kind == "disabled" || kind == "keyboard_mouse" || kind == "keyboard+mouse") {
        message = "Selected device has no haptics";
    } else {
        std::string sony;
        ok = sony_hid_rumble(device, low, high, &sony);
        if (ok) message = sony + " rumble active";
        else if (kind == "auto" && xinput_rumble_device(device, low, high)) { ok = true; message = "XInput rumble active"; }
        else message = "No compatible XInput / DualShock 4 / DualSense haptics device found";
    }
    if (status && status_size) {
        std::snprintf(status, status_size, "%s", message.c_str());
        status[status_size - 1u] = '\0';
    }
    return ok ? 1 : 0;
}


int maple_probe_host_haptics_impl(const char* input_spec, char* status, std::size_t status_size) {
    const std::string spec = input_spec ? lower_copy(input_spec) : std::string("auto");
    std::uint32_t device = 0u;
    const auto colon = spec.find(':');
    std::string kind = spec;
    if (colon != std::string::npos) {
        kind = spec.substr(0u, colon);
        try { device = static_cast<std::uint32_t>(std::stoul(spec.substr(colon + 1u))); } catch (...) { device = 0u; }
    }
    std::string message;
    bool ok = false;
    if (kind == "xinput") {
        ok = true;
        message = "XInput haptics ready";
    } else if (kind == "none" || kind == "disabled" || kind == "keyboard_mouse" || kind == "keyboard+mouse") {
        message = "Selected device has no haptics";
    } else {
        std::lock_guard<std::mutex> lock(g_sony_hid_mutex);
        scan_sony_hid_pads_locked(false);
        if (!g_sony_hid_pads.empty()) {
            const std::size_t i = g_sony_hid_pads.size() == 1u ? 0u : std::min<std::size_t>(device, g_sony_hid_pads.size() - 1u);
            message = g_sony_hid_pads[i].label + " ready";
            ok = true;
        } else if (kind == "auto") {
            message = "Automatic: no Sony HID haptics detected; XInput may still be available";
        } else {
            message = "No DualShock 4 / DualSense HID haptics device detected";
        }
    }
    if (status && status_size) {
        std::snprintf(status, status_size, "%s", message.c_str());
        status[status_size - 1u] = '\0';
    }
    return ok ? 1 : 0;
}

#endif // _WIN32


void maple_host_rumble(MapleStateV2& s, std::uint32_t port, std::uint16_t low, std::uint16_t high) {
#ifdef _WIN32
    if (port >= s.maple_ports.size()) return;
    auto& ps = s.maple_ports[port];
    std::uint32_t device = ps.host_device;
    std::string kind = lower_copy(ps.host_backend);
    const std::string spec = lower_copy(ps.input_spec);
    const auto colon = spec.find(':');
    if (colon != std::string::npos) {
        kind = spec.substr(0u, colon);
        try { device = static_cast<std::uint32_t>(std::stoul(spec.substr(colon + 1u))); } catch (...) {}
    }

    bool ok = false;
    std::string backend_name;
    if (kind == "xinput" || ps.host_backend == "xinput") {
        ok = xinput_rumble_device(device, low, high);
        backend_name = "XInput";
    } else if (kind != "none" && kind != "disabled" && kind != "keyboard_mouse" && kind != "keyboard+mouse") {
        ok = sony_hid_rumble(device, low, high, &backend_name);
    }
    if (!ok && (kind == "auto" || kind == "winmm" || kind == "directinput" || kind == "ps4" || kind == "ps5" || ps.host_backend == "directinput/winmm")) {
        ok = sony_hid_rumble(device, low, high, &backend_name);
    }
    if (s.maple_trace && (low || high)) {
        if (ok) std::cout << "[MAPLE RUMBLE] Port " << static_cast<char>('A'+port) << " -> " << backend_name << " low=" << low << " high=" << high << "\n";
        else std::cout << "[MAPLE RUMBLE] Port " << static_cast<char>('A'+port) << " host=" << ps.host_backend << " has no compatible haptics backend\n";
    }
#else
    (void)s; (void)port; (void)low; (void)high;
#endif
}

void maple_stop_rumble(MapleStateV2& s, std::uint32_t port, std::uint32_t slot) {
    if (port>=4u || slot>=2u) return;
    auto& r=s.maple_rumble[port][slot];
    r.low_motor=r.high_motor=0u; r.stop_at_ms=0u;
    maple_host_rumble(s,port,0u,0u);
}

void maple_apply_purupuru(MapleStateV2& s, std::uint32_t port, std::uint32_t slot, std::uint32_t effect) {
    if (port>=4u || slot>=2u) return;
    auto& r=s.maple_rumble[port][slot]; r.vibset=effect; ++r.commands;
    const std::uint32_t pow_pos=(effect>>8u)&7u, pow_neg=(effect>>12u)&7u, freq=(effect>>16u)&0xffu;
    int inc=static_cast<int>((effect>>24u)&0xffu); if (effect&0x8000u) inc=-inc; else if (!(effect&0x0800u)) inc=0;
    const bool cont=(effect&1u)!=0u;
    const float power=std::min((pow_pos+pow_neg)/7.0f,1.0f);
    const std::uint32_t ast_ms=static_cast<std::uint32_t>(r.ast)*250u+250u;
    std::uint32_t duration=ast_ms;
    if (freq>0u && (!cont || inc!=0)) {
        const int peak=static_cast<int>(std::max(pow_pos,pow_neg));
        const int base=inc ? std::abs(inc)*std::max(peak,1) : 1;
        duration=std::min<std::uint32_t>(static_cast<std::uint32_t>(1000*base/static_cast<int>(freq)),ast_ms);
    }
    const std::uint16_t intensity=static_cast<std::uint16_t>(std::clamp(static_cast<int>(std::lround(power*65535.0f)),0,65535));
    if (power<=0.0f) { maple_stop_rumble(s,port,slot); return; }
    // XInput exposes low/high frequency motors. Bias by the Dreamcast frequency while
    // keeping both active so games written for a single Puru motor still feel natural.
    const float hf=std::clamp(static_cast<float>(freq)/30.0f,0.0f,1.0f);
    r.low_motor=static_cast<std::uint16_t>(std::lround(intensity*(1.0f-0.45f*hf)));
    r.high_motor=static_cast<std::uint16_t>(std::lround(intensity*(0.35f+0.65f*hf)));
    r.stop_at_ms=monotonic_ms()+duration;
    maple_host_rumble(s,port,r.low_motor,r.high_motor);
    if (s.maple_trace) std::cout << "[MAPLE RUMBLE] Port " << static_cast<char>('A'+port) << " slot=" << (slot+1u)
        << " effect=0x" << std::hex << effect << std::dec << " power=" << power << " freq=" << freq << " duration=" << duration << "ms\n";
}

void maple_write_purupuru_devinfo_response(MapleStateV2& s, std::uint32_t recv,
                                            std::uint8_t request_src, std::uint8_t device_addr,
                                            bool all_info) {
    maple_write_response_header(s, recv, all_info ? 6 : 5, request_src, device_addr, 28u);
    dc_write32(s, recv+4u, kMapleFuncPuruPuru);
    dc_write32(s, recv+8u, 0x00000101u); dc_write32(s, recv+12u, 0u); dc_write32(s, recv+16u, 0u);
    dc_write8(s, recv+20u, 0xffu); dc_write8(s, recv+21u, 0u);
    maple_write_padded_text(s, recv+22u, "Puru Puru Pack", 30u);
    maple_write_padded_text(s, recv+52u, "Produced By or Under License From SEGA ENTERPRISES,LTD.", 60u);
    dc_write16(s, recv+112u, 0x00C8u); dc_write16(s, recv+114u, 0x0640u);
}

void maple_process_purupuru(MapleStateV2& s, std::uint32_t recv, std::uint32_t cursor,
                            std::uint32_t send_words, std::uint8_t cmd, std::uint8_t src,
                            std::uint8_t dst, std::uint32_t port, std::uint32_t slot) {
    auto& r=s.maple_rumble[port][slot];
    const std::uint32_t function=send_words ? dc_read32(s,cursor+12u) : 0u;
    if (cmd==1u || cmd==2u) maple_write_purupuru_devinfo_response(s,recv,src,dst,cmd==2u);
    else if (cmd==9u) { maple_write_response_header(s,recv,8,src,dst,2u); dc_write32(s,recv+4u,kMapleFuncPuruPuru); dc_write32(s,recv+8u,r.vibset); }
    else if (cmd==10u) { maple_write_response_header(s,recv,8,src,dst,2u); dc_write32(s,recv+4u,kMapleFuncPuruPuru); dc_write32(s,recv+8u,0x3B07E010u); }
    else if (cmd==11u) { maple_write_response_header(s,recv,8,src,dst,3u); dc_write32(s,recv+4u,kMapleFuncPuruPuru); dc_write32(s,recv+8u,0u); dc_write32(s,recv+12u,(static_cast<std::uint32_t>(r.ast)<<16u)|2u); }
    else if (cmd==12u) { if (send_words>=3u) r.ast=dc_read8(s,cursor+20u); maple_write_response_header(s,recv,7,src,dst,0u); }
    else if (cmd==14u && function==kMapleFuncPuruPuru && send_words>=2u) { maple_apply_purupuru(s,port,slot,dc_read32(s,cursor+16u)); maple_write_response_header(s,recv,7,src,dst,0u); }
    else if (cmd==3u || cmd==4u) { maple_stop_rumble(s,port,slot); maple_write_response_header(s,recv,7,src,dst,0u); }
    else if (function && function!=kMapleFuncPuruPuru) maple_write_response_header(s,recv,-2,src,dst,0u);
    else maple_write_response_header(s,recv,-3,src,dst,0u);
}

std::uint32_t maple_bswap32(std::uint32_t value) {
    return ((value & 0x000000FFu) << 24u)
         | ((value & 0x0000FF00u) << 8u)
         | ((value & 0x00FF0000u) >> 8u)
         | ((value & 0xFF000000u) >> 24u);
}

void maple_vmu_write_le16(std::array<std::uint8_t, kMapleVmuSize>& flash,
                          std::size_t offset, std::uint16_t value) {
    flash[offset + 0u] = static_cast<std::uint8_t>(value & 0xFFu);
    flash[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
}

void maple_vmu_format(MapleStateV2& s) {
    auto& flash = s.maple_vmu_flash;
    flash.fill(0u);

    // Standard 128 KiB / 200 user-block VMU layout. Block 255 contains the
    // system/root record, block 254 the FAT and blocks 253..241 the directory.
    // Data blocks 0..199 start free (0xFFFC); 0xFFFA is end-of-chain.
    const std::size_t root = 255u * kMapleVmuBlockSize;
    std::fill(flash.begin() + static_cast<std::ptrdiff_t>(root),
              flash.begin() + static_cast<std::ptrdiff_t>(root + 16u), 0x55u);

    // The 24-byte media-info record begins at root+0x40. These bytes also map
    // directly onto the conventional VMU root fields used by retail SDKs.
    std::size_t m = root + 0x40u;
    const auto put16 = [&](std::uint16_t v) {
        maple_vmu_write_le16(flash, m, v);
        m += 2u;
    };
    put16(0x00FFu); // total size / highest block
    put16(0x0000u); // partition
    put16(0x00FFu); // system area block
    put16(0x00FEu); // FAT block
    put16(0x0001u); // FAT size
    put16(0x00FDu); // directory start
    put16(0x000Du); // directory blocks
    flash[m++] = 0u; // volume icon
    flash[m++] = 0u; // reserved
    put16(0x00C8u); // 200 user blocks
    put16(0x001Fu); // media-info compatibility field
    put16(0x0000u); // reserved

    const std::size_t fat = 254u * kMapleVmuBlockSize;
    for (std::size_t block = 0; block < kMapleVmuBlockCount; ++block) {
        maple_vmu_write_le16(flash, fat + block * 2u, block < 200u ? 0xFFFCu : 0xFFFAu);
    }
}

bool maple_vmu_persist(MapleStateV2& s) {
    std::ofstream file(s.maple_vmu_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        ++s.maple_vmu_persist_failures;
        return false;
    }
    file.write(reinterpret_cast<const char*>(s.maple_vmu_flash.data()),
               static_cast<std::streamsize>(s.maple_vmu_flash.size()));
    if (!file) {
        ++s.maple_vmu_persist_failures;
        return false;
    }
    return true;
}

void maple_vmu_ensure_initialized(MapleStateV2& s) {
    if (s.maple_vmu_initialized) return;
    s.maple_vmu_initialized = true;

    std::ifstream file(s.maple_vmu_path, std::ios::binary | std::ios::ate);
    if (file && file.tellg() == static_cast<std::streamoff>(s.maple_vmu_flash.size())) {
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(s.maple_vmu_flash.data()),
                  static_cast<std::streamsize>(s.maple_vmu_flash.size()));
        if (file) {
            std::cout << "[Maple VMU] loaded " << s.maple_vmu_path << " (128 KiB)\n";
            return;
        }
    }

    maple_vmu_format(s);
    const bool saved = maple_vmu_persist(s);
    std::cout << "[Maple VMU] created formatted A1 image " << s.maple_vmu_path
              << (saved ? "" : " (persistence failed)") << "\n";
}

void maple_write_vmu_devinfo_response(MapleStateV2& s, std::uint32_t recv,
                                      std::uint8_t request_src, std::uint8_t device_addr,
                                      bool all_info) {
    maple_vmu_ensure_initialized(s);
    maple_write_response_header(s, recv, all_info ? 6 : 5, request_src, device_addr, 28u);
    dc_write32(s, recv + 4u, kMapleFuncMemcard | kMapleFuncLcd | kMapleFuncClock);
    dc_write32(s, recv + 8u, 0x403F7E7Eu);  // clock / official VMU signature
    dc_write32(s, recv + 12u, 0x00100500u); // LCD capabilities
    dc_write32(s, recv + 16u, 0x00410F00u); // storage capabilities
    dc_write8(s, recv + 20u, 0xFFu);
    dc_write8(s, recv + 21u, 0u);
    maple_write_padded_text(s, recv + 22u, "Visual Memory", 30u);
    maple_write_padded_text(s, recv + 52u,
                            "Produced By or Under License From SEGA ENTERPRISES,LTD.", 60u);
    dc_write16(s, recv + 112u, 0x007Cu);
    dc_write16(s, recv + 114u, 0x0082u);
    ++s.maple_vmu_devinfo;
}

void maple_write_vmu_minfo_response(MapleStateV2& s, std::uint32_t recv,
                                    std::uint8_t request_src, std::uint8_t device_addr,
                                    std::uint32_t function) {
    maple_vmu_ensure_initialized(s);
    if (function != kMapleFuncMemcard) {
        maple_write_response_header(s, recv, -2, request_src, device_addr, 0u);
        return;
    }
    maple_write_response_header(s, recv, 8, request_src, device_addr, 7u);
    dc_write32(s, recv + 4u, kMapleFuncMemcard);
    const std::size_t media = 255u * kMapleVmuBlockSize + 0x40u;
    dc_load_bytes(s, recv + 8u, s.maple_vmu_flash.data() + media, 24u);
    ++s.maple_vmu_minfo;
}

std::filesystem::path maple_lcd_frame_path(const MapleStateV2& s);
bool maple_lcd_write_frame(MapleStateV2& s, std::uint32_t payload_addr, std::size_t count);
bool maple_lcd_read_frame(const MapleStateV2& s, std::array<std::uint8_t,192>& bitmap);

void maple_write_vmu_bread_response(MapleStateV2& s, std::uint32_t recv,
                                    std::uint8_t request_src, std::uint8_t device_addr,
                                    std::uint32_t function, std::uint32_t block_id) {
    maple_vmu_ensure_initialized(s);
    if (function == kMapleFuncLcd) {
        std::array<std::uint8_t, 192> bitmap{};
        // No screen frame yet: the VMU powers up with a blank LCD.
        (void)maple_lcd_read_frame(s, bitmap);
        maple_write_response_header(s, recv, 8, request_src, device_addr, 50u);
        dc_write32(s, recv + 4u, kMapleFuncLcd);
        dc_write32(s, recv + 8u, block_id);
        dc_load_bytes(s, recv + 12u, bitmap.data(), bitmap.size());
        return;
    }
    if (function != kMapleFuncMemcard) {
        maple_write_response_header(s, recv, -2, request_src, device_addr, 0u);
        return;
    }
    const std::uint32_t swapped = maple_bswap32(block_id);
    const std::uint32_t block = swapped & 0xFFFFu;
    if (block >= kMapleVmuBlockCount) {
        maple_write_response_header(s, recv, -4, request_src, device_addr, 0u);
        return;
    }
    maple_write_response_header(s, recv, 8, request_src, device_addr, 130u);
    dc_write32(s, recv + 4u, kMapleFuncMemcard);
    dc_write32(s, recv + 8u, block_id);
    dc_load_bytes(s, recv + 12u,
                  s.maple_vmu_flash.data() + block * kMapleVmuBlockSize,
                  kMapleVmuBlockSize);
    ++s.maple_vmu_reads;
}

// DCR VMU LCD side channel: runtime sends real Maple LCD frames, never save data.
// Versioned 208-byte frame: "DCRLCD01", little-endian Unix milliseconds, 192 bitmap bytes.
// Updating a separate file permits the Python Aqua launcher to watch the live display
// without sharing mutable pointers across plugin/host process boundaries.
std::filesystem::path maple_lcd_frame_path(const MapleStateV2& s) {
    auto path = std::filesystem::path(s.maple_vmu_path);
    path.replace_extension(".lcd");
    return path;
}

bool maple_lcd_write_frame(MapleStateV2& s, std::uint32_t payload_addr, std::size_t count) {
    if (count != 192u) return false;
    const auto path = maple_lcd_frame_path(s);
    const auto temp = path.string() + ".tmp";
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write("DCRLCD01", 8);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::uint64_t stamp = static_cast<std::uint64_t>(ms);
    for (unsigned i = 0; i < 8; ++i) out.put(static_cast<char>((stamp >> (8u*i)) & 0xFFu));
    for (std::size_t i = 0; i < 192u; ++i)
        out.put(static_cast<char>(dc_read8(s, payload_addr + static_cast<std::uint32_t>(i))));
    out.close();
    if (!out) { std::filesystem::remove(temp, ec); return false; }
#ifdef _WIN32
    // Windows std::filesystem::rename cannot replace an existing destination.
    if (!MoveFileExW(std::filesystem::path(temp).c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp, ec); return false;
    }
#else
    std::filesystem::rename(temp, path, ec);
    if (ec) { std::filesystem::remove(temp, ec); return false; }
#endif
    return true;
}

bool maple_lcd_read_frame(const MapleStateV2& s, std::array<std::uint8_t,192>& bitmap) {
    std::ifstream in(maple_lcd_frame_path(s), std::ios::binary);
    if (!in) return false;
    char header[16];
    in.read(header, 16);
    if (!in || std::memcmp(header, "DCRLCD01", 8) != 0) return false;
    in.read(reinterpret_cast<char*>(bitmap.data()), static_cast<std::streamsize>(bitmap.size()));
    return static_cast<bool>(in);
}

void maple_write_vmu_bwrite_response(MapleStateV2& s, std::uint32_t recv,
                                     std::uint8_t request_src, std::uint8_t device_addr,
                                     std::uint32_t function, std::uint32_t block_id,
                                     std::uint32_t payload_addr, std::size_t payload_bytes) {
    maple_vmu_ensure_initialized(s);
    if (function == kMapleFuncLcd) {
        const bool captured = maple_lcd_write_frame(s, payload_addr, payload_bytes);
        // A host-side capture failure must never cause the guest LCD transfer to fail.
        // The game sees Maple ACK for a valid 192-byte image, independent of host disk.
        (void)captured;
        maple_write_response_header(s, recv, payload_bytes == 192u ? 7 : -4,
                                    request_src, device_addr, 0u);
        return;
    }
    if (function != kMapleFuncMemcard) {
        maple_write_response_header(s, recv, -2, request_src, device_addr, 0u);
        return;
    }
    const std::uint32_t swapped = maple_bswap32(block_id);
    const std::uint32_t block = swapped & 0xFFFFu;
    const std::uint32_t phase = (swapped >> 16u) & 0xFFu;
    const std::size_t offset = static_cast<std::size_t>(block) * kMapleVmuBlockSize
                             + static_cast<std::size_t>(phase) * 128u;
    if (block >= kMapleVmuBlockCount || phase >= 4u || payload_bytes > 128u ||
        offset + payload_bytes > s.maple_vmu_flash.size()) {
        maple_write_response_header(s, recv, -4, request_src, device_addr, 0u);
        return;
    }
    for (std::size_t i = 0; i < payload_bytes; ++i)
        s.maple_vmu_flash[offset + i] = dc_read8(s, payload_addr + static_cast<std::uint32_t>(i));
    ++s.maple_vmu_writes;
    maple_write_response_header(s, recv, 7, request_src, device_addr, 0u);
}

void maple_write_vmu_bsync_response(MapleStateV2& s, std::uint32_t recv,
                                    std::uint8_t request_src, std::uint8_t device_addr,
                                    std::uint32_t function) {
    maple_vmu_ensure_initialized(s);
    if (function != kMapleFuncMemcard) {
        maple_write_response_header(s, recv, -2, request_src, device_addr, 0u);
        return;
    }
    maple_vmu_persist(s);
    ++s.maple_vmu_syncs;
    maple_write_response_header(s, recv, 7, request_src, device_addr, 0u);
}

bool maple_load_controller_profile_impl(MapleStateV2& s, const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    auto trim = [](std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string{};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1u);
    };
    auto lower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    };

    s.maple_controller_backend = "auto";
    s.maple_controller_device = 0u;
    s.maple_controller_deadzone = 0.18f;
    s.maple_controller_bindings.clear();
    // 0.1.2 controller defaults are backend-neutral. `logical:*` resolves to
    // native XInput semantics when an Xbox-compatible pad is present and to
    // the standard DualShock 4 / DirectInput layout when WinMM is used. This
    // prevents an automatic PS4 fallback from interpreting the POV hat as the
    // old XInput button indices 10-13. Explicit raw bindings still override it.
    s.maple_controller_bindings = {
        {"a", "logical:a"}, {"b", "logical:b"}, {"x", "logical:x"}, {"y", "logical:y"},
        {"start", "logical:start"}, {"up", "logical:up"}, {"down", "logical:down"},
        {"left", "logical:left"}, {"right", "logical:right"}, {"c", "logical:c"}, {"z", "logical:z"},
        {"joy_x", "logical:joy_x"}, {"joy_y", "logical:joy_y"},
        {"joy2_x", "logical:joy2_x"}, {"joy2_y", "logical:joy2_y"},
        {"ltrig", "logical:ltrig"}, {"rtrig", "logical:rtrig"}
    };

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = lower(trim(line.substr(0, eq)));
        const std::string value = trim(line.substr(eq + 1u));
        if (key == "backend") {
            const std::string backend = lower(value);
            if (backend == "auto" || backend == "xinput" || backend == "winmm" || backend == "ps4")
                s.maple_controller_backend = backend;
        } else if (key == "device") {
            try { s.maple_controller_device = std::min<std::uint32_t>(15u, static_cast<std::uint32_t>(std::stoul(value))); }
            catch (...) {}
        } else if (key == "deadzone") {
            try { s.maple_controller_deadzone = std::clamp(std::stof(value), 0.0f, 0.50f); }
            catch (...) {}
        } else if (key == "a" || key == "b" || key == "x" || key == "y" || key == "start" ||
                   key == "up" || key == "down" || key == "left" || key == "right" ||
                   key == "c" || key == "z" || key == "joy_x" || key == "joy_y" ||
                   key == "joy2_x" || key == "joy2_y" || key == "ltrig" || key == "rtrig") {
            s.maple_controller_bindings[key] = value;
        }
    }

    s.maple_controller_profile_loaded = true;
    s.maple_controller_profile_path = path;
    std::cout << "[Maple] controller-profile=" << path
              << " backend=" << s.maple_controller_backend
              << " device=" << s.maple_controller_device
              << " deadzone=" << s.maple_controller_deadzone << "\n";
    return true;
}

void maple_poll_windows_controller_profile(MapleStateV2& s) {
    const MapleHostController previous = s.maple_controller;
    MapleHostController next{};
    next.connected = true;
    std::uint16_t keyboard_buttons = 0u;
    std::uint16_t xinput_buttons = 0u;
    std::uint16_t winmm_buttons = 0u;

#ifdef _WIN32
    ++s.maple_host_polls;
    // The ABI2 plugin does not own the PVR HWND. Inspect the foreground HWND's
    // process and class directly instead of trusting its unset pvr_window_handle.
    const bool input_focused = dcr_game_window_has_focus();
    s.maple_input_focused = input_focused;
    if (!input_focused) {
        ++s.maple_unfocused_polls;
    } else {
        ++s.maple_keyboard_polls;
        const auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
        const auto key_button = [&](bool pressed, std::uint16_t mask) {
            if (!pressed) return;
            keyboard_buttons |= mask;
            next.buttons |= mask;
        };
        key_button(down('J') || down(VK_SPACE) || down('Z'), 1u << 2);
        key_button(down('K') || down('X'), 1u << 1);
        key_button(down('U') || down('C'), 1u << 10);
        key_button(down('I') || down('V'), 1u << 9);
        key_button(down(VK_RETURN), 1u << 3);
        key_button(down(VK_UP), 1u << 4);
        key_button(down(VK_DOWN), 1u << 5);
        key_button(down(VK_LEFT), 1u << 6);
        key_button(down(VK_RIGHT), 1u << 7);
        next.ltrig = down('Q') ? 255u : 0u;
        next.rtrig = down('E') ? 255u : 0u;
        next.joyx = down('A') ? -127 : down('D') ? 127 : 0;
        next.joyy = down('W') ? -127 : down('S') ? 127 : 0;

        struct PhysicalPad {
            bool connected{};
            bool xinput{};
            std::uint32_t device{};
            std::uint32_t buttons{};
            std::array<int, 6> axes{}; // LX/LY/RX/RY or X/Y/Z/R/U/V
            int lt{};
            int rt{};
            std::uint32_t pov{0xFFFFu};
        } pad;

        const auto poll_xinput = [&]() -> bool {
            using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
            static HMODULE module = nullptr;
            static XInputGetStateFn get_state = nullptr;
            static bool checked = false;
            if (!checked) {
                checked = true;
                const char* dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
                for (const char* dll : dlls) {
                    module = LoadLibraryA(dll);
                    if (!module) continue;
                    get_state = reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
                    if (get_state) break;
                    FreeLibrary(module); module = nullptr;
                }
            }
            if (!get_state) return false;
            XINPUT_STATE xs{};
            ++s.maple_xinput_polls;
            if (get_state(s.maple_controller_device, &xs) != ERROR_SUCCESS) return false;
            pad.connected = true; pad.xinput = true; pad.device = s.maple_controller_device; pad.buttons = xs.Gamepad.wButtons;
            pad.axes = {xs.Gamepad.sThumbLX, xs.Gamepad.sThumbLY, xs.Gamepad.sThumbRX,
                        xs.Gamepad.sThumbRY, 0, 0};
            pad.lt = xs.Gamepad.bLeftTrigger; pad.rt = xs.Gamepad.bRightTrigger;
            return true;
        };
        const auto poll_winmm_id = [&](std::uint32_t device) -> bool {
            JOYINFOEX ji{}; ji.dwSize = sizeof(ji); ji.dwFlags = JOY_RETURNALL;
            ++s.maple_winmm_polls;
            if (joyGetPosEx(device, &ji) != JOYERR_NOERROR) return false;
            pad.connected = true; pad.xinput = false; pad.device = device; pad.buttons = ji.dwButtons;
            pad.axes = {static_cast<int>(ji.dwXpos) - 32767, static_cast<int>(ji.dwYpos) - 32767,
                        static_cast<int>(ji.dwZpos) - 32767, static_cast<int>(ji.dwRpos) - 32767,
                        static_cast<int>(ji.dwUpos) - 32767, static_cast<int>(ji.dwVpos) - 32767};
            pad.lt = 0; pad.rt = 0; pad.pov = ji.dwPOV;
            return true;
        };
        const auto poll_winmm = [&]() -> bool {
            // Try the configured slot first, then scan the legacy joystick slots.
            // Native DualShock 4 pads commonly appear here as a DirectInput/
            // WinMM "Wireless Controller" and do not expose XInput at all.
            const std::uint32_t preferred = s.maple_controller_device;
            if (poll_winmm_id(preferred)) return true;
            const std::uint32_t count = std::min<std::uint32_t>(joyGetNumDevs(), 16u);
            for (std::uint32_t device = 0; device < count; ++device) {
                if (device == preferred) continue;
                if (poll_winmm_id(device)) return true;
            }
            return false;
        };

        const std::string backend = s.maple_controller_backend;
        bool have_pad = false;
        if (backend == "xinput") have_pad = poll_xinput();
        else if (backend == "winmm" || backend == "ps4") have_pad = poll_winmm();
        else { have_pad = poll_xinput(); if (!have_pad) have_pad = poll_winmm(); }

        if (have_pad) {
            const std::string active_backend = pad.xinput ? "xinput" : "directinput/winmm";
            if (s.maple_host_backend != active_backend || s.maple_host_device != pad.device) {
                s.maple_host_backend = active_backend;
                s.maple_host_device = pad.device;
                std::cout << "[Maple] host-controller=" << active_backend << " #" << pad.device
                          << (pad.xinput ? "" : " (native PS4/DirectInput compatible)") << "\n";
            }
            const auto lower = [](std::string value) {
                std::transform(value.begin(), value.end(), value.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return value;
            };
            const auto spec = [&](const char* name) -> std::string {
                const auto it = s.maple_controller_bindings.find(name);
                return it == s.maple_controller_bindings.end() ? std::string{} : lower(it->second);
            };
            const auto raw_axis = [&](const std::string& name) -> int {
                if (pad.xinput) {
                    if (name == "lx") return pad.axes[0]; if (name == "ly") return pad.axes[1];
                    if (name == "rx") return pad.axes[2]; if (name == "ry") return pad.axes[3];
                } else {
                    if (name == "x") return pad.axes[0]; if (name == "y") return pad.axes[1];
                    if (name == "z") return pad.axes[2]; if (name == "r") return pad.axes[3];
                    if (name == "u") return pad.axes[4]; if (name == "v") return pad.axes[5];
                }
                return 0;
            };
            const auto button_down = [&](int index) -> bool {
                if (index < 0 || index >= 32) return false;
                if (!pad.xinput) return (pad.buttons & (1u << index)) != 0u;
                static constexpr WORD masks[] = {
                    XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
                    XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                    XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB,
                    XINPUT_GAMEPAD_RIGHT_THUMB, XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
                    XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT
                };
                return index < static_cast<int>(std::size(masks)) && (pad.buttons & masks[index]) != 0u;
            };
            const int threshold = static_cast<int>(32767.0f * s.maple_controller_deadzone);
            const auto logical_button_down = [&](const std::string& name) -> bool {
                if (pad.xinput) {
                    if (name == "a") return button_down(0);
                    if (name == "b") return button_down(1);
                    if (name == "x") return button_down(2);
                    if (name == "y") return button_down(3);
                    if (name == "c") return button_down(4);
                    if (name == "z") return button_down(5);
                    if (name == "start") return button_down(7);
                    if (name == "up") return button_down(10);
                    if (name == "down") return button_down(11);
                    if (name == "left") return button_down(12);
                    if (name == "right") return button_down(13);
                    return false;
                }
                // Native DS4/DirectInput numbering is Square=0, Cross=1,
                // Circle=2, Triangle=3, L1=4, R1=5, L2=6, R2=7,
                // Share=8, Options=9, L3=10, R3=11. D-pad is the POV hat.
                if (name == "a") return button_down(1);
                if (name == "b") return button_down(2);
                if (name == "x") return button_down(0);
                if (name == "y") return button_down(3);
                if (name == "c") return button_down(4);
                if (name == "z") return button_down(5);
                if (name == "start") return button_down(9);
                if (name == "up" || name == "down" || name == "left" || name == "right") {
                    if (pad.pov == 0xFFFFu) return false;
                    const std::uint32_t p = pad.pov % 36000u;
                    if (name == "up") return p >= 31500u || p <= 4500u;
                    if (name == "right") return p >= 4500u && p <= 13500u;
                    if (name == "down") return p >= 13500u && p <= 22500u;
                    if (name == "left") return p >= 22500u && p <= 31500u;
                }
                return false;
            };
            const auto logical_axis_value = [&](const std::string& name) -> int {
                if (pad.xinput) {
                    if (name == "joy_x") return pad.axes[0];
                    if (name == "joy_y") return -pad.axes[1];
                    if (name == "joy2_x") return pad.axes[2];
                    if (name == "joy2_y") return -pad.axes[3];
                } else {
                    // WinMM exposes the DS4 left stick as X/Y and right stick
                    // as Z/R. Its Y axes already grow toward screen-down.
                    if (name == "joy_x") return pad.axes[0];
                    if (name == "joy_y") return pad.axes[1];
                    if (name == "joy2_x") return pad.axes[2];
                    if (name == "joy2_y") return pad.axes[3];
                }
                return 0;
            };
            const auto logical_trigger_value = [&](const std::string& name) -> int {
                if (pad.xinput) return name == "ltrig" ? pad.lt : name == "rtrig" ? pad.rt : 0;
                // Do not assume WinMM U/V are independent triggers. Native DS4
                // drivers may expose a combined trigger axis (or reuse U/V for
                // unrelated controls), which caused L2/R2 cross-talk in 0.1.2.
                // The independent L2/R2 button bits are the safe default. Users
                // can retain analog pressure with a learned range:<axis>:<rest>:<full> binding.
                const int button = name == "ltrig" ? 6 : name == "rtrig" ? 7 : -1;
                return button >= 0 && button_down(button) ? 255 : 0;
            };
            const auto axis_value = [&](const std::string& binding) -> int {
                if (binding.rfind("logical:", 0) == 0u) {
                    int value = logical_axis_value(binding.substr(8u));
                    if (std::abs(value) <= threshold) return 0;
                    return value;
                }
                if (binding.rfind("axis:", 0) != 0u) return 0;
                std::string rest = binding.substr(5u);
                bool invert = false;
                const auto inv = rest.find(":invert");
                if (inv != std::string::npos) { invert = true; rest.erase(inv); }
                if (!rest.empty() && (rest.back() == '+' || rest.back() == '-')) {
                    if (rest.back() == '-') invert = !invert;
                    rest.pop_back();
                }
                int value = raw_axis(rest);
                if (std::abs(value) <= threshold) return 0;
                if (invert) value = -value;
                return value;
            };
            const auto pov_down = [&](const std::string& dir) -> bool {
                if (pad.xinput || pad.pov == 0xFFFFu) return false;
                const std::uint32_t p = pad.pov % 36000u;
                if (dir == "up") return p >= 31500u || p <= 4500u;
                if (dir == "right") return p >= 4500u && p <= 13500u;
                if (dir == "down") return p >= 13500u && p <= 22500u;
                if (dir == "left") return p >= 22500u && p <= 31500u;
                return false;
            };
            const auto pressed = [&](const std::string& binding) -> bool {
                if (binding.rfind("logical:", 0) == 0u)
                    return logical_button_down(binding.substr(8u));
                if (binding.rfind("button:", 0) == 0u) {
                    try { return button_down(std::stoi(binding.substr(7u))); } catch (...) { return false; }
                }
                if (binding == "trigger:lt") return pad.lt > 80;
                if (binding == "trigger:rt") return pad.rt > 80;
                if (binding.rfind("pov:", 0) == 0u) return pov_down(binding.substr(4u));
                if (binding.rfind("axis:", 0) == 0u) {
                    std::string rest = binding.substr(5u);
                    const auto colon = rest.find(':'); if (colon != std::string::npos) rest.resize(colon);
                    char direction = 0;
                    if (!rest.empty() && (rest.back() == '+' || rest.back() == '-')) { direction = rest.back(); rest.pop_back(); }
                    const int value = raw_axis(rest);
                    if (direction == '+') return value > threshold;
                    if (direction == '-') return value < -threshold;
                    return std::abs(value) > threshold;
                }
                return false;
            };
            const auto analog = [&](const std::string& binding) -> std::int8_t {
                int value = axis_value(binding);
                if (value == 0) return 0;
                return static_cast<std::int8_t>(std::clamp(value / 256, -128, 127));
            };
            const auto trigger = [&](const std::string& binding) -> std::uint8_t {
                if (binding.rfind("logical:", 0) == 0u)
                    return static_cast<std::uint8_t>(logical_trigger_value(binding.substr(8u)));
                if (binding == "trigger:lt") return static_cast<std::uint8_t>(std::clamp(pad.lt, 0, 255));
                if (binding == "trigger:rt") return static_cast<std::uint8_t>(std::clamp(pad.rt, 0, 255));
                if (binding.rfind("range:", 0) == 0u) {
                    // Calibrated WinMM analog trigger: range:<axis>:<rest>:<full>.
                    // This supports separate axes and a single combined L2/R2 axis.
                    const std::string payload = binding.substr(6u);
                    const auto c1 = payload.find(':');
                    const auto c2 = c1 == std::string::npos ? std::string::npos : payload.find(':', c1 + 1u);
                    if (c1 != std::string::npos && c2 != std::string::npos) {
                        try {
                            const std::string axis = payload.substr(0u, c1);
                            const int rest = std::stoi(payload.substr(c1 + 1u, c2 - c1 - 1u));
                            const int full = std::stoi(payload.substr(c2 + 1u));
                            const int span = full - rest;
                            if (span != 0) {
                                const double t = static_cast<double>(raw_axis(axis) - rest) / static_cast<double>(span);
                                return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(t * 255.0)), 0, 255));
                            }
                        } catch (...) {}
                    }
                    return 0u;
                }
                if (binding.rfind("axis:", 0) == 0u) {
                    const int value = std::abs(axis_value(binding));
                    return static_cast<std::uint8_t>(std::clamp(value / 128, 0, 255));
                }
                return pressed(binding) ? 255u : 0u;
            };
            const auto map_button = [&](const char* name, std::uint16_t mask) {
                if (!pressed(spec(name))) return;
                next.buttons |= mask;
                if (pad.xinput) xinput_buttons |= mask; else winmm_buttons |= mask;
            };
            map_button("a", 1u << 2); map_button("b", 1u << 1);
            map_button("x", 1u << 10); map_button("y", 1u << 9);
            map_button("start", 1u << 3); map_button("up", 1u << 4);
            map_button("down", 1u << 5); map_button("left", 1u << 6);
            map_button("right", 1u << 7); map_button("c", 1u << 0); map_button("z", 1u << 8);
            next.ltrig = std::max(next.ltrig, trigger(spec("ltrig")));
            next.rtrig = std::max(next.rtrig, trigger(spec("rtrig")));
            next.joyx = analog(spec("joy_x")); next.joyy = analog(spec("joy_y"));
            next.joy2x = analog(spec("joy2_x")); next.joy2y = analog(spec("joy2_y"));
        }
    }
#else
    ++s.maple_host_polls;
#endif
    s.maple_keyboard_buttons = keyboard_buttons;
    s.maple_xinput_buttons = xinput_buttons;
    s.maple_winmm_buttons = winmm_buttons;
    s.maple_buttons_seen_interval |= static_cast<std::uint16_t>(next.buttons & 0xFFFFu);
    if (previous.buttons != next.buttons || previous.ltrig != next.ltrig || previous.rtrig != next.rtrig ||
        previous.joyx != next.joyx || previous.joyy != next.joyy ||
        previous.joy2x != next.joy2x || previous.joy2y != next.joy2y)
        ++s.maple_input_changes;
    s.maple_controller = next;
}

void maple_poll_windows_controller(MapleStateV2& s) {
    if (!s.maple_controller_present) {
        s.maple_controller = {};
        s.maple_controller.connected = false;
        return;
    }

    // Use the same backend-neutral mapping path even when the user has not
    // created a profile. This keeps XInput behavior intact while allowing a
    // native DualShock 4 (DirectInput/WinMM) to work without DS4Windows.
    if (!s.maple_controller_profile_loaded && s.maple_controller_bindings.empty()) {
        s.maple_controller_backend = "auto";
        s.maple_controller_device = 0u;
        s.maple_controller_deadzone = 0.18f;
        s.maple_controller_bindings = {
            {"a", "logical:a"}, {"b", "logical:b"}, {"x", "logical:x"}, {"y", "logical:y"},
            {"start", "logical:start"}, {"up", "logical:up"}, {"down", "logical:down"},
            {"left", "logical:left"}, {"right", "logical:right"}, {"c", "logical:c"}, {"z", "logical:z"},
            {"joy_x", "logical:joy_x"}, {"joy_y", "logical:joy_y"},
            {"joy2_x", "logical:joy2_x"}, {"joy2_y", "logical:joy2_y"},
            {"ltrig", "logical:ltrig"}, {"rtrig", "logical:rtrig"}
        };
    }
    maple_poll_windows_controller_profile(s);
}


void maple_poll_windows_controller_port(MapleStateV2& s, std::uint32_t port) {
    if (port >= s.maple_ports.size()) return;
    auto& ps=s.maple_ports[port];
    const std::string spec=lower_copy(ps.input_spec);
#ifdef _WIN32
    // The unified P1-P4 selector uses this path, including keyboard + mouse.
    // Clear all active buttons/axes/triggers even if a controller remains
    // connected. Never sample GetAsyncKeyState, XInput or WinMM in background.
    // Reset mouse baseline so refocusing cannot turn a desktop cursor move
    // into a large in-game camera/analog movement.
    const bool input_focused = dcr_game_window_has_focus();
    if (port == 0u && s.maple_input_focused != input_focused) {
        std::cout << "[MAPLE FOCUS] " << (input_focused ? "active" : "inactive")
                  << " | input=" << (input_focused ? "enabled" : "neutral A-D") << "\n";
    }
    s.maple_input_focused = input_focused;
    if (!input_focused) {
        ++s.maple_unfocused_polls;
        MapleHostController neutral{};
        neutral.connected = ps.controller_present;
        ps.controller = neutral;
        ps.mouse_initialized = false;
        if (port == 0u) {
            s.maple_controller = neutral;
            s.maple_controller_present = ps.controller_present;
        }
        return;
    }
#endif
    if (port==0u && (spec.empty() || spec=="auto")) { maple_poll_windows_controller(s); maple_sync_port_a_tree(s); ps.input_spec="auto"; return; }
#ifdef _WIN32
    MapleHostController next{}; next.connected=false; bool found=false;
    if (spec=="none" || spec=="disabled") { ps.controller=next; ps.controller_present=false; ps.host_backend="none"; return; }
    if (spec=="keyboard_mouse" || spec=="keyboard+mouse" || spec=="kbm") {
        found=true; next.connected=true; ps.host_backend="keyboard+mouse"; ps.host_device=0;
        const auto down=[](int vk){return (GetAsyncKeyState(vk)&0x8000)!=0;};
        if (down('J')||down(VK_SPACE)||down('Z')||down(VK_LBUTTON)) next.buttons|=1u<<2;
        if (down('K')||down('X')||down(VK_RBUTTON)) next.buttons|=1u<<1;
        if (down('U')||down('C')||down(VK_MBUTTON)) next.buttons|=1u<<10;
        if (down('I')||down('V')||down(VK_XBUTTON1)) next.buttons|=1u<<9;
        if (down(VK_RETURN)||down(VK_XBUTTON2)) next.buttons|=1u<<3;
        if (down(VK_UP)) next.buttons|=1u<<4; if (down(VK_DOWN)) next.buttons|=1u<<5; if (down(VK_LEFT)) next.buttons|=1u<<6; if (down(VK_RIGHT)) next.buttons|=1u<<7;
        next.ltrig=down('Q')?255u:0u; next.rtrig=down('E')?255u:0u;
        next.joyx=down('A')?-127:down('D')?127:0; next.joyy=down('W')?-127:down('S')?127:0;
        POINT pt{};
        if (GetCursorPos(&pt)) {
            const int mouse_x = static_cast<int>(pt.x);
            const int mouse_y = static_cast<int>(pt.y);
            if (ps.mouse_initialized) {
                const int dx = (mouse_x - ps.mouse_x) * 5;
                const int dy = (mouse_y - ps.mouse_y) * 5;
                next.joy2x = static_cast<std::int8_t>(std::clamp(dx, -127, 127));
                next.joy2y = static_cast<std::int8_t>(std::clamp(dy, -127, 127));
            }
            ps.mouse_x = mouse_x;
            ps.mouse_y = mouse_y;
            ps.mouse_initialized = true;
        }
        ++s.maple_keyboard_polls;
    } else {
        std::string kind=spec; std::uint32_t device=port; const auto colon=kind.find(':');
        if (colon!=std::string::npos) { try { device=static_cast<std::uint32_t>(std::stoul(kind.substr(colon+1u))); } catch (...) {} kind.resize(colon); }
        const bool allow_x=kind=="xinput" || kind=="auto"; const bool allow_w=kind=="winmm" || kind=="directinput" || kind=="ps4" || kind=="auto";
        using XInputGetStateFn=DWORD (WINAPI*)(DWORD,XINPUT_STATE*); static HMODULE xm=nullptr; static XInputGetStateFn xget=nullptr; static bool xc=false;
        if (!xc) { xc=true; for(const char* dll:{"xinput1_4.dll","xinput1_3.dll","xinput9_1_0.dll"}) { xm=LoadLibraryA(dll); if(!xm)continue; xget=reinterpret_cast<XInputGetStateFn>(GetProcAddress(xm,"XInputGetState")); if(xget)break; FreeLibrary(xm); xm=nullptr; } }
        if (allow_x && xget) { XINPUT_STATE xs{}; ++s.maple_xinput_polls; if(xget(device,&xs)==ERROR_SUCCESS) { found=true; next.connected=true; ps.host_device=device; ps.host_backend="xinput"; const WORD b=xs.Gamepad.wButtons;
            if(b&XINPUT_GAMEPAD_A)next.buttons|=1u<<2; if(b&XINPUT_GAMEPAD_B)next.buttons|=1u<<1; if(b&XINPUT_GAMEPAD_X)next.buttons|=1u<<10; if(b&XINPUT_GAMEPAD_Y)next.buttons|=1u<<9; if(b&XINPUT_GAMEPAD_START)next.buttons|=1u<<3;
            if(b&XINPUT_GAMEPAD_DPAD_UP)next.buttons|=1u<<4; if(b&XINPUT_GAMEPAD_DPAD_DOWN)next.buttons|=1u<<5; if(b&XINPUT_GAMEPAD_DPAD_LEFT)next.buttons|=1u<<6; if(b&XINPUT_GAMEPAD_DPAD_RIGHT)next.buttons|=1u<<7; if(b&XINPUT_GAMEPAD_LEFT_SHOULDER)next.buttons|=1u<<0; if(b&XINPUT_GAMEPAD_RIGHT_SHOULDER)next.buttons|=1u<<8;
            next.ltrig=xs.Gamepad.bLeftTrigger; next.rtrig=xs.Gamepad.bRightTrigger; auto axis=[](SHORT v,bool inv=false){int x=inv?-static_cast<int>(v):static_cast<int>(v); if(std::abs(x)<5898)return(std::int8_t)0; return static_cast<std::int8_t>(std::clamp(x/256,-128,127));};
            next.joyx=axis(xs.Gamepad.sThumbLX); next.joyy=axis(xs.Gamepad.sThumbLY,true); next.joy2x=axis(xs.Gamepad.sThumbRX); next.joy2y=axis(xs.Gamepad.sThumbRY,true); } }
        if (!found && allow_w) { JOYINFOEX ji{}; ji.dwSize=sizeof(ji); ji.dwFlags=JOY_RETURNALL; ++s.maple_winmm_polls; if(joyGetPosEx(device,&ji)==JOYERR_NOERROR) { found=true; next.connected=true; ps.host_device=device; ps.host_backend="directinput/winmm"; auto bd=[&](int i){return i>=0&&i<32&&(ji.dwButtons&(1u<<i))!=0u;};
            if(bd(1))next.buttons|=1u<<2; if(bd(2))next.buttons|=1u<<1; if(bd(0))next.buttons|=1u<<10; if(bd(3))next.buttons|=1u<<9; if(bd(9))next.buttons|=1u<<3; if(bd(4))next.buttons|=1u<<0; if(bd(5))next.buttons|=1u<<8;
            if(ji.dwPOV!=0xFFFFu){auto pov=ji.dwPOV%36000u;if(pov>=31500u||pov<=4500u)next.buttons|=1u<<4;if(pov>=4500u&&pov<=13500u)next.buttons|=1u<<7;if(pov>=13500u&&pov<=22500u)next.buttons|=1u<<5;if(pov>=22500u&&pov<=31500u)next.buttons|=1u<<6;}
            next.ltrig=bd(6)?255u:0u; next.rtrig=bd(7)?255u:0u; auto ax=[](DWORD v){int x=static_cast<int>(v)-32767;if(std::abs(x)<5898)return(std::int8_t)0;return static_cast<std::int8_t>(std::clamp(x/256,-128,127));}; next.joyx=ax(ji.dwXpos); next.joyy=ax(ji.dwYpos); next.joy2x=ax(ji.dwZpos); next.joy2y=ax(ji.dwRpos); } }
    }
    ps.controller=next; ps.controller_present=found;
    if (port==0u) { s.maple_controller=next; s.maple_controller_present=found; s.maple_host_backend=ps.host_backend; s.maple_host_device=ps.host_device; }
    if (found && s.maple_trace) std::cout<<"[MAPLE TRACE] Port "<<static_cast<char>('A'+port)<<" input="<<spec<<" host="<<ps.host_backend<<" #"<<ps.host_device<<"\n";
#else
    (void)s; (void)port;
#endif
}

void maple_process_dma(MapleStateV2& s) {
    if ((s.maple_enable & 1u) == 0u || s.maple_dma_address == 0u) {
        s.maple_state = 0u;
        s.mmio32[kMapleStatePhysical] = 0u;
        return;
    }

    ++s.maple_dma_runs;
    std::uint32_t cursor = s.maple_dma_address & 0x1FFFFFFFu;
    // A Maple DMA list is a chain of descriptors. Pattern 0 (START) is the
    // only format followed by a receive pointer and a Maple request packet.
    // Patterns 2/3/4/7 are one-word control descriptors. Treating a NOP as a
    // START makes the parser consume the following descriptor words as packet
    // fields and eventually walk arbitrary RAM -- exactly what happened in
    // ChuChu Rocket's Katana-generated lists.
    for (unsigned descriptor_index = 0; descriptor_index < 256u; ++descriptor_index) {
        const std::uint32_t desc0 = dc_read32(s, cursor);
        const bool last = (desc0 & 0x80000000u) != 0u;
        const std::uint32_t pattern = (desc0 >> 8u) & 0x7u;

        if (pattern != 0u) {
            if (pattern == 7u) ++s.maple_desc_nop;
            else ++s.maple_desc_other;

            // RESET (3), SDCKB occupy/cancel (2/4) and NOP (7) are all
            // single-word descriptors. Their detailed peripheral side effects
            // are not needed by ChuChu, but their layout absolutely matters.
            cursor += 4u;
            if (last) break;
            continue;
        }

        ++s.maple_desc_start;
        const std::uint32_t send_words = desc0 & 0xFFu;
        const std::uint32_t recv_raw = dc_read32(s, cursor + 4u);
        // Maple response buffers are 32-byte aligned bus addresses. Retail SDK
        // code can submit only the low 24 bits; real Maple DMA targets SH-4
        // area-3 RAM in that case. Normalize exactly at the DMA boundary rather
        // than mapping low boot-ROM addresses as writable memory.
        std::uint32_t recv = recv_raw & 0x1FFFFFE0u;
        if (recv < 0x0C000000u || recv >= 0x10000000u) {
            recv = 0x0C000000u | (recv & 0x00FFFFE0u);
            ++s.maple_recv_fixups;
        }
        const std::uint32_t request = dc_read32(s, cursor + 8u);
        const std::uint8_t cmd = static_cast<std::uint8_t>(request);
        const std::uint8_t dst = static_cast<std::uint8_t>(request >> 8);
        const std::uint8_t src = static_cast<std::uint8_t>(request >> 16);
        const int port = (dst >> 6) & 3;
        int unit = -1;
        if (dst & 0x20u) unit = 0;
        else {
            for (int u = 1; u < 6; ++u) if (dst & (1u << (u - 1))) { unit = u; break; }
        }
        ++s.maple_dma_frames;
        s.maple_last_cmd = cmd;
        s.maple_last_dst = dst;
        s.maple_last_src = src;
        s.maple_last_unit = static_cast<std::int8_t>(unit);
        s.maple_last_function = send_words ? dc_read32(s, cursor + 12u) : 0u;

        if (unit == 0 && s.maple_host_input) maple_poll_windows_controller_port(s, static_cast<std::uint32_t>(port));
        const MapleDeviceKind device_kind = maple_device_kind(s, static_cast<std::uint32_t>(port), unit);
        const bool controller = device_kind == MapleDeviceKind::Controller;
        const bool vmu = device_kind == MapleDeviceKind::Vmu;
        const bool purupuru = device_kind == MapleDeviceKind::PuruPuru;
        if (!controller) ++s.maple_noncontroller_frames;
        maple_trace_line(s, cmd, dst, src, port, unit, send_words, device_kind);

        // A main Maple device advertises the attached expansion units in its
        // source address. cpp36a derives these bits from the device tree.
        const std::uint8_t controller_addr = static_cast<std::uint8_t>(
            dst | maple_expansion_mask(s, static_cast<std::uint32_t>(port)));

        if (purupuru) {
            maple_process_purupuru(s, recv, cursor, send_words, cmd, src, dst, static_cast<std::uint32_t>(port), static_cast<std::uint32_t>(unit-1));
        } else if (vmu) {
            const std::uint32_t function = send_words ? dc_read32(s, cursor + 12u) : 0u;
            if (cmd == 1u || cmd == 2u) {
                maple_write_vmu_devinfo_response(s, recv, src, dst, cmd == 2u);
            } else if (cmd == 10u) {
                maple_write_vmu_minfo_response(s, recv, src, dst, function);
            } else if (cmd == 11u) {
                const std::uint32_t block_id = send_words >= 2u ? dc_read32(s, cursor + 16u) : 0u;
                maple_write_vmu_bread_response(s, recv, src, dst, function, block_id);
            } else if (cmd == 12u) {
                const std::uint32_t block_id = send_words >= 2u ? dc_read32(s, cursor + 16u) : 0u;
                const std::size_t payload_bytes = send_words > 2u ? static_cast<std::size_t>(send_words - 2u) * 4u : 0u;
                maple_write_vmu_bwrite_response(s, recv, src, dst, function, block_id,
                                                cursor + 20u, payload_bytes);
            } else if (cmd == 13u) {
                maple_write_vmu_bsync_response(s, recv, src, dst, function);
            } else if (cmd == 3u || cmd == 4u) {
                maple_write_response_header(s, recv, 7, src, dst, 0u);
            } else {
                maple_write_response_header(s, recv, -3, src, dst, 0u);
            }
        } else if (!controller) {
            maple_write_response_header(s, recv, -1, src, dst, 0u);
        } else if (cmd == 1u || cmd == 2u) {
            maple_write_devinfo_response(s, recv, src, controller_addr);
        } else if (cmd == 9u) {
            const std::uint32_t function = send_words ? dc_read32(s, cursor + 12u) : 0u;
            if (function == kMapleFuncController)
                maple_write_getcond_response(s, recv, src, controller_addr, static_cast<std::uint32_t>(port));
            else
                maple_write_response_header(s, recv, -2, src, controller_addr, 0u);
        } else if (cmd == 3u || cmd == 4u) {
            maple_write_response_header(s, recv, 7, src, controller_addr, 0u);
        } else {
            maple_write_response_header(s, recv, -3, src, controller_addr, 0u);
        }

        cursor += (3u + send_words) * 4u;
        if (last) break;
    }

    // The payload transfer is still synchronous, but completion now reaches
    // Holly's normal event-pending register. ASIC_EVT_MAPLE_DMA is ACK_A bit 12;
    // software can observe/acknowledge it exactly like the real System ASIC.
    s.maple_state = 0u;
    s.mmio32[kMapleStatePhysical] = 0u;
    s.holly_pending[0] |= kAsicEventMapleDmaBit;
    if (s.host.raise_irq) s.host.raise_irq(s.host.host_context, 0u, kAsicEventMapleDmaBit);
    ++s.maple_dma_irqs;
}

bool maple_builtin_read32_impl(MapleStateV2& s, std::uint32_t address, std::uint32_t& value) {
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    if (physical == kMapleDmaAddrPhysical) { value = s.maple_dma_address; return true; }
    if (physical == kMapleEnablePhysical) { value = s.maple_enable; return true; }
    if (physical == kMapleStatePhysical) { value = s.maple_state; return true; }
    if (physical == kMapleDmaTselPhysical || physical == kMapleSpeedPhysical || physical == kMapleDmaProtPhysical) {
        const auto it = s.mmio32.find(physical);
        value = it == s.mmio32.end() ? 0u : it->second;
        return true;
    }
    return false;
}

bool maple_builtin_write32_impl(MapleStateV2& s, std::uint32_t address, std::uint32_t value) {
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    if (physical == kMapleDmaAddrPhysical) {
        s.maple_dma_address = value & 0x1FFFFFFFu;
        s.mmio32[physical] = s.maple_dma_address;
        return true;
    }
    if (physical == kMapleEnablePhysical) {
        s.maple_enable = value & 1u;
        s.mmio32[physical] = s.maple_enable;
        return true;
    }
    if (physical == kMapleDmaTselPhysical || physical == kMapleSpeedPhysical || physical == kMapleDmaProtPhysical) {
        s.mmio32[physical] = value;
        return true;
    }
    if (physical == kMapleStatePhysical) {
        s.maple_state = value & 1u;
        s.mmio32[physical] = s.maple_state;
        if (s.maple_state != 0u) maple_process_dma(s);
        return true;
    }
    return false;
}



void set_vmu_path_from_host(MapleStateV2& s) {
    const char* root = s.host.get_config ? s.host.get_config(s.host.host_context, "DCR_SYSTEM_DATA_DIR") : nullptr;
    if (!root || !*root) root = s.host.get_config ? s.host.get_config(s.host.host_context, "DCR_SYSTEM_DATA_ROOT") : nullptr;
    if (root && *root) {
        std::error_code ec;
        std::filesystem::path dir(root);
        std::filesystem::create_directories(dir, ec);
        s.maple_vmu_path = (dir / "dreamcast_vmu_a1.bin").string();
    }
}

bool ctl_load_profile(void* p, const char* path) {
    return p && path && maple_load_controller_profile_impl(*static_cast<MapleStateV2*>(p), path);
}
void ctl_enable_input(void* p, bool enabled) {
    if (!p) return; auto& s=*static_cast<MapleStateV2*>(p); s.maple_host_input=enabled; if (enabled) for (std::uint32_t port=0;port<4u;++port) maple_poll_windows_controller_port(s,port);
}
void ctl_set_present(void* p, bool present) {
    if (!p) return; auto& s=*static_cast<MapleStateV2*>(p); maple_set_port_controller_present(s,0u,present); maple_sync_port_a_tree(s);
}
void ctl_set_state(void* p, const DCRMapleControllerStateV2* st) {
    if (!p || !st) return; auto& s=*static_cast<MapleStateV2*>(p);
    s.maple_controller_present=st->present; s.maple_controller.connected=st->connected;
    s.maple_controller.buttons=st->buttons; s.maple_controller.ltrig=st->ltrig; s.maple_controller.rtrig=st->rtrig;
    s.maple_controller.joyx=st->joyx; s.maple_controller.joyy=st->joyy; s.maple_controller.joy2x=st->joy2x; s.maple_controller.joy2y=st->joy2y; maple_sync_port_a_tree(s);
}
void ctl_poll(void* p) { if (!p) return; auto& s=*static_cast<MapleStateV2*>(p); if (s.maple_host_input) for (std::uint32_t port=0;port<4u;++port) maple_poll_windows_controller_port(s,port); }
bool ctl_get_state(void* p, DCRMapleControllerStateV2* st) {
    if (!p || !st || st->struct_size < sizeof(DCRMapleControllerStateV2)) return false;
    auto& s=*static_cast<MapleStateV2*>(p); const auto& c=s.maple_controller;
    st->present=s.maple_controller_present; st->connected=c.connected; st->buttons=static_cast<std::uint16_t>(c.buttons & 0xffffu);
    st->ltrig=c.ltrig; st->rtrig=c.rtrig; st->joyx=c.joyx; st->joyy=c.joyy; st->joy2x=c.joy2x; st->joy2y=c.joy2y; return true;
}
void ctl_print_stats(void* p) {
    if (!p) return; auto& s=*static_cast<MapleStateV2*>(p);
    std::cout << "[Maple V2] private-state=yes present=" << (s.maple_controller_present?"yes":"no")
              << " host=" << s.maple_host_backend << "/" << s.maple_host_device
              << " dma=" << s.maple_dma_runs << "/" << s.maple_dma_frames
              << " vmu=A1/" << s.maple_vmu_devinfo << "/" << s.maple_vmu_minfo << "/" << s.maple_vmu_reads << "/" << s.maple_vmu_writes << "/" << s.maple_vmu_syncs
              << " path=" << s.maple_vmu_path << "\n";
}
const DCRMapleControlV2 kMapleControl{
    DCR_MAPLE_CONTROL_VERSION_V2, sizeof(DCRMapleControlV2), &ctl_load_profile, &ctl_enable_input, &ctl_set_present, &ctl_set_state, &ctl_poll, &ctl_get_state, &ctl_print_stats
};

void ctl3_trace(void* p, bool enabled) { if (p) static_cast<MapleStateV2*>(p)->maple_trace=enabled; }
bool ctl3_trace_enabled(void* p) { return p && static_cast<MapleStateV2*>(p)->maple_trace; }
bool ctl3_set_port(void* p, std::uint32_t port, const DCRMapleControllerStateV2* st) {
    if (!p || !st || port>=4u) return false; auto& s=*static_cast<MapleStateV2*>(p);
    auto& c=maple_port_controller(s,port); c.connected=st->connected; c.buttons=st->buttons; c.ltrig=st->ltrig; c.rtrig=st->rtrig; c.joyx=st->joyx; c.joyy=st->joyy; c.joy2x=st->joy2x; c.joy2y=st->joy2y;
    maple_set_port_controller_present(s,port,st->present); if (port==0u) maple_sync_port_a_tree(s); return true;
}
bool ctl3_get_port(void* p, std::uint32_t port, DCRMapleControllerStateV2* st) {
    if (!p || !st || port>=4u || st->struct_size<sizeof(DCRMapleControllerStateV2)) return false; auto& s=*static_cast<MapleStateV2*>(p); if (s.maple_host_input) maple_poll_windows_controller_port(s,port); const auto& c=maple_port_controller(s,port);
    st->present=maple_port_controller_present(s,port); st->connected=c.connected; st->buttons=static_cast<std::uint16_t>(c.buttons); st->ltrig=c.ltrig; st->rtrig=c.rtrig; st->joyx=c.joyx; st->joyy=c.joyy; st->joy2x=c.joy2x; st->joy2y=c.joy2y; return true;
}
bool ctl3_set_expansion(void* p, std::uint32_t port, std::uint32_t slot, std::uint32_t kind) {
    if (!p || port>=4u || slot<1u || slot>2u || kind>DCR_MAPLE_DEVICE_PURUPURU) return false; auto& s=*static_cast<MapleStateV2*>(p);
    if (kind==DCR_MAPLE_DEVICE_VMU && !(port==0u && slot==1u)) return false; // cpp36a storage implementation is A1; tree supports all slots.
    if (s.maple_ports[port].expansions[slot-1u]==MapleDeviceKind::PuruPuru && kind!=DCR_MAPLE_DEVICE_PURUPURU) maple_stop_rumble(s,port,slot-1u);
    s.maple_ports[port].expansions[slot-1u]=static_cast<MapleDeviceKind>(kind); if (port==0u && slot==1u) s.maple_vmu_present=kind==DCR_MAPLE_DEVICE_VMU; return true;
}
std::uint32_t ctl3_get_expansion(void* p, std::uint32_t port, std::uint32_t slot) { if(!p||port>=4u||slot<1u||slot>2u) return DCR_MAPLE_DEVICE_EMPTY; return static_cast<std::uint32_t>(static_cast<MapleStateV2*>(p)->maple_ports[port].expansions[slot-1u]); }
bool ctl3_get_port_state(void* p, std::uint32_t port, DCRMaplePortStateV3* out) {
    if (!p || !out || port>=4u || out->struct_size<sizeof(DCRMaplePortStateV3)) return false; auto& s=*static_cast<MapleStateV2*>(p); DCRMapleControllerStateV2 c{}; c.struct_size=sizeof(c); if(!ctl3_get_port(p,port,&c)) return false; out->port=port; out->controller=c; out->slot1_kind=static_cast<std::uint32_t>(s.maple_ports[port].expansions[0]); out->slot2_kind=static_cast<std::uint32_t>(s.maple_ports[port].expansions[1]); out->host_device=s.maple_ports[port].host_device; return true;
}
void ctl3_print_tree(void* p) { if(!p)return; auto& s=*static_cast<MapleStateV2*>(p); for(std::uint32_t port=0;port<4u;++port){ std::cout<<"[Maple Tree] Port "<<static_cast<char>('A'+port)<<" controller="<<(maple_port_controller_present(s,port)?"yes":"no")<<" slot1="<<maple_kind_name(s.maple_ports[port].expansions[0])<<" slot2="<<maple_kind_name(s.maple_ports[port].expansions[1])<<" host="<<s.maple_ports[port].host_backend<<"/"<<s.maple_ports[port].host_device<<"\n"; } }
const DCRMapleControlV3 kMapleControlV3{ DCR_MAPLE_CONTROL_VERSION_V3,sizeof(DCRMapleControlV3),&ctl3_trace,&ctl3_trace_enabled,&ctl3_set_port,&ctl3_get_port,&ctl3_set_expansion,&ctl3_get_expansion,&ctl3_get_port_state,&ctl3_print_tree };

void* maple_create(const DCRHostServicesV2* host) {
    if (!host || host->abi_version != DCR_HOST_SERVICES_ABI_V2 || host->struct_size < sizeof(DCRHostServicesV2) ||
        !host->read32 || !host->write32 || !host->raise_irq) return nullptr;
    auto* s=new(std::nothrow) MapleStateV2{}; if(!s) return nullptr; s->host=*host; set_vmu_path_from_host(*s);
    for (std::uint32_t port=0;port<4u;++port) { s->maple_ports[port].host_device=port; s->maple_ports[port].controller.connected=false; }
    maple_load_port_config(*s);
    for (std::uint32_t port=0; port<4u; ++port) {
        const bool configured = lower_copy(s->maple_ports[port].input_spec) != "none" && lower_copy(s->maple_ports[port].input_spec) != "disabled";
        s->maple_ports[port].controller_present = configured;
        s->maple_ports[port].controller.connected = configured;
    }
    s->maple_controller_present = s->maple_ports[0].controller_present;
    s->maple_controller.connected = s->maple_controller_present;
    s->maple_trace = config_truthy(s->host.get_config ? s->host.get_config(s->host.host_context,"DCR_MAPLE_TRACE") : nullptr);
    maple_sync_port_a_tree(*s);
    if (s->host.log) s->host.log(s->host.host_context,1u,"maple","cpp36b1g Maple + PuruPuru + XInput/DS4/DualSense haptics active");
    if (s->maple_trace) ctl3_print_tree(s);
    return s;
}
void maple_destroy(void* p) { if (!p) return; auto* s=static_cast<MapleStateV2*>(p); if (s->maple_vmu_initialized) maple_vmu_persist(*s); delete s; }
void maple_reset(void* p) { if (!p) return; auto& s=*static_cast<MapleStateV2*>(p); s.maple_dma_address=0; s.maple_enable=0; s.maple_state=0; s.mmio32.clear(); }
void maple_tick(void* p, std::uint64_t cycles) {
    if (!p) return; auto& s=*static_cast<MapleStateV2*>(p); s.ticks += cycles; const auto now=monotonic_ms();
    for(std::uint32_t port=0;port<4u;++port) for(std::uint32_t slot=0;slot<2u;++slot) { auto& r=s.maple_rumble[port][slot]; if(r.stop_at_ms && now>=r.stop_at_ms) maple_stop_rumble(s,port,slot); }
}
bool maple_handles(void*, std::uint32_t address) { const auto p=address&0x1fffffffu; return p>=kMapleBasePhysical && p<kMapleBasePhysical+0x90u; }
bool maple_read8(void*, std::uint32_t, std::uint8_t*) { return false; }
bool maple_write8(void*, std::uint32_t, std::uint8_t) { return false; }
bool maple_read32(void* p, std::uint32_t a, std::uint32_t* v) { return p && v && maple_builtin_read32_impl(*static_cast<MapleStateV2*>(p),a,*v); }
bool maple_write32(void* p, std::uint32_t a, std::uint32_t v) { return p && maple_builtin_write32_impl(*static_cast<MapleStateV2*>(p),a,v); }
const void* maple_query_interface(void*, const char* name, std::uint32_t min_version) {
    if (name && std::strcmp(name,DCR_MAPLE_CONTROL_IFACE_V3)==0 && min_version<=DCR_MAPLE_CONTROL_VERSION_V3) return &kMapleControlV3;
    if (name && std::strcmp(name,DCR_MAPLE_CONTROL_IFACE_V2)==0 && min_version<=DCR_MAPLE_CONTROL_VERSION_V2) return &kMapleControl;
    return nullptr;
}
const DCRPluginDescriptorV2 kMaplePlugin{
    DCR_PLUGIN_ABI_V2,sizeof(DCRPluginDescriptorV2),DCR_PLUGIN_KIND_MAPLE,0u,
    "DreamcastRecompiled Maple","cpp38e-input-focus-guard-v1",
    &maple_create,&maple_destroy,&maple_reset,&maple_tick,&maple_handles,&maple_read8,&maple_write8,&maple_read32,&maple_write32,&maple_query_interface
};

} // namespace

int maple_test_host_rumble_public(const char* input_spec, std::uint16_t low, std::uint16_t high, char* status, std::size_t status_size) {
#ifdef _WIN32
    return maple_test_host_rumble_impl(input_spec, low, high, status, status_size);
#else
    if (status && status_size) { std::snprintf(status, status_size, "Sony/XInput haptics are Windows-only"); status[status_size-1u] = '\0'; }
    (void)input_spec; (void)low; (void)high;
    return 0;
#endif
}

int maple_probe_host_haptics_public(const char* input_spec, char* status, std::size_t status_size) {
#ifdef _WIN32
    return maple_probe_host_haptics_impl(input_spec, status, status_size);
#else
    if (status && status_size) { std::snprintf(status, status_size, "Sony/XInput haptics are Windows-only"); status[status_size-1u] = '\0'; }
    (void)input_spec;
    return 0;
#endif
}

} // namespace dcrecomp_generated

#ifdef _WIN32
extern "C" __declspec(dllexport) int dcr_maple_test_host_rumble_v1(const char* input_spec, std::uint16_t low, std::uint16_t high, char* status, std::size_t status_size) {
    return dcrecomp_generated::maple_test_host_rumble_public(input_spec, low, high, status, status_size);
}
#else
extern "C" __attribute__((visibility("default"))) int dcr_maple_test_host_rumble_v1(const char* input_spec, std::uint16_t low, std::uint16_t high, char* status, std::size_t status_size) {
    return dcrecomp_generated::maple_test_host_rumble_public(input_spec, low, high, status, status_size);
}
#endif

#ifdef _WIN32
extern "C" __declspec(dllexport) int dcr_maple_probe_host_haptics_v1(const char* input_spec, char* status, std::size_t status_size) {
    return dcrecomp_generated::maple_probe_host_haptics_public(input_spec, status, status_size);
}
#else
extern "C" __attribute__((visibility("default"))) int dcr_maple_probe_host_haptics_v1(const char* input_spec, char* status, std::size_t status_size) {
    return dcrecomp_generated::maple_probe_host_haptics_public(input_spec, status, status_size);
}
#endif

DCR_PLUGIN_V2_EXPORT const dcrecomp_generated::DCRPluginDescriptorV2* dcr_plugin_query_v2() { return &dcrecomp_generated::kMaplePlugin; }
