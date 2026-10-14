#pragma once

#include "dcr_plugin_api_v2.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace dcrecomp_generated {

struct DCRuntime;

// Runtime-only bridge for cpp37b/c. Kept out of the frozen Guest ABI1 header so
// cached game modules remain binary-compatible while GD-ROM evolves.
bool dc_plugin_gdrom_phase2_available(DCRuntime& runtime);
bool dc_plugin_gdrom_reset_command_state(DCRuntime& runtime, std::uint32_t current_fad);
bool dc_plugin_gdrom_request_command(DCRuntime& runtime, std::uint32_t command,
                                     const std::uint32_t* params4, std::uint32_t& request_id);
bool dc_plugin_gdrom_get_command_state(DCRuntime& runtime, DCRGDROMCommandStateV2& state);
bool dc_plugin_gdrom_publish_command_state(DCRuntime& runtime, const DCRGDROMCommandStateV2& state,
                                           bool delegated_to_runtime);
bool dc_plugin_gdrom_begin_multi(DCRuntime& runtime, std::uint32_t fad, std::uint32_t count);
bool dc_plugin_gdrom_multi_transfer(DCRuntime& runtime, std::uint32_t dest, std::uint32_t bytes, bool dma);
bool dc_plugin_gdrom_abort_multi(DCRuntime& runtime);
bool dc_plugin_gdrom_set_callback(DCRuntime& runtime, std::uint32_t callback, std::uint32_t callback_arg);
bool dc_plugin_gdrom_dma_end_ack(DCRuntime& runtime);
bool dc_plugin_gdrom_get_stats_v2(DCRuntime& runtime, DCRGDROMStatsV2& stats);

// cpp37c phase-3 CDDA bridge. Audio frames are batched across the DLL
// boundary by dcr_plugin_host.cpp so the 44.1 kHz mixer does not issue one
// plugin call per stereo sample.
bool dc_plugin_gdrom_phase3_available(DCRuntime& runtime);
bool dc_plugin_gdrom_cdda_reset(DCRuntime& runtime, std::uint32_t current_fad);
bool dc_plugin_gdrom_cdda_play_tracks(DCRuntime& runtime, std::uint32_t first_track,
                                       std::uint32_t last_track, std::uint32_t repeat);
bool dc_plugin_gdrom_cdda_play_sectors(DCRuntime& runtime, std::uint32_t start_fad,
                                        std::uint32_t end_fad, std::uint32_t repeat);
bool dc_plugin_gdrom_cdda_pause(DCRuntime& runtime);
bool dc_plugin_gdrom_cdda_release(DCRuntime& runtime);
bool dc_plugin_gdrom_cdda_stop(DCRuntime& runtime);
bool dc_plugin_gdrom_cdda_seek(DCRuntime& runtime, std::uint32_t fad);
bool dc_plugin_gdrom_cdda_advance(DCRuntime& runtime, std::uint32_t sectors);
bool dc_plugin_gdrom_cdda_mix_frame(DCRuntime& runtime, bool host_clock,
                                     std::int16_t& left, std::int16_t& right);
bool dc_plugin_gdrom_get_cdda_state_v3(DCRuntime& runtime, DCRGDROMCDDAStateV3& state);
bool dc_plugin_gdrom_get_stats_v3(DCRuntime& runtime, DCRGDROMStatsV3& stats);

} // namespace dcrecomp_generated
