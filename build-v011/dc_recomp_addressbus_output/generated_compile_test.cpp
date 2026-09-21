#include "generated_program.hpp"
#include "dc_image.hpp"
#include "dc_native_overrides.hpp"

int main() {
    dcrecomp_generated::DCRuntime runtime;
    dcrecomp_generated::load_embedded_elf_image(runtime);
    dcrecomp_generated::register_recompiled_program(runtime);
    dcrecomp_generated::register_native_overrides(runtime);
    // 0.0.40 regression: P4 Store Queue writes must stay in the SQ until PREF.
    const auto sq_before = dcrecomp_generated::dc_read32(runtime, 0x00800000u);
    dcrecomp_generated::dc_write32(runtime, 0xE0800000u, 0x11223344u);
    if (dcrecomp_generated::dc_read32(runtime, 0x00800000u) != sq_before) return 9;
    dcrecomp_generated::dc_pref(runtime, 0xE0800000u, 0u);
    if (dcrecomp_generated::dc_read32(runtime, 0x00800000u) != 0x11223344u) return 10;
    dcrecomp_generated::dc_aica_enable_arm7(runtime, true, 16u);
    dcrecomp_generated::dc_aica_set_arm7_boot_slice(runtime, 512u);
    dcrecomp_generated::dc_aica_set_timer_div(runtime, 1u);
    dcrecomp_generated::dc_write32(runtime, 0x00800000u, 0xEA000006u);
    dcrecomp_generated::dc_write32(runtime, 0x0080001Cu, 0xEA000007u);
    dcrecomp_generated::dc_write32(runtime, 0x00800020u, 0xE10F1000u);
    dcrecomp_generated::dc_write32(runtime, 0x00800024u, 0xE3C11040u);
    dcrecomp_generated::dc_write32(runtime, 0x00800028u, 0xE121F001u);
    dcrecomp_generated::dc_write32(runtime, 0x0080002Cu, 0xEAFFFFFEu);
    dcrecomp_generated::dc_write32(runtime, 0x00800040u, 0xE2800001u);
    dcrecomp_generated::dc_write32(runtime, 0x00800044u, 0xE59F4010u);
    dcrecomp_generated::dc_write32(runtime, 0x00800048u, 0xE5945000u);
    dcrecomp_generated::dc_write32(runtime, 0x0080004Cu, 0xE3A02040u);
    dcrecomp_generated::dc_write32(runtime, 0x00800050u, 0xE59F3008u);
    dcrecomp_generated::dc_write32(runtime, 0x00800054u, 0xE5832000u);
    dcrecomp_generated::dc_write32(runtime, 0x00800058u, 0xE25EF004u);
    dcrecomp_generated::dc_write32(runtime, 0x0080005Cu, 0x00802D00u);
    dcrecomp_generated::dc_write32(runtime, 0x00800060u, 0x008028A4u);
    dcrecomp_generated::dc_write32(runtime, 0x007028A8u, 0x18u);
    dcrecomp_generated::dc_write32(runtime, 0x007028ACu, 0x50u);
    dcrecomp_generated::dc_write32(runtime, 0x007028B0u, 0x08u);
    dcrecomp_generated::dc_write32(runtime, 0x00702890u, 0xFEu);
    dcrecomp_generated::dc_write32(runtime, 0x0070289Cu, 0x40u);
    dcrecomp_generated::dc_write32(runtime, 0x00702C00u, 0u);
    if (runtime.aica_arm7_in_reset) return 2;
    if (runtime.aica_arm.fiq_exceptions == 0u || runtime.aica_arm.r[0] == 0u) return 3;
    if (runtime.aica_timers[0].overflows == 0u || runtime.aica_arm.r[5] != 2u) return 4;
    // Maple DMA regression: GETCOND must return active-low buttons and analog fields.
    dcrecomp_generated::dc_maple_set_controller_state(runtime, (1u << 2) | (1u << 3), 17u, 33u, -12, 34, 0, 0);
    constexpr std::uint32_t maple_desc = 0x8C7FD000u;
    constexpr std::uint32_t maple_recv = 0x8C7FD100u;
    dcrecomp_generated::dc_write32(runtime, maple_desc + 0u, 0x80000001u);
    dcrecomp_generated::dc_write32(runtime, maple_desc + 4u, 0x0C7FD100u);
    dcrecomp_generated::dc_write32(runtime, maple_desc + 8u, 9u | (0x20u << 8) | (1u << 24));
    dcrecomp_generated::dc_write32(runtime, maple_desc + 12u, 0x01000000u);
    dcrecomp_generated::dc_write32(runtime, 0xA05F6C14u, 1u);
    dcrecomp_generated::dc_write32(runtime, 0xA05F6C04u, 0x0C7FD000u);
    dcrecomp_generated::dc_write32(runtime, 0xA05F6C18u, 1u);
    if ((dcrecomp_generated::dc_read32(runtime, maple_recv) & 0xFFu) != 8u) return 5;
    if (dcrecomp_generated::dc_read32(runtime, maple_recv + 4u) != 0x01000000u) return 6;
    if (dcrecomp_generated::dc_read16(runtime, maple_recv + 8u) != 0xFFF3u) return 7;
    if (runtime.maple_getcond_responses != 1u || runtime.maple_state != 0u) return 8;
    if ((dcrecomp_generated::dc_read32(runtime, 0xA05F6900u) & (1u << 12)) == 0u) return 11;
    dcrecomp_generated::dc_write32(runtime, 0xA05F6900u, 1u << 12);
    if ((dcrecomp_generated::dc_read32(runtime, 0xA05F6900u) & (1u << 12)) != 0u) return 12;
    if (!dcrecomp_generated::dc_pvr_vertex_decoder_selftest()) return 13;
    if (!dcrecomp_generated::dc_pvr_ta_staging_selftest()) return 28;
    if (!dcrecomp_generated::dc_pvr_texture_dirty_region_selftest()) return 29;
    runtime.qacr[0] = 0x10u;
    runtime.perf_profile_enabled = true;
    dcrecomp_generated::SQTAFusedCapture fused_caps[8]{};
    for (std::uint32_t n = 0u; n < 8u; ++n) fused_caps[n] = {0xE0000000u + n * 4u, 0u, 4u};
    const auto fused_before = runtime.sq_ta_fusion_profile_hits;
    const auto zero_before = runtime.sq_ta_zero_copy_profile_hits;
    if (!dcrecomp_generated::dc_pref_ta_fused_packet(runtime, 0xE0000000u, 0x1220u, fused_caps, 8u)) return 30;
    if (runtime.sq_ta_fusion_profile_hits != fused_before + 1u || runtime.sq_ta_fusion_profile_captures < 8u) return 31;
    if (runtime.sq_ta_zero_copy_profile_hits != zero_before + 1u) return 32;
    for (std::uint32_t n = 0u; n < 8u; ++n) dcrecomp_generated::dc_write32_hot(runtime, 0xE0000000u + n * 4u, (n == 0u) ? 0u : n);
    const auto ta_native_before = runtime.pref_ta_native_hits;
    dcrecomp_generated::dc_pref_ta_native(runtime, 0xE0000000u, 0x1234u);
    if (runtime.pref_ta_native_hits != ta_native_before + 1u || runtime.pref_ta_native_local_hits == 0u) return 14;
    const auto ta_flow_before = runtime.pref_ta_native_flow_hits;
    dcrecomp_generated::dc_pref_ta_native(runtime, 0xE0000000u, 0x1235u, true);
    if (runtime.pref_ta_native_flow_hits != ta_flow_before + 1u) return 16;
    const auto ta_fallback_before = runtime.pref_ta_native_fallbacks;
    const auto ta_flow_fallback_before = runtime.pref_ta_native_flow_fallbacks;
    dcrecomp_generated::dc_pref_ta_native(runtime, 0x8C010000u, 0x1236u, true);
    if (runtime.pref_ta_native_fallbacks != ta_fallback_before + 1u || runtime.pref_ta_native_flow_fallbacks != ta_flow_fallback_before + 1u) return 15;
    const auto ta_boundary_fb = runtime.pref_ta_native_fallbacks;
    dcrecomp_generated::dc_pref_ta_native(runtime, 0xE1000000u, 0x1237u);
    if (runtime.pref_ta_native_fallbacks != ta_boundary_fb + 1u) return 17;
    if (!runtime.targets.contains(0x8C010000u)) return 1; // _main
    if (!runtime.targets.contains(0x8C010080u)) return 1; // _memTestAddressBus
    return 0;
}
