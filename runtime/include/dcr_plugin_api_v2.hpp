#pragma once

#include <cstddef>
#include <cstdint>

namespace dcrecomp_generated {

inline constexpr std::uint32_t DCR_PLUGIN_ABI_V2 = 0x00020000u;
inline constexpr std::uint32_t DCR_HOST_SERVICES_ABI_V2 = 0x00020000u;

struct DCRMemoryRegionV2 {
    std::uint8_t* data{};
    std::size_t size{};
    std::uint32_t guest_base{};
    std::uint32_t flags{};
};

enum DCRMemoryRegionFlagsV2 : std::uint32_t {
    DCR_MEM_READ  = 1u << 0u,
    DCR_MEM_WRITE = 1u << 1u,
    DCR_MEM_EXEC  = 1u << 2u,
};

struct DCRHostServicesV2 {
    std::uint32_t abi_version;
    std::uint32_t struct_size;
    void* host_context;

    std::uint8_t  (*read8)(void*, std::uint32_t);
    std::uint16_t (*read16)(void*, std::uint32_t);
    std::uint32_t (*read32)(void*, std::uint32_t);
    void (*write8)(void*, std::uint32_t, std::uint8_t);
    void (*write16)(void*, std::uint32_t, std::uint16_t);
    void (*write32)(void*, std::uint32_t, std::uint32_t);

    bool (*get_memory_region)(void*, std::uint32_t, std::size_t, DCRMemoryRegionV2*);
    void (*raise_irq)(void*, std::uint32_t bank, std::uint32_t bits);
    void (*clear_irq)(void*, std::uint32_t bank, std::uint32_t bits);
    bool (*schedule_event)(void*, std::uint64_t cycles_from_now, std::uint32_t event_id, std::uint64_t userdata);
    void (*log)(void*, std::uint32_t level, const char* component, const char* message);
    const char* (*get_config)(void*, const char* key);

    // cpp35a transitional bridge. It lets an ABI2 plugin take ownership of
    // dispatch before the proven cpp34 device implementation/state is moved
    // out of DreamcastRuntime.dll. No DCRuntime pointer crosses the ABI.
    bool (*compat_device_read32)(void*, std::uint32_t kind, std::uint32_t address, std::uint32_t* value);
    bool (*compat_device_write32)(void*, std::uint32_t kind, std::uint32_t address, std::uint32_t value);
};


inline constexpr const char* DCR_MAPLE_CONTROL_IFACE_V2 = "dcr.maple.control.v2";
inline constexpr std::uint32_t DCR_MAPLE_CONTROL_VERSION_V2 = 0x00020000u;

struct DCRMapleControllerStateV2 {
    std::uint32_t struct_size;
    bool present;
    bool connected;
    std::uint16_t buttons;
    std::uint8_t ltrig;
    std::uint8_t rtrig;
    std::int8_t joyx;
    std::int8_t joyy;
    std::int8_t joy2x;
    std::int8_t joy2y;
};

struct DCRMapleControlV2 {
    std::uint32_t version;
    std::uint32_t struct_size;
    bool (*load_controller_profile)(void* instance, const char* path);
    void (*enable_host_input)(void* instance, bool enabled);
    void (*set_controller_present)(void* instance, bool present);
    void (*set_controller_state)(void* instance, const DCRMapleControllerStateV2* state);
    void (*poll_host)(void* instance);
    bool (*get_controller_state)(void* instance, DCRMapleControllerStateV2* state);
    void (*print_stats)(void* instance);
};

// cpp36a: port-aware Maple control plane. V2 remains frozen as the Port-A
// compatibility view; V3 is optional and never crosses the guest ABI.
inline constexpr const char* DCR_MAPLE_CONTROL_IFACE_V3 = "dcr.maple.control.v3";
inline constexpr std::uint32_t DCR_MAPLE_CONTROL_VERSION_V3 = 0x00030000u;

enum DCRMapleDeviceKindV3 : std::uint32_t {
    DCR_MAPLE_DEVICE_EMPTY = 0u,
    DCR_MAPLE_DEVICE_CONTROLLER = 1u,
    DCR_MAPLE_DEVICE_VMU = 2u,
    DCR_MAPLE_DEVICE_PURUPURU = 3u,
};

struct DCRMaplePortStateV3 {
    std::uint32_t struct_size;
    std::uint32_t port;
    DCRMapleControllerStateV2 controller;
    std::uint32_t slot1_kind;
    std::uint32_t slot2_kind;
    std::uint32_t host_device;
    std::uint32_t reserved;
};

struct DCRMapleControlV3 {
    std::uint32_t version;
    std::uint32_t struct_size;
    void (*enable_trace)(void* instance, bool enabled);
    bool (*trace_enabled)(void* instance);
    bool (*set_port_controller_state)(void* instance, std::uint32_t port, const DCRMapleControllerStateV2* state);
    bool (*get_port_controller_state)(void* instance, std::uint32_t port, DCRMapleControllerStateV2* state);
    bool (*set_expansion)(void* instance, std::uint32_t port, std::uint32_t slot, std::uint32_t kind);
    std::uint32_t (*get_expansion)(void* instance, std::uint32_t port, std::uint32_t slot);
    bool (*get_port_state)(void* instance, std::uint32_t port, DCRMaplePortStateV3* state);
    void (*print_device_tree)(void* instance);
};


// cpp37a: GD-ROM control plane. Physical media ownership is kept outside the
// frozen guest ABI. The first revision deliberately exposes bulk operations so
// sector data never crosses the DLL boundary byte-by-byte.
inline constexpr const char* DCR_GDROM_CONTROL_IFACE_V1 = "dcr.gdrom.control.v1";
inline constexpr std::uint32_t DCR_GDROM_CONTROL_VERSION_V1 = 0x00010000u;

struct DCRGDROMTrackV1 {
    std::uint32_t struct_size;
    std::uint32_t session;
    std::uint32_t number;
    std::uint32_t mode;
    std::uint32_t start_fad;
    std::uint32_t end_fad;
    std::uint64_t file_base;
    std::uint32_t sector_size;
    std::uint32_t user_offset;
    std::uint32_t user_size;
};

struct DCRGDROMSummaryV1 {
    std::uint32_t struct_size;
    std::uint32_t track_count;
    std::uint32_t boot_track;
    std::uint32_t boot_fad;
    std::uint32_t logical_sector_size;
    std::uint32_t session_count;
    std::uint64_t image_size;
};

struct DCRGDROMSessionV1 {
    std::uint32_t struct_size;
    std::uint32_t session;
    std::uint32_t first_track;
    std::uint32_t last_track;
    std::uint32_t start_fad;
    std::uint32_t end_fad;
};

struct DCRGDROMControlV1 {
    std::uint32_t version;
    std::uint32_t struct_size;
    bool (*load_disc_map)(void* instance, const char* path);
    const char* (*image_path)(void* instance);
    bool (*get_summary)(void* instance, DCRGDROMSummaryV1* summary);
    bool (*get_track)(void* instance, std::uint32_t index, DCRGDROMTrackV1* track);
    bool (*get_track_by_fad)(void* instance, std::uint32_t fad, DCRGDROMTrackV1* track);
    bool (*get_track_by_number)(void* instance, std::uint32_t number, DCRGDROMTrackV1* track);
    bool (*get_session)(void* instance, std::uint32_t index, DCRGDROMSessionV1* session);
    bool (*read_bytes)(void* instance, std::uint32_t fad, std::uint32_t byte_offset,
                       std::uint32_t bytes, std::uint8_t* out, std::size_t out_size);
    bool (*build_toc)(void* instance, std::uint32_t area, std::uint32_t* words, std::size_t word_count);
    void (*enable_trace)(void* instance, bool enabled);
    bool (*trace_enabled)(void* instance);
    void (*print_stats)(void* instance);
};


// cpp37b: phase-2 GD-ROM control plane. Command lifecycle, multi-transfer
// progress and G1/GD DMA state live inside the device plugin. DCRuntime keeps
// a compatibility mirror only for CDDA code that is intentionally migrated in
// cpp37c. This interface is host/runtime-only and does not alter Guest ABI1.
inline constexpr const char* DCR_GDROM_CONTROL_IFACE_V2 = "dcr.gdrom.control.v2";
inline constexpr std::uint32_t DCR_GDROM_CONTROL_VERSION_V2 = 0x00020000u;

struct DCRGDROMCommandStateV2 {
    std::uint32_t struct_size;
    std::uint32_t last_request_id;
    std::uint32_t next_request_id;
    std::int32_t status;
    std::uint32_t command;
    std::uint32_t params[4];
    std::uint32_t result[4];
    std::uint32_t current_fad;
    std::uint32_t multi_fad;
    std::uint32_t multi_remaining;
    std::uint32_t multi_total;
    std::uint32_t multi_offset;
    std::uint32_t callback;
    std::uint32_t callback_arg;
};

struct DCRGDROMStatsV2 {
    std::uint32_t struct_size;
    std::uint32_t native_phase;
    std::uint64_t map_loads;
    std::uint64_t bulk_reads;
    std::uint64_t sector_reads;
    std::uint64_t bytes_read;
    std::uint64_t toc_builds;
    std::uint64_t session_queries;
    std::uint64_t command_requests;
    std::uint64_t command_state_syncs;
    std::uint64_t multi_starts;
    std::uint64_t multi_transfers;
    std::uint64_t dma_transfers;
    std::uint64_t pio_transfers;
    std::uint64_t dma_irqs;
    std::uint64_t dma_acks;
    std::uint64_t delegated_commands;
};

struct DCRGDROMControlV2 {
    std::uint32_t version;
    std::uint32_t struct_size;
    bool (*reset_command_state)(void* instance, std::uint32_t current_fad);
    bool (*request_command)(void* instance, std::uint32_t command,
                            const std::uint32_t* params4, std::uint32_t* request_id);
    bool (*get_command_state)(void* instance, DCRGDROMCommandStateV2* state);
    bool (*publish_command_state)(void* instance, const DCRGDROMCommandStateV2* state,
                                  bool delegated_to_runtime);
    bool (*begin_multi)(void* instance, std::uint32_t fad, std::uint32_t count);
    bool (*multi_transfer)(void* instance, std::uint32_t dest, std::uint32_t bytes, bool dma);
    bool (*abort_multi)(void* instance);
    bool (*set_callback)(void* instance, std::uint32_t callback, std::uint32_t callback_arg);
    bool (*dma_end_ack)(void* instance);
    bool (*get_stats)(void* instance, DCRGDROMStatsV2* stats);
};

// cpp37c: phase-3 CDDA control/decoder plane. The GD-ROM plugin now owns the
// Red-Book play state, track/FAD resolution and raw 2352-byte PCM decoding.
// DreamcastRuntime remains responsible only for scheduling the 75 Hz drive
// clock and mixing already-decoded stereo PCM into the host audio path.
// This is runtime/plugin-only and leaves the frozen Guest ABI1 untouched.
inline constexpr const char* DCR_GDROM_CONTROL_IFACE_V3 = "dcr.gdrom.control.v3";
inline constexpr std::uint32_t DCR_GDROM_CONTROL_VERSION_V3 = 0x00030000u;

struct DCRGDROMCDDAStateV3 {
    std::uint32_t struct_size;
    bool playing;
    bool paused;
    bool terminated;
    bool sector_mode;
    bool audio_available;
    std::uint32_t start_fad;
    std::uint32_t end_fad;
    std::uint32_t repeat;
    std::uint32_t current_fad;
    std::uint32_t audio_fad;
    std::uint32_t audio_frame;
    std::uint32_t audio_track_number;
    std::uint32_t scramble_mode;
    std::uint32_t byte_order;
    std::uint64_t sector_ticks;
    std::uint64_t completions;
    std::uint64_t sector_reads;
    std::uint64_t frames_mixed;
    std::uint64_t nonzero_frames;
    std::uint64_t read_failures;
    std::uint64_t track_misses;
    std::uint64_t scramble_analyzed;
    std::uint64_t scramble_raw_delta;
    std::uint64_t scramble_yb_delta;
    std::uint64_t scramble_transitions;
    std::uint64_t sync_repairs;
    std::uint64_t impulse_repairs;
    std::uint64_t order_analyzed;
    std::uint64_t order_le_delta;
    std::uint64_t order_be_delta;
};

struct DCRGDROMStatsV3 {
    std::uint32_t struct_size;
    std::uint32_t native_phase;
    std::uint64_t cdda_play_commands;
    std::uint64_t cdda_pause_commands;
    std::uint64_t cdda_release_commands;
    std::uint64_t cdda_stop_commands;
    std::uint64_t cdda_sector_ticks;
    std::uint64_t cdda_completions;
    std::uint64_t cdda_sector_reads;
    std::uint64_t cdda_frames_rendered;
    std::uint64_t cdda_nonzero_frames;
    std::uint64_t cdda_read_failures;
    std::uint64_t cdda_track_misses;
};

struct DCRGDROMControlV3 {
    std::uint32_t version;
    std::uint32_t struct_size;
    bool (*reset_cdda)(void* instance, std::uint32_t current_fad);
    bool (*play_tracks)(void* instance, std::uint32_t first_track,
                        std::uint32_t last_track, std::uint32_t repeat);
    bool (*play_sectors)(void* instance, std::uint32_t start_fad,
                         std::uint32_t end_fad, std::uint32_t repeat);
    bool (*pause)(void* instance);
    bool (*release)(void* instance);
    bool (*stop)(void* instance);
    bool (*seek)(void* instance, std::uint32_t fad);
    bool (*advance_sectors)(void* instance, std::uint32_t count);
    std::uint32_t (*render_frames)(void* instance, std::int16_t* stereo_interleaved,
                                   std::uint32_t frames, bool host_clock);
    bool (*get_cdda_state)(void* instance, DCRGDROMCDDAStateV3* state);
    bool (*get_stats)(void* instance, DCRGDROMStatsV3* stats);
};

struct DCRPluginDescriptorV2 {
    std::uint32_t abi_version;
    std::uint32_t struct_size;
    std::uint32_t kind;
    std::uint32_t flags;
    const char* name;
    const char* build_id;

    void* (*create)(const DCRHostServicesV2* host);
    void (*destroy)(void* instance);
    void (*reset)(void* instance);
    void (*tick)(void* instance, std::uint64_t cycles);

    bool (*handles_address)(void* instance, std::uint32_t address);
    bool (*read8)(void* instance, std::uint32_t address, std::uint8_t* value);
    bool (*write8)(void* instance, std::uint32_t address, std::uint8_t value);
    bool (*read32)(void* instance, std::uint32_t address, std::uint32_t* value);
    bool (*write32)(void* instance, std::uint32_t address, std::uint32_t value);

    const void* (*query_interface)(void* instance, const char* interface_name, std::uint32_t min_version);
};

using DCRPluginQueryV2 = const DCRPluginDescriptorV2* (*)();

} // namespace dcrecomp_generated

#if defined(_WIN32)
#  if defined(DCR_PLUGIN_BUILD)
#    define DCR_PLUGIN_V2_EXPORT extern "C" __declspec(dllexport)
#  else
#    define DCR_PLUGIN_V2_EXPORT extern "C"
#  endif
#else
#  define DCR_PLUGIN_V2_EXPORT extern "C" __attribute__((visibility("default")))
#endif
