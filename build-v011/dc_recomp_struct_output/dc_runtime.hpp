#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <string>
#include <ostream>
#include <unordered_map>
#include <vector>

#if defined(_MSC_VER) && defined(DCR_HOST_AVX2)
#include <immintrin.h>
#endif

#include "dc_arm7.hpp"

namespace dcrecomp_generated {

// 0.0.196: optional compile-time specialization for validated commercial builds.
#if defined(DCR_FIXED_FPU_MODE)
#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) (DCR_FIXED_FPU_MODE == 2)
#if defined(DCR_FIXED_FPU_REGION_METRICS)
#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((DCR_FIXED_FPU_MODE != 3) || (DCR_FIXED_FPU_REGION_METRICS != 0))
#else
#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((DCR_FIXED_FPU_MODE != 3) || (runtime).fpu_region_metrics)
#endif
#else
#define DCR_FPU_SUPERBLOCK_ENABLED(runtime) ((runtime).fpu_aot_mode == 2u)
#define DCR_FPU_REGION_METRICS_ENABLED(runtime) ((runtime).fpu_aot_mode != 3u || (runtime).fpu_region_metrics)
#endif

#if defined(DCR_LIGHTWEIGHT_HOT_METRICS)
#define DCR_HOT_METRIC_INC(value) ((void)0)
inline constexpr const char* kHotMetricMode = "light";
#else
#define DCR_HOT_METRIC_INC(value) (++(value))
inline constexpr const char* kHotMetricMode = "full";
#endif

// 0.0.170: Flycast-style scalar host FPU primitive. Commercial Win64 builds
// are host-tuned by default and lower FMAC to one FMA3 instruction; portable
// builds retain std::fma and therefore identical single-rounding semantics.
inline float dc_host_fma(float a, float b, float c) noexcept {
#if defined(_MSC_VER) && defined(DCR_HOST_AVX2)
    const __m128 va = _mm_set_ss(a);
    const __m128 vb = _mm_set_ss(b);
    const __m128 vc = _mm_set_ss(c);
    return _mm_cvtss_f32(_mm_fmadd_ss(va, vb, vc));
#else
    return std::fma(a, b, c);
#endif
}
#if defined(_MSC_VER) && defined(DCR_HOST_AVX2)
inline constexpr const char* kHostFmaMode = "fma3";
#else
inline constexpr const char* kHostFmaMode = "portable";
#endif

struct DCRuntime;

struct TraceCallEvent {
    std::uint32_t caller{};
    std::uint32_t target{};
    std::uint32_t r8{};
    std::uint32_t sp{};
};

// 0.0.156: compile-time recognized Store Queue producers capture the four
// 64-bit (or eight 32-bit) writes in cheap function-local storage, then hand
// one complete 32-byte packet to the TA. The capture format is also used by
// the conservative replay path when any runtime guard fails.
struct SQTAFusedCapture {
    std::uint32_t address{};
    std::uint64_t value{};
    std::uint8_t width{}; // 4 or 8 bytes
};

struct SH4Context {
    std::array<std::uint32_t, 16> r{};
    std::uint32_t pc{};
    std::uint32_t pr{};
    std::uint32_t gbr{};
    std::uint32_t vbr{};
    std::uint32_t ssr{};
    std::uint32_t spc{};
    std::uint32_t sgr{};
    std::uint32_t dbr{};
    std::array<std::uint32_t, 8> r_bank{};
    std::uint32_t mach{};
    std::uint32_t macl{};
    std::uint32_t sr{};
    std::uint32_t fpul{};
    std::uint32_t fpscr{};
    std::array<std::uint32_t, 16> fr_bits{};
    std::array<std::uint32_t, 16> xf_bits{};
};

using RecompiledFunction = void(*)(SH4Context&, DCRuntime&);

struct DispatchCacheEntry {
    std::uint32_t requested{0xFFFFFFFFu};
    std::uint32_t target{};
    std::uint32_t relocation_source{};
    std::uint32_t relocation_target{};
    RecompiledFunction function{};
};

struct PVRSoftVertex {
    float x{};
    float y{};
    float z{};
    float u{};
    float v{};
    std::uint32_t argb{};
    std::uint32_t offset_argb{};
    // Ingress source of the actual vertex packet. 0.0.83 colored wireframe
    // from the polygon-header source, which could misattribute a mixed stream.
    std::uint8_t source{};
};

struct DCRuntime;
using PVRVertexDecodeFn = PVRSoftVertex (*)(DCRuntime&, const std::uint8_t*);

struct PVRSoftState {
    bool textured{};
    bool gouraud{};
    bool vertex_alpha{};
    bool texture_alpha{};
    bool depth_write{true};
    bool nontwiddled{};
    bool vq{};
    bool mipmapped{};
    bool stride_select{};
    bool uv_16bit{};
    bool offset_color{};
    bool two_volumes{};
    // 0.0.146: retain the complete PCW/TSP surface identity used by Flycast.
    // These are surface state, not vertex data, so the 28-byte hot vertex stays unchanged.
    bool shadow{};
    bool supersample{};
    bool color_clamp{};
    bool dst_select{};
    bool src_select{};
    bool dcalc_ctrl{};
    bool cache_bypass{};
    std::uint8_t mipmap_d{};
    std::uint8_t fog_ctrl{};
    std::uint8_t clip_mode{}; // PCW User_Clip: 0/1 off, 2 keep inside rect, 3 keep outside rect
    // USER TILE CLIP is global TA state, but each polygon captures the current
    // rectangle at header registration time. This matters for deferred lists.
    bool user_clip_valid{};
    std::uint8_t user_clip_xmin{};
    std::uint8_t user_clip_ymin{};
    std::uint8_t user_clip_xmax{};
    std::uint8_t user_clip_ymax{};
    std::uint8_t list_type{};
    std::uint8_t color_format{};
    std::uint8_t vertex_type{};
    // 0.0.132: derived once from vertex_type at polygon-header decode time.
    // The hot Type-7 vertex path no longer re-runs the 64-byte type switch.
    bool vertex_type_64{};
    std::uint8_t texture_shading{};
    std::uint8_t blend_src{};
    std::uint8_t blend_dst{};
    std::uint8_t depth_mode{6u};
    std::uint8_t cull_mode{};
    std::uint8_t pixel_mode{};
    std::uint8_t texture_filter{};
    std::uint8_t uv_flip{};
    std::uint8_t uv_clamp{};
    std::uint8_t palette_select{};
    std::uint16_t texture_width{};
    std::uint16_t texture_height{};
    std::uint32_t texture_offset{};
    std::uint32_t sprite_argb{0xFFFFFFFFu};
    std::uint32_t sprite_offset_argb{0x00000000u};
    // Intensity color formats use a face color supplied by the polygon
    // parameter. Col_Type=3 intentionally reuses the previously supplied
    // intensity face color, so keep these across ordinary packed/float headers.
    std::uint32_t face_base_argb{0xFFFFFFFFu};
    std::uint32_t face_offset_argb{0xFFFFFFFFu};
    // 0.0.83 provenance: remember which ingress path supplied the Global
    // Parameter that owns this surface. Vertices inherit this state, which
    // lets diagnostics isolate Store Queue vs CH2 geometry without guessing
    // from Z or screen position.
    std::uint8_t source{}; // 0 unknown, 1 SQ, 2 CH2, 3 direct CPU
    std::uint32_t source_pc{};
    std::uint64_t surface_id{};
};

struct PVRDeferredState {
    PVRSoftState state{};
    // 0.0.126: texture/state support is invariant for every triangle that
    // references this snapshot. Validate it once at capture time instead of
    // re-checking dimensions/ownership for every triangle at frame submit.
    bool gpu_supported{true};
    // 0.0.117: immutable decoded texture ownership is now per deferred surface
    // state rather than per triangle. A heavy commercial frame can contain
    // 10k+ triangles but only a few hundred actual TA surfaces, so keeping one
    // shared_ptr/state snapshot per surface avoids thousands of atomic refcount
    // operations and full PVRSoftState copies while preserving exact fallback
    // semantics.
    std::shared_ptr<const std::vector<std::uint32_t>> texture_pixels;
    bool background{};
};

struct PVRDeferredTriangle {
    // 0.0.136: triangles reference the frame's contiguous vertex arena instead
    // of embedding three full PVRSoftVertex copies. Triangle strips therefore
    // keep each decoded TA vertex once while preserving the exact triangle-list
    // order used by the proven 0.0.132 renderer.
    std::uint32_t i0{};
    std::uint32_t i1{};
    std::uint32_t i2{};
    std::uint32_t state_index{};
    float sort_z{};
    std::uint64_t order{};
};

static_assert(sizeof(PVRDeferredTriangle) <= 32u,
              "Indexed deferred PVR triangles must remain compact");

// 0.0.148: opaque/punch-through geometry does not need per-triangle sort
// metadata. Keep its already-triangulated index stream compact and retain
// only the state ranges needed by the GPU/software consumers. Translucent
// geometry intentionally keeps PVRDeferredTriangle because its stable Z/order
// sort is semantically observable.
struct PVRDeferredOpaqueRun {
    std::uint32_t state_index{};
    std::uint32_t first_index{};
    std::uint32_t index_count{};
    // 0=triangle list, 1=materialized triangle strip with 0xFFFFFFFF cut
    // indices, 2=0.0.170 late strip descriptor. For topology 2, first_index is
    // firstVertex and index_count is vertexCount; no index array entries exist
    // until GPU submission. The software fallback consumes the vertex range
    // directly.
    std::uint8_t topology{};
    std::uint8_t reserved0{};
    std::uint16_t reserved1{};
};

static_assert(sizeof(PVRDeferredOpaqueRun) == 16u,
              "Opaque deferred runs must remain compact");

struct PVRTextureCacheEntry {
    bool valid{};
    std::uint32_t texture_offset{};
    std::uint16_t width{};
    std::uint16_t height{};
    std::uint8_t pixel_mode{};
    std::uint8_t palette_select{};
    std::uint8_t palette_format{};
    std::uint8_t flags{}; // bit0 nontwiddled, bit1 VQ, bit2 stride, bit3 texture alpha, bit4 mipmapped
    std::uint32_t stride_pixels{};
    // 0.0.141: Flycast-style texture dirty regions. Each decoded cache entry
    // registers the conservative VRAM byte range backing the texture. VRAM
    // writes dirty only entries whose registered 4 KiB pages intersect.
    std::uint32_t vram_start{};
    std::uint32_t vram_size{};
    std::uint64_t vram_epoch{}; // diagnostic: epoch of last decode/refresh
    std::uint64_t palette_epoch{};
    std::shared_ptr<std::vector<std::uint32_t>> pixels;
};

struct AICAHostChannel {
    bool playing{};
    std::uint32_t base{};
    std::uint32_t type{};
    std::uint32_t length{};
    std::uint32_t loop{};
    std::uint32_t loopstart{};
    std::uint32_t loopend{};
    std::uint32_t freq{44100u};
    std::uint32_t vol{255u};
    std::uint32_t pan{128u};
    std::uint32_t pos{};
};

struct AICANativeSlot {
    bool playing{};
    bool releasing{};
    std::uint32_t reg0{};
    std::uint32_t sample_address{};
    std::uint32_t loop_start{};
    std::uint32_t loop_end{};
    std::uint32_t amp_env1{};
    std::uint32_t amp_env2{};
    std::uint32_t pitch{};
    std::uint8_t pan{};
    std::uint8_t direct_send_level{};
    std::uint8_t total_level{255u};
    std::uint8_t format{};
    bool loop{};
    double position{};
    double pitch_step{1.0};
    double envelope_attenuation{}; // 0..1023 AEG attenuation units
    double release_attenuation_step{};
    // 0.0.126: non-releasing voices keep constant TL/DIPAN/Send gains for
    // thousands of 44.1 kHz output frames. Cache those gains and only rebuild
    // them after the corresponding slot registers change.
    double cached_left_gain{};
    double cached_right_gain{};
    bool cached_gain_valid{};
    // Yamaha AICA ADPCM decoder state. The hardware codec is predictive, so
    // random nibble reads are not valid: each sample depends on the preceding
    // sample and quantizer. Keep a tiny rolling cache for interpolation and a
    // loop-start snapshot for normal ADPCM (PCMS=2). Long-stream ADPCM
    // (PCMS=3) deliberately keeps predictor/quantizer state across loops.
    std::uint32_t adpcm_cursor{}; // next sample index to decode
    std::int32_t adpcm_prev_sample{};
    std::int32_t adpcm_quant{127};
    bool adpcm_loop_snapshot_valid{};
    std::int32_t adpcm_loop_prev_sample{};
    std::int32_t adpcm_loop_quant{127};
    std::array<std::uint32_t, 4> adpcm_cache_index{
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
    std::array<std::int16_t, 4> adpcm_cache_sample{};
    std::uint8_t adpcm_cache_next{};
    std::uint64_t starts{};
};

struct AICATimerState {
    std::uint8_t counter{};
    std::uint8_t prescale{};
    std::uint64_t arm_step_accumulator{};
    std::uint64_t overflows{};
};

struct MapleHostController {
    bool connected{true};
    std::uint32_t buttons{}; // cooked KOS-style pressed bits
    std::uint8_t ltrig{};
    std::uint8_t rtrig{};
    std::int8_t joyx{};
    std::int8_t joyy{};
    std::int8_t joy2x{};
    std::int8_t joy2y{};
};

struct G2DMAChannel {
    std::uint32_t g2_addr{};
    std::uint32_t sh4_addr{};
    std::uint32_t size{};
    std::uint32_t dir{};
    std::uint32_t trigger_select{};
    std::uint32_t enable{};
    std::uint32_t start{};
    std::uint32_t suspend{};
};

struct SH4TMUChannel {
    std::uint32_t tcor{0xFFFFFFFFu};
    std::uint32_t tcnt{0xFFFFFFFFu};
    std::uint16_t tcr{};
    std::uint64_t cycle_accumulator{};
    // 0.0.197: no-IRQ TMU channels may accumulate guest cycles lazily and
    // materialize their exact visible state only when software observes or
    // reprograms the timer.
    std::uint64_t lazy_sh4_cycles{};
    std::uint64_t ticks{};
    std::uint64_t underflows{};
};

struct GDHleState {
    std::uint32_t last_request_id{0xFFFFFFFFu};
    std::uint32_t next_request_id{2u};
    std::int32_t status{}; // 0 idle, 1 busy, 2 complete, 3 continue, -1 error
    std::uint32_t command{};
    std::array<std::uint32_t, 4> params{};
    std::array<std::uint32_t, 4> result{};
    std::uint32_t current_fad{150u};
    std::uint32_t multi_fad{};
    std::uint32_t multi_remaining{};
    std::uint32_t multi_total{};
    std::uint32_t multi_offset{};
    std::uint32_t callback{};
    std::uint32_t callback_arg{};
    bool cdda_playing{};
    bool cdda_paused{};
    bool cdda_terminated{};
    bool cdda_sector_mode{};
    std::uint32_t cdda_start{};
    std::uint32_t cdda_end{};
    std::uint32_t cdda_repeat{};
    std::uint64_t cdda_cycle_fraction{};
    // 0.0.101: in --device-clock-host mode the drive itself advances from wall
    // time. Guest SH-4 work can run faster/slower than real time, while
    // Red-Book CDDA is fixed at exactly 75 sectors/s.
    std::uint64_t cdda_host_anchor_ns{};
    std::uint64_t cdda_host_fraction{};
    std::uint64_t cdda_sector_ticks{};
    std::uint64_t cdda_completions{};
    // 0.0.104: raw CD-DA is nominally 44.1 kHz / signed 16-bit / stereo.
    // Most sectors contain 588 frames, but some self-boot Yellow-Book sectors
    // carry only 585 payload frames plus a 12-byte sync header and are
    // resampled back to 588 for output. Keep an independent PCM cursor so
    // host audio can consume samples smoothly while the GD status playhead
    // advances at 75 sectors/s from the active guest or host-synchronized drive clock.
    std::uint32_t cdda_audio_fad{};
    std::uint32_t cdda_audio_frame{};
    std::uint32_t cdda_audio_cache_fad{0xFFFFFFFFu};
    std::array<std::uint8_t, 2352> cdda_audio_cache{};
    bool cdda_audio_cache_valid{};
    bool cdda_audio_available{};
    std::uint64_t cdda_audio_sector_reads{};
    std::uint64_t cdda_audio_frames_mixed{};
    std::uint64_t cdda_audio_nonzero_frames{};
    std::uint64_t cdda_audio_read_failures{};
    std::uint64_t cdda_audio_track_misses{};
    // 0.0.101: old/self-boot DiscJuggler images can change CDDA storage
    // encoding even inside one logical audio track. Classify every sector as
    // raw PCM or Yellow-Book L2 scrambled instead of locking one mode per track.
    // 0=undecided, 1=raw PCM, 2=YellowBook for the current decoded sector.
    std::uint32_t cdda_audio_track_number{};
    std::uint32_t cdda_audio_scramble_mode{};
    std::uint32_t cdda_audio_scramble_analyzed{};
    std::uint64_t cdda_audio_scramble_raw_delta{};
    std::uint64_t cdda_audio_scramble_yb_delta{};
    bool cdda_audio_scramble_defaulted{};
    std::uint64_t cdda_audio_scramble_transitions{};
    std::uint64_t cdda_audio_sync_repairs{};
    // 0.0.101: decoded Yellow-Book CDDA can contain sparse one-sample
    // impulses even after correct L2 descrambling. A real optical path would
    // conceal isolated bad audio samples; count the host-side interpolation.
    std::uint64_t cdda_audio_impulse_repairs{};
    bool cdda_audio_sector_sync_prefix{};
    bool cdda_audio_prev_sample_valid{};
    std::int16_t cdda_audio_prev_l{};
    std::int16_t cdda_audio_prev_r{};
    // DiscJuggler audio is normally little-endian, but a few images/tools can
    // leave the payload byte-swapped. 0=undecided (decode LE), 1=LE, 2=BE.
    std::uint32_t cdda_audio_byte_order{};
    std::uint32_t cdda_audio_order_analyzed{};
    std::uint64_t cdda_audio_order_le_delta{};
    std::uint64_t cdda_audio_order_be_delta{};
    bool cdda_audio_order_defaulted{};
    std::uint64_t get_drv_stat_calls{};
    std::uint64_t req_stat_commands{};
    std::uint64_t get_scd_commands{};
};

struct GDDiscTrack {
    std::uint32_t session{};
    std::uint32_t number{};
    std::uint32_t mode{};
    std::uint32_t start_fad{};
    std::uint32_t end_fad{};
    std::uint64_t file_base{};
    std::uint32_t sector_size{2048u};
    std::uint32_t user_offset{};
    std::uint32_t user_size{2048u};

    bool data() const { return mode != 0u; }
    bool audio() const { return mode == 0u; }
    bool contains(std::uint32_t fad) const { return fad >= start_fad && fad <= end_fad; }
};

struct DCRuntime {
    static constexpr std::uint32_t kMainRamPhysicalBase = 0x0C000000u;
    static constexpr std::size_t kMainRamSize = 16u * 1024u * 1024u;
    // PC-relative literals are normally baked into generated code. Retail loaders
    // may patch their literal pools at runtime (Lodoss stores its saved SP through
    // a P2 alias and reloads it with MOV.L @(disp,PC)). Track guest-written SDRAM
    // pages so only modified literal pages fall back to a real RAM read.
    static constexpr std::size_t kMainRamLiteralDirtyPageShift = 12u;
    static constexpr std::size_t kMainRamLiteralDirtyPageSize = 1u << kMainRamLiteralDirtyPageShift;
    static constexpr std::size_t kMainRamLiteralDirtyPages = kMainRamSize / kMainRamLiteralDirtyPageSize;
    static constexpr std::size_t kVramSize = 8u * 1024u * 1024u;
    static constexpr std::size_t kAicaRamSize = 2u * 1024u * 1024u;
    // The retail BIOS font syscall returns 0xA0100020 inside the upper half
    // of the Dreamcast's 2 MiB boot-ROM window. 0.0.55 incorrectly exposed
    // only 128 KiB after 0x00100000; Japanese glyph lookup legitimately
    // reaches well beyond that (ChuChu hits physical 0x0012BAC0). Keep a
    // deterministic synthetic backing for the complete upper 1 MiB ROM/font
    // region without redistributing any Dreamcast BIOS bytes.
    // The lower 1 MiB of the 2 MiB Dreamcast boot-ROM window contains BIOS
    // code/data. Retail Katana bootstrap code may legally sweep this region
    // through its uncached P2 alias (0xA0000000...) as part of cache setup.
    // Keep a deterministic read-only HLE backing so BIOS-less commercial
    // titles see valid hardware address space instead of an unmapped fault.
    static constexpr std::uint32_t kBootRomPhysicalBase = 0x00000000u;
    static constexpr std::size_t kBootRomLowSize = 0x00100000u;
    static constexpr std::uint32_t kBiosFontPhysicalBase = 0x00100000u;
    static constexpr std::size_t kBiosFontSize = 0x00100000u;
    // FONTROM_ADDRESS returns 0xA0100020. The bytes beginning at that pointer
    // use the Dreamcast BIOS font layout documented by KallistiOS: 288
    // 12x24 narrow slots followed by 24x24 JIS glyphs. We synthesize glyphs
    // lazily from host fonts instead of redistributing Sega BIOS font data.
    static constexpr std::size_t kBiosFontDataOffset = 0x20u;
    static constexpr std::size_t kBiosFontThinWidth = 12u;
    static constexpr std::size_t kBiosFontWideWidth = 24u;
    static constexpr std::size_t kBiosFontHeight = 24u;
    static constexpr std::size_t kBiosFontThinBytes = 36u;
    static constexpr std::size_t kBiosFontWideBytes = 72u;
    static constexpr std::size_t kBiosFontNarrowGlyphs = 288u;
    static constexpr std::size_t kBiosFontWideGlyphs = 7056u;
    static constexpr std::size_t kBiosFontWideStart = kBiosFontNarrowGlyphs * kBiosFontThinBytes;
    // SH-4 operand-cache RAM mode exposes a small on-chip RAM window through
    // 0x7C000000-0x7FFFFFFF. The Katana IP.BIN bootstrap uses 0x7E001000 as
    // its temporary stack before handing control to 1ST_READ.BIN.
    static constexpr std::size_t kOnChipRamSize = 16u * 1024u;
    static constexpr std::uint32_t kPvrRegsPhysicalBase = 0x005F8000u;
    static constexpr std::uint32_t kPvrRegsPhysicalEnd = 0x005FA000u;
    static constexpr std::uint32_t kHollyPhysicalBase = 0x005F0000u;
    static constexpr std::uint32_t kHollyPhysicalEnd = 0x00600000u;

    std::vector<std::uint8_t> main_ram;
    std::array<std::uint8_t, kMainRamLiteralDirtyPages> main_ram_literal_dirty{};
    std::uint64_t main_ram_dynamic_literal_reads{};
    std::vector<std::uint8_t> vram;
    std::vector<std::uint8_t> aica_ram;
    std::vector<std::uint8_t> boot_rom_low;
    std::vector<std::uint8_t> bios_font_rom;
    std::array<std::uint8_t, kBiosFontNarrowGlyphs> bios_font_narrow_generated{};
    std::vector<std::uint8_t> bios_font_wide_generated;
    std::uint64_t boot_rom_reads{};
    bool boot_rom_external{};
    std::uint64_t bios_font_raw_reads{};
    std::uint64_t bios_font_synth_narrow{};
    std::uint64_t bios_font_synth_wide{};
    std::vector<std::uint8_t> onchip_ram;
    std::unordered_map<std::uint32_t, std::uint32_t> mmio32;
    std::uint8_t tmu_tocr{};
    std::uint8_t tmu_tstr{};
    std::array<SH4TMUChannel, 3> tmu{};
    std::unordered_map<std::uint32_t, RecompiledFunction> targets;
    // Dynamic SH-4 calls are extremely hot in retail titles. ChuChu Rocket's
    // title screen alone executes >130 million native dispatches, while the
    // target set is tiny and highly repetitive. A small direct-mapped cache
    // avoids 2-3 unordered_map probes on the common path without changing
    // guest-visible dispatch semantics.
    static constexpr std::size_t kDispatchCacheSize = 4096u;
    std::array<DispatchCacheEntry, kDispatchCacheSize> dispatch_cache{};
    std::uint64_t dispatch_cache_hits{};
    std::uint64_t dispatch_cache_misses{};
    bool fast_dispatch_enabled{};
    std::uint64_t fast_dispatch_calls{};
    // 0.0.96: generated indirect CALL sites execute the common direct-cache
    // hit path inline in generated_program.cpp, avoiding a cross-TU runtime
    // call for the millions of repetitive retail entity/callback dispatches.
    std::uint64_t inline_dynamic_dispatch_hits{};
    std::uint64_t inline_dynamic_dispatch_fallbacks{};
    // 0.0.120: safe per-call-site direct hints for dynamic JSR/BSRF. The
    // generated site compares the real runtime target against analyzer-proven
    // candidates before making a normal direct C++ call. A mismatch always
    // falls back to the existing dynamic dispatcher, so function-pointer sites
    // that legitimately change target keep exact guest semantics.
    std::uint64_t dynamic_direct_hint_hits{};
    std::uint64_t dynamic_direct_hint_fallbacks{};
    // 0.0.120: readiness latches collapse the invariant tracing/observer/
    // relocation checks out of the >100k/frame retail dispatch hot path.
    // They are refreshed only when configuration or relocation context changes.
    bool fast_dynamic_dispatch_ready{};
    bool direct_dispatch_ready{};
    std::uint64_t dispatch_ready_refreshes{};
    bool direct_dispatch_enabled{};
    std::uint64_t direct_static_calls{};
    std::uint64_t direct_tail_calls{};
    std::uint64_t direct_dispatch_fallbacks{};
    // Retail 3D path diagnostics. In particular, FTRV must read the
    // architectural XF bank, which swaps with FR when FPSCR.FR is set.
    std::uint64_t fpu_ftrv_ops{};
    std::uint64_t fpu_ftrv_fr1_ops{};
    std::uint64_t fpu_fipr_ops{};
    std::uint64_t fpu_fmac_ops{};
    std::uint64_t fpu_fsca_ops{};
    std::uint64_t fpu_fsrra_ops{};
    std::uint64_t fpu_frchg_ops{};
    // 0.0.154: AOT basic-block FR/XF cache telemetry. Hits execute the
    // single-precision cached path; fallbacks retain the 0.0.152 helpers when
    // PR/SZ/RM are incompatible. Reuse/writeback counts are static block-plan
    // estimates accumulated per execution so logs can quantify coverage.
    std::uint64_t fpu_block_cache_hits{};
    std::uint64_t fpu_block_cache_fallbacks{};
    std::uint64_t fpu_block_cache_ops{};
    std::uint64_t fpu_block_cache_reuses{};
    std::uint64_t fpu_block_cache_writebacks{};
    // 0.0.154 trace counters are retained for log compatibility; 0.0.155
    // defaults to the cheaper static superblock cache below.
    std::uint64_t fpu_trace_guard_entries{};
    std::uint64_t fpu_trace_guard_reuses{};
    std::uint64_t fpu_trace_lazy_loads{};
    std::uint64_t fpu_trace_actual_writebacks{};
    std::uint64_t fpu_trace_tick_flushes{};
    std::uint64_t fpu_trace_barrier_flushes{};
    std::uint64_t fpu_trace_exit_flushes{};
    std::uint64_t fpu_trace_cross_block_keeps{};
    // 0.0.170: runtime-selectable AOT FPU strategy. 1 = legacy/proven region
    // telemetry path, 2 = static function-superblock A/B, 3 = Region+ production.
    std::uint32_t fpu_aot_mode{3u};
    // Region+ keeps expensive per-region diagnostic counters off by default.
    // DCR_FPU_METRICS=1 re-enables them without recompilation.
    bool fpu_region_metrics{};
    std::uint64_t fpu_super_entries{};
    std::uint64_t fpu_super_reuses{};
    std::uint64_t fpu_super_static_loads{};
    std::uint64_t fpu_super_actual_writebacks{};
    std::uint64_t fpu_super_tick_flushes{};
    std::uint64_t fpu_super_barrier_flushes{};
    std::uint64_t fpu_super_exit_flushes{};
    std::uint64_t fpu_super_cross_block_keeps{};
    // 0.0.170 block-local scalar FPU cache telemetry. Unlike fpu-super these
    // counters describe only FR scalar values; FTRV matrix reads stay outside
    // the scalar allocator to avoid XMM pressure.
    std::uint64_t fpu_ssa_regions{};
    std::uint64_t fpu_ssa_scalar_loads{};
    std::uint64_t fpu_ssa_scalar_writebacks{};
    std::uint64_t fpu_ssa_matrix_reads{};
    std::uint64_t fpu_ssa_fmac_ops{};
    // 0.0.158: static integer-register cache telemetry. Counters are updated
    // only in explicit perf-profile mode so the optimization itself does not
    // recreate the global-write overhead removed in 0.0.155.
    std::uint64_t gpr_cache_entries{};
    std::uint64_t gpr_cache_static_loads{};
    std::uint64_t gpr_cache_writebacks{};
    std::uint64_t gpr_cache_tick_flushes{};
    std::uint64_t gpr_cache_barrier_flushes{};
    std::uint64_t gpr_cache_exit_flushes{};
    // 0.0.156: one-build adaptive hot traces. Natural-loop blocks are emitted
    // ahead of time; a tiny saturating execution profile promotes them after
    // repeated use. Once promoted, block-entry PC/current-PC stores are elided
    // until the exact scheduler boundary or an architectural/external barrier.
    bool hot_trace_enabled{false};
    std::uint32_t hot_trace_threshold{64u};
    std::uint64_t hot_trace_candidates{};
    std::uint64_t hot_trace_promotions{};
    std::uint64_t hot_trace_entries{};
    std::uint64_t hot_trace_pc_elisions{};
    std::uint64_t hot_trace_syncs{};
    std::uint64_t hot_trace_full_ticks{};
    // 0.0.83: raw SZ=1 FMOV pair traffic. FMOV moves adjacent 32-bit lanes
    // in register/memory order; this is different from DRn's numeric-double
    // high/low word interpretation. Retail 3D paths often write these pairs
    // directly into Store Queue before PREF commits them to the TA.
    std::uint64_t fpu_fmov64_loads{};
    std::uint64_t fpu_fmov64_stores{};
    std::uint64_t fpu_fmov64_sq_stores{};
    std::uint64_t fpu_fmov64_reg_moves{};
    // Native templates copied by the guest (notably VBR exception handlers)
    // are rebound lazily by byte signature to their original recompiled entry.
    std::unordered_map<std::uint32_t, std::uint32_t> relocated_code_aliases;
    // While a copied SH-4 code template is executing through its original
    // recompiled body, PC-relative addresses must retain the copied code
    // location. These fields describe the currently active source->copy
    // mapping and are saved/restored across nested native dispatch.
    std::uint32_t active_relocation_source{};
    std::uint32_t active_relocation_target{};
    std::uint64_t relocated_code_matches{};
    std::array<std::uint8_t, 64> store_queues{};
    std::array<std::uint32_t, 2> qacr{};
    std::array<std::uint8_t, 32> ta_partial{};
    std::size_t ta_partial_size{};
    // 0.0.140: Flycast-style TA front-end staging. The emulation-side path
    // stores raw 32-byte TA transfers and advances only a tiny state machine.
    // Full polygon/vertex decoding is deferred until STARTRENDER/TA_LIST_INIT,
    // where the contiguous stream can be parsed in cache-friendly runs.
    // 0.0.155: keep the hot TA stream at 32 bytes plus a one-byte source
    // sidecar. Per-packet source PCs are diagnostic-only and are captured only
    // when DCR_PVR_PROVENANCE=1, so normal gameplay no longer drags 8 bytes of
    // cold metadata through every Type-7/8 vertex run.
    struct alignas(32) PVRTAStreamRaw {
        std::array<std::uint8_t, 32> bytes{};
    };
    static_assert(sizeof(PVRTAStreamRaw) == 32u);
    std::vector<PVRTAStreamRaw> pvr_ta_stream;
    std::vector<std::uint8_t> pvr_ta_stream_source;
    std::vector<std::uint32_t> pvr_ta_stream_pc;
    bool pvr_ta_provenance_enabled{};
    std::uint8_t pvr_ta_front_state{};
    std::uint8_t pvr_ta_front_list{7u};
    bool pvr_ta_replay_active{};
    std::uint64_t pvr_ta_capture_packets{};
    std::uint64_t pvr_ta_parse_flushes{};
    std::uint64_t pvr_ta_bulk_runs{};
    std::uint64_t pvr_ta_bulk_vertices{};
    // 0.0.155: Type-7/8 replay batches heartbeat/source accounting once per
    // contiguous run instead of touching several uint64 counters per vertex.
    std::uint64_t pvr_ta_metric_batched_vertices{};
    // 0.0.143: live coverage for direct typed staged loops. Indexed by the
    // conventional TA vertex-format id; currently only 7 and 8 are enabled.
    std::array<std::uint64_t, 15> pvr_ta_typed_runs{};
    std::array<std::uint64_t, 15> pvr_ta_typed_vertices{};
    // 0.0.146: Flycast-style strip closure for the dominant staged Type-7/8
    // path. When the complete strip (including EOL) is already present in the
    // captured TA stream, vertices are accumulated first and its triangle-list
    // records are materialized in one contiguous batch at strip end.
    std::uint64_t pvr_ta_strip_bulk_runs{};
    std::uint64_t pvr_ta_strip_bulk_vertices{};
    std::uint64_t pvr_ta_strip_bulk_triangles{};
    std::uint64_t pvr_ta_strip_bulk_fallbacks{};
    // 0.0.170: Flycast-style whole-strip decode. Complete staged Type-7/8
    // strips bypass pvr_append_strip_vertex() entirely: one contiguous arena
    // growth, one tight typed decode loop and one EndPolyStrip-style commit.
    std::uint64_t pvr_ta_direct_strip_runs{};
    std::uint64_t pvr_ta_direct_strip_vertices{};
    std::uint64_t pvr_ta_direct_strip_fallbacks{};
    // 0.0.148: cumulative coverage for the compact opaque index-stream path.
    std::uint64_t pvr_opaque_index_triangles{};
    std::uint64_t pvr_opaque_index_indices{};
    std::uint64_t pvr_opaque_index_runs{};
    std::uint64_t pvr_opaque_index_merges{};
    // 0.0.149: complete staged opaque strips resolve their state without
    // materializing a special first triangle, then emit the entire index
    // sequence with one resize/commit.
    std::uint64_t pvr_opaque_strip_bulk_runs{};
    std::uint64_t pvr_opaque_strip_bulk_triangles{};
    std::uint64_t pvr_opaque_strip_bulk_indices{};
    // 0.0.152 native strip telemetry: strips/runs that reached D3D11 without
    // triangle-list expansion, vertices/actual indices and indices avoided.
    std::uint64_t pvr_gpu_native_strip_runs{};
    std::uint64_t pvr_gpu_native_strip_vertices{};
    std::uint64_t pvr_gpu_native_strip_indices{};
    std::uint64_t pvr_gpu_native_strip_indices_saved{};
    std::uint64_t pvr_gpu_native_strip_fallbacks{};
    // 0.0.170: native opaque strips can stay as firstVertex/vertexCount
    // descriptors until final GPU submission. This removes the TA-time index
    // write plus the later runtime->GPU scratch copy; the final R32 strip-cut
    // stream is materialized exactly once in submission order.
    std::uint64_t pvr_late_strip_runs{};
    std::uint64_t pvr_late_strip_vertices{};
    std::uint64_t pvr_late_strip_indices_materialized{};
    std::uint64_t pvr_late_strip_index_writes_avoided{};
    std::uint64_t pvr_late_strip_gpu_run_merges{};
    std::uint64_t pvr_gpu_topology_switches{};
    std::uint64_t pvr_ta_generic_replays{};
    std::uint64_t pvr_ta_stream_peak{};
    std::uint64_t pvr_ta_stream_parse_ns{};
    std::array<std::uint8_t, 64> pvr_sprite_partial{};
    std::size_t pvr_sprite_partial_size{}; // 0=no pending half, 32=waiting for sprite tail
    // A Sprite Global Parameter remains active for *multiple* 64-byte Sprite
    // Vertex Parameters until another global parameter/EOL changes TA state.
    // 0.0.78 incorrectly cleared this after the first sprite, so the next
    // sprite's A half was decoded as an ordinary polygon vertex and its B half
    // commonly looked like a bogus Parameter Type 2 / OBJECT LIST SET packet.
    bool pvr_sprite_mode{};
    // Some TA polygon headers and vertex parameters occupy two consecutive
    // 32-byte FIFO writes. Keep the first half until the second arrives rather
    // than accidentally interpreting the continuation as a fresh PCW.
    std::array<std::uint8_t, 64> pvr_long_partial{};
    std::uint8_t pvr_long_kind{}; // 0 none, 1 polygon header, 2 polygon vertex, 3 modifier-volume vertex
    std::uint8_t pvr_long_vertex_type{};
    // 0.0.136: Flycast-style per-surface vertex decoder selection. The
    // polygon header chooses one fully specialized decoder once; Type-7 vertex
    // packets no longer execute the generic 0..14 format switch individually.
    PVRVertexDecodeFn pvr_vertex_decoder{};
    std::uint8_t pvr_vertex_decoder_type{0xFFu};
    bool pvr_modifier_volume_active{};
    // USER TILE CLIP is a 32-byte TA control parameter. Coordinates are tile
    // units (32x32 pixels) and remain active until another clip command.
    std::uint8_t pvr_user_clip_xmin{};
    std::uint8_t pvr_user_clip_ymin{};
    std::uint8_t pvr_user_clip_xmax{};
    std::uint8_t pvr_user_clip_ymax{};
    bool pvr_user_clip_valid{};
    std::vector<std::uint32_t> pvr_framebuffer;       // current TA registration frame
    std::vector<std::uint32_t> pvr_render_buffer;     // scene handed from TA to ISP/TSP
    std::vector<std::uint32_t> pvr_present_buffer;    // framebuffer currently scanned out
    std::vector<float> pvr_depth;
    // 0.0.112: triangle strips only need a three-vertex rolling window.
    std::array<PVRSoftVertex, 3> pvr_strip_window{};
    // 0.0.136 mirrors the 3-slot vertex ring with arena indices. When the
    // current strip is deferred, these indices let consecutive triangles share
    // decoded vertices instead of embedding A/B/C repeatedly.
    std::array<std::uint32_t, 3> pvr_strip_index_window{};
    std::uint32_t pvr_strip_size{};
    std::uint8_t pvr_strip_head{};
    bool pvr_strip_indexed{};
    std::uint32_t pvr_strip_arena_start{};
    // 0.0.152: correlate rejected vertices with the exact strip they shorten.
    std::uint64_t pvr_strip_bad_start{};
    std::uint64_t pvr_strip_packet_start{};
    bool pvr_strip_correlation_active{};
    // 0.0.143: while replaying a captured TA stream, polygon/texture state
    // cannot mutate between vertices. Resolve the deferred state once at the
    // first triangle of a strip instead of re-querying it for every triangle.
    // Immediate (non-staged) TA submission intentionally keeps the older path,
    // because guest VRAM/palette writes can occur between individual vertices.
    std::uint32_t pvr_strip_deferred_state_index{0xFFFFFFFFu};
    std::uint8_t pvr_strip_deferred_mode{}; // 0 unresolved, 1 GPU, 2 MT, 3 translucent
    // 0.0.144: GPU unit-Z eligibility is a vertex property. Track it once as
    // vertices enter a staged strip instead of re-testing the two overlapping
    // vertices again for every triangle emitted by that strip.
    bool pvr_strip_gpu_unit_z_ok{true};
    // 0.0.146: only enabled when the typed staged parser has pre-scanned a
    // complete strip through EndOfStrip. This keeps incomplete/abandoned runs
    // on the exact 0.0.144 per-vertex triangulation path.
    bool pvr_strip_bulk_defer{};
    // Translucent polygons are registered by the TA first and resolved by the
    // ISP/TSP later. Keep them until the scene boundary so software rendering
    // can reproduce the Dreamcast's far-to-near translucent ordering instead
    // of incorrectly using FIFO submission order.
    // 0.0.136: one contiguous decoded-vertex arena per logical frame.
    std::vector<PVRSoftVertex> pvr_deferred_vertices;
    // 0.0.148: opaque/punch-through triangles are stored once as their final
    // triangle-list indices plus compact state runs. This removes the 32-byte
    // PVRDeferredTriangle write and later struct->index expansion for ~90% of
    // Mouse Mania geometry while preserving exact submission order.
    std::vector<std::uint32_t> pvr_deferred_opaque_indices;
    std::vector<PVRDeferredOpaqueRun> pvr_deferred_opaque_runs;
    std::vector<PVRDeferredTriangle> pvr_deferred_translucent;
    std::vector<PVRDeferredState> pvr_deferred_states;
    std::uint64_t pvr_deferred_order{};
    // Last-surface cache for the common triangle-strip path. It is valid only
    // while surface id and texture-affecting epochs remain unchanged.
    std::uint64_t pvr_deferred_cached_surface{~0ull};
    std::uint64_t pvr_deferred_cached_vram_epoch{};
    std::uint64_t pvr_deferred_cached_palette_epoch{};
    bool pvr_deferred_cached_background{};
    std::uint32_t pvr_deferred_cached_state_index{0xFFFFFFFFu};
    bool pvr_mt_enabled{};
    bool pvr_mt_frame_active{};
    std::uint32_t pvr_mt_worker_count{};
    std::uint64_t pvr_mt_frames{};
    std::uint64_t pvr_mt_render_ns{};
    // 0.0.96: Direct3D 11 raster + direct DXGI scanout. Display scenes stay on
    // the GPU all the way to the host window: render target -> GPU snapshot ->
    // swapchain backbuffer -> Present. CPU readback is now reserved for cases
    // that genuinely need host pixels (RTT, dumps, or a GDI fallback). The old
    // readback-every-frame path remains opt-in through --pvr-gpu-readback for
    // clean A/B performance testing. --pvr-mt remains the full-frame fallback.
    bool pvr_gpu_enabled{};
    bool pvr_gpu_force_readback{};
    // 0.0.126: command-line/probe eligibility is immutable during ordinary
    // gameplay, so do not re-evaluate the full probe predicate per triangle.
    bool pvr_gpu_fastpath_ready{};
    bool pvr_mt_fastpath_ready{};
    bool pvr_gpu_frame_supported{true};
    bool pvr_gpu_frame_active{};
    bool pvr_gpu_ready{};
    bool pvr_gpu_failed{};
    bool pvr_gpu_present_pending{};
    bool pvr_gpu_scanout_valid{};
    std::uint64_t pvr_gpu_frames{};
    std::uint64_t pvr_gpu_fallback_frames{};
    std::uint64_t pvr_gpu_queued_triangles{};
    // 0.0.136 indexed-deferred diagnostics. `vertex_refs` counts the legacy
    // three-per-triangle references, while `unique_vertices` counts actual
    // PVRSoftVertex objects retained in the frame arena.
    std::uint64_t pvr_indexed_unique_vertices{};
    std::uint64_t pvr_indexed_vertex_refs{};
    // Perf-only usage histogram for the specialized TA vertex decoders. Keeping
    // it conditional on perf mode avoids a counter increment in normal play.
    std::array<std::uint64_t, 15> pvr_vertex_decoder_packets{};
    std::uint64_t pvr_vertex_decoder_fallbacks{};
    std::uint64_t pvr_gpu_draw_calls{};
    // 0.0.117: deferred-state compaction diagnostics.
    std::uint64_t pvr_deferred_state_captures{};
    std::uint64_t pvr_deferred_state_reuses{};
    // 0.0.118: PVR CPU profiler piggybacks on the existing perf sampler.
    // Only one TA packet per perf stride is timed; nested state/geometry work
    // is timed only while that sampled packet is active, keeping the hot path
    // free of clock calls on the other packets.
    std::uint64_t pvr_perf_next_packet{};
    std::uint64_t pvr_perf_ta_sample_ns{};
    std::uint64_t pvr_perf_state_sample_ns{};
    std::uint64_t pvr_perf_geom_sample_ns{};
    std::uint64_t pvr_perf_ta_samples{};
    std::uint64_t pvr_perf_state_samples{};
    std::uint64_t pvr_perf_geom_samples{};
    bool pvr_perf_packet_sample_active{};
    // 0.0.116: count actual D3D11 state/constant updates after redundant-bind elimination.
    std::uint64_t pvr_gpu_constant_updates{};
    std::uint64_t pvr_gpu_blend_binds{};
    std::uint64_t pvr_gpu_depth_binds{};
    std::uint64_t pvr_gpu_sampler_binds{};
    std::uint64_t pvr_gpu_srv_binds{};
    std::uint64_t pvr_gpu_texture_uploads{};
    std::uint64_t pvr_gpu_texture_reuses{};
    std::uint64_t pvr_gpu_unsupported_triangles{};
    std::uint64_t pvr_gpu_render_ns{};
    // 0.0.126 profile-only frame-stage timing. Four clock reads per rendered
    // frame when --perf is enabled; zero timing overhead in the normal runner.
    std::uint64_t pvr_gpu_sort_ns{};
    std::uint64_t pvr_gpu_build_ns{};
    std::uint64_t pvr_gpu_upload_ns{};
    std::uint64_t pvr_gpu_draw_ns{};
    std::uint64_t pvr_gpu_readback_ns{};
    std::uint64_t pvr_gpu_readback_frames{};
    std::uint64_t pvr_gpu_scanout_copies{};
    // 0.0.183: CPU/MT fallback frames uploaded into the DXGI scanout so a
    // GPU->CPU fallback cannot leave the last GPU frame frozen on screen.
    std::uint64_t pvr_gpu_cpu_scanout_uploads{};
    std::uint64_t pvr_gpu_present_calls{};
    std::uint64_t pvr_gpu_present_skips{};
    std::uint64_t pvr_gpu_present_ns{};
    std::string pvr_gpu_error;
    std::uint64_t pvr_translucent_deferred{};
    std::uint64_t pvr_translucent_flushes{};
    std::uint64_t pvr_translucent_peak{};
    PVRSoftState pvr_state{};
    std::uint32_t pvr_scene_begin_target{};
    std::uint32_t pvr_scene_begin_txr_target{};
    std::uint32_t pvr_scene_begin_rtt_target{};
    bool pvr_pending_scene_rtt{};
    std::uint32_t pvr_pending_rtt_address{};
    std::uint32_t pvr_pending_rtt_width{};
    std::uint32_t pvr_pending_rtt_height{};
    std::uint32_t pvr_pending_rtt_stride{};
    bool pvr_active_scene_rtt{};
    std::uint32_t pvr_active_rtt_address{};
    std::uint32_t pvr_active_rtt_width{};
    std::uint32_t pvr_active_rtt_height{};
    std::uint32_t pvr_active_rtt_stride{};
    std::uint8_t pvr_current_list{0xFFu};
    std::uint8_t pvr_last_ended_list{0xFFu};
    std::uint32_t pvr_ended_list_mask{};
    bool pvr_list_open{};
    bool pvr_frame_has_geometry{};
    bool pvr_background_applied{};
    bool pvr_background_attempted{};
    bool pvr_render_pending{};
    bool pvr_render_busy{};
    bool pvr_render_completed{};
    // 0.0.192: hardware-visible ISP/TSP completion can be scheduled in guest
    // SH-4 cycles instead of being raised synchronously inside STARTRENDER.
    bool pvr_render_done_scheduled_mode{};
    bool pvr_render_done_pending{};
    std::uint64_t pvr_render_done_due_cycle{};
    std::uint64_t pvr_render_done_scheduled_count{};
    std::uint64_t pvr_render_done_fired_count{};
    std::uint64_t pvr_render_done_last_delay_cycles{};
    std::uint64_t pvr_render_done_last_ta_bytes{};
    std::uint64_t pvr_render_done_late_cycles{};
    std::uint64_t pvr_last_parsed_ta_bytes{};
    bool pvr_frame_sync_window{};
    std::string pvr_dump_path;
    bool pvr_window_enabled{};
    bool pvr_window_closed{};
    std::uint64_t pvr_window_present_interval{256};
    std::uint64_t pvr_window_last_present_packet{};
    std::uint32_t pvr_window_scale{1};
    std::uint32_t pvr_window_throttle_ms{0};
    std::uint32_t pvr_window_target_fps{60};
    std::uint64_t pvr_window_next_present_ns{};
    std::uint64_t pvr_window_last_pump_ns{};
    std::uint64_t pvr_window_pump_interval_ns{10000000ull}; // 0.0.199: wall-clock cap ~100 Hz
    // 0.0.163: cheap guest-cycle gate avoids steady_clock/PeekMessage on every
    // full SH-4 scheduler quantum without changing Dreamcast timing.
    std::uint64_t pvr_window_pump_cycle_accum{};
    std::uint64_t pvr_window_pump_cycle_interval{2000000ull}; // 0.0.199: 10 ms at 200 MHz
    std::uint64_t pvr_window_service_checks{};
    std::uint64_t pvr_window_service_pumps{};
    std::uint64_t pvr_window_aica_service_slices{};
    void* pvr_window_handle{};
    bool pvr_profile{};
    // Pixel-level diagnostic counters are intentionally disabled during normal
    // gameplay in 0.0.90. They are automatically enabled by --pvr-profile,
    // which keeps probes exhaustive without paying several counter updates for
    // every covered/filtered pixel in the fast runner.
    bool pvr_detailed_stats{};
    std::uint64_t pvr_profile_start_ns{};
    std::uint64_t pvr_profile_raster_ns{};
    std::uint64_t pvr_profile_present_ns{};
    std::uint64_t pvr_profile_clear_ns{};
    // Decoded-texture cache. 0.0.90 grows the direct-mapped cache and, more
    // importantly, remembers the already-bound surface. ChuChu Rocket submits
    // ~20 triangles per polygon header; rescanning texture VRAM pages and
    // rehashing the same texture once per triangle was a major software-PVR
    // cost even on cache hits.
    // 0.0.115: the old 64-entry direct-mapped decoded-texture cache collided
    // heavily in commercial scenes with many repeated animated objects. Use a
    // 4-way set-associative 128-entry cache instead: the active index still fits
    // in uint8_t while retaining substantially more decoded textures.
    static constexpr std::size_t kPvrTextureCacheWays = 4u;
    static constexpr std::size_t kPvrTextureCacheSets = 32u;
    static constexpr std::size_t kPvrTextureCacheSize = kPvrTextureCacheWays * kPvrTextureCacheSets;
    static_assert((kPvrTextureCacheSets & (kPvrTextureCacheSets - 1u)) == 0u);
    std::array<PVRTextureCacheEntry, kPvrTextureCacheSize> pvr_texture_cache{};
    std::array<std::uint8_t, kPvrTextureCacheSets> pvr_texture_cache_next_way{};
    std::uint8_t pvr_texture_cache_active{0xFFu};
    std::uint64_t pvr_texture_cache_bound_surface{};
    std::uint64_t pvr_texture_cache_bound_vram_epoch{};
    std::uint64_t pvr_texture_cache_bound_palette_epoch{};
    std::uint64_t pvr_texture_cache_fast_reuses{};
    static constexpr std::size_t kPvrVramPageShift = 12u;
    static constexpr std::size_t kPvrVramPageSize = 1u << kPvrVramPageShift;
    static constexpr std::size_t kPvrVramPageCount = kVramSize / kPvrVramPageSize;
    static constexpr std::size_t kPvrTextureDirtyWords = (kPvrTextureCacheSize + 63u) / 64u;
    std::uint64_t pvr_vram_write_epoch{1u};
    std::array<std::uint64_t, kPvrVramPageCount> pvr_vram_page_epoch{};
    // Page -> cached-texture membership. A VRAM write ORs the touched pages'
    // 128-bit membership into pvr_texture_dirty_mask, so texture lookup never
    // needs to rescan all pages just to discover whether its bytes changed.
    std::array<std::array<std::uint64_t, kPvrTextureDirtyWords>, kPvrVramPageCount>
        pvr_texture_page_slots{};
    std::array<std::uint64_t, kPvrTextureDirtyWords> pvr_texture_dirty_mask{};
    std::uint64_t pvr_palette_write_epoch{1u};
    std::uint64_t pvr_texture_cache_hits{};
    std::uint64_t pvr_texture_cache_misses{};
    std::uint64_t pvr_texture_cache_decodes{};
    std::uint64_t pvr_texture_cache_texels{};
    std::uint64_t pvr_texture_region_write_pages{};
    std::uint64_t pvr_texture_region_new_dirty{};
    std::uint64_t pvr_texture_region_clean_hits{};
    std::uint64_t pvr_texture_region_dirty_refreshes{};
    // Hot PVR registers used by the software sampler. Keeping compact mirrors
    // avoids unordered_map lookups for every paletted/punch-through texel.
    std::array<std::uint32_t, 1024> pvr_palette_raw{};
    std::uint8_t pvr_palette_format{};
    std::uint32_t pvr_texture_stride_pixels_reg{};
    std::uint8_t pvr_alpha_ref{0xFFu};
    std::uint64_t mmio_reads{};
    std::uint64_t mmio_writes{};
    std::uint64_t pvr_mmio_reads{};
    std::uint64_t pvr_mmio_writes{};
    std::uint64_t qacr_writes{};
    std::uint64_t sq_writes{};
    std::uint64_t pref_calls{};
    std::uint64_t pref_sq_calls{};
    std::uint64_t pref_ta_commits{};
    // 0.0.136: local and whole-function producer analysis can route guarded
    // PREF sites through the TA-specialized helper. Runtime validation remains
    // authoritative, so false-positive flow candidates are semantics-neutral.
    std::uint64_t pref_ta_native_hits{};
    std::uint64_t pref_ta_native_fallbacks{};
    std::uint64_t pref_ta_native_local_hits{};
    std::uint64_t pref_ta_native_local_fallbacks{};
    std::uint64_t pref_ta_native_flow_hits{};
    std::uint64_t pref_ta_native_flow_fallbacks{};
    // 0.0.156: complete 32-byte SQ producer fusion. Normal gameplay keeps
    // per-packet hit accounting disabled; exact hits/capture counts are only
    // collected under --perf-profile so diagnostics cannot become the hot path.
    std::uint64_t sq_ta_fusion_sites{};
    std::uint64_t sq_ta_fusion_profile_hits{};
    std::uint64_t sq_ta_fusion_profile_captures{};
    std::uint64_t sq_ta_fusion_fallbacks{};
    std::uint64_t sq_ta_zero_copy_profile_hits{};
    std::uint64_t pref_texture_commits{};
    std::uint64_t pref_other_commits{};
    std::uint32_t last_pref_address{};
    std::uint32_t last_pref_target{};
    std::uint32_t last_pref_source_pc{};
    std::uint32_t last_pvr_mmio_offset{};
    std::uint32_t last_pvr_mmio_value{};
    std::uint64_t pvr_list_init_writes{};
    std::uint64_t pvr_list_cont_writes{};
    std::uint32_t pvr_ta_render_pass{};
    // 0.0.194-rebased: compact mirror for TA_ISP_CURRENT (0x005F8138).
    std::uint32_t pvr_ta_current_pointer{};
    std::uint64_t pvr_ta_guest_pointer_advances{};
    std::uint64_t pvr_list_end_irqs{};
    std::uint64_t pvr_render_done_irqs{};
    std::uint64_t pvr_background_planes{};
    std::uint64_t pvr_background_failures{};
    std::uint64_t pvr_fb_read_base_writes{};
    std::uint64_t pvr_isp_start_writes{};
    std::uint64_t sq_commits{};
    std::uint64_t ta_packets{};
    std::uint64_t pvr_vertices{};
    std::uint64_t pvr_triangles{};
    // ISP/TSP state actually used by rasterized triangles. Keeping these as
    // cumulative counters makes retail gameplay logs useful even when the live
    // image itself is too corrupted to navigate visually.
    std::array<std::uint64_t, 8> pvr_depth_mode_triangles{};
    std::array<std::uint64_t, 4> pvr_cull_mode_triangles{};
    std::array<std::uint64_t, 5> pvr_list_triangles{};
    std::array<std::uint64_t, 15> pvr_vertex_type_triangles{};
    std::uint64_t pvr_offset_color_triangles{};
    std::uint64_t pvr_mipmapped_triangles{};
    std::uint64_t pvr_mipmapped_samples{};
    std::uint64_t pvr_mipmapped_cache_decodes{};
    std::uint64_t pvr_perspective_triangles{};
    // 0.0.83 3D pipeline diagnostics. "3D" here means a non-background
    // triangle whose submitted inverse-W varies across its vertices; this is
    // deliberately heuristic but cleanly separates ChuChu's screen-space UI
    // from its projected scene in the retail traces.
    std::uint64_t pvr_3d_triangles{};
    std::uint64_t pvr_2d_triangles{};
    std::uint64_t pvr_3d_onscreen_triangles{};
    std::uint64_t pvr_3d_offscreen_triangles{};
    std::uint64_t pvr_3d_pixels_covered{};
    std::uint64_t pvr_3d_depth_pass_pixels{};
    std::uint64_t pvr_3d_depth_fail_pixels{};
    std::uint64_t pvr_3d_pixels_written{};
    std::array<std::uint64_t, 8> pvr_3d_depth_mode_triangles{};
    bool pvr_3d_z_seen{};
    float pvr_3d_z_min{};
    float pvr_3d_z_max{};
    std::uint64_t pvr_background_probe_skips{};
    std::array<std::uint64_t, 4> pvr_color_format_triangles{};
    std::array<std::uint64_t, 4> pvr_type7_color_format_triangles{};
    std::array<std::uint64_t, 4> pvr_type7_shading_triangles{};
    std::array<std::uint64_t, 5> pvr_type7_list_triangles{};
    std::uint64_t pvr_blend_src_othercolor_triangles{};
    std::uint64_t pvr_blend_dst_othercolor_triangles{};
    std::uint64_t pvr_modifier_volume_headers{};
    std::uint64_t pvr_modifier_volume_vertices{};
    std::uint64_t pvr_modifier_volume_packets_skipped{};
    std::uint64_t pvr_user_clip_commands{};
    std::uint64_t pvr_object_list_set_commands{};
    // TA stream diagnostics. A retail game producing millions of Parameter
    // Type 2 packets is much more likely to be a framing/source problem than
    // legitimate OBJECT LIST SET traffic, so keep source and type histograms.
    std::array<std::uint64_t, 8> pvr_param_type_packets{};
    std::array<std::uint64_t, 4> pvr_packet_source_counts{};
    std::array<std::uint64_t, 4> pvr_objset_source_counts{};
    // If a supposedly TYPE2 SQ packet is really an adjacent-dword half swap,
    // dword1 often contains the PCW that should have been dword0. Keep its
    // apparent parameter type so 0.0.83 can verify the FMOV64 pair-order fix.
    std::array<std::uint64_t, 8> pvr_sq_objset_word1_types{};
    // Per-ingress TA histograms. Index 1=Store Queue, 2=CH2 DMA,
    // 3=direct CPU and 0=unknown, matching PVRPacketSource.
    std::array<std::array<std::uint64_t, 8>, 4> pvr_param_type_by_source{};
    std::array<std::uint64_t, 4> pvr_surface_source_counts{};
    std::array<std::uint64_t, 4> pvr_vertex_source_counts{};
    std::array<std::uint64_t, 4> pvr_triangle_source_counts{};
    std::array<std::uint64_t, 4> pvr_onscreen_triangle_source_counts{};
    // Long parameters started on each ingress path:
    // [source][0=header64,1=vertex64,2=sprite64,3=modvol64].
    std::array<std::array<std::uint64_t, 4>, 4> pvr_long_source_counts{};
    // 0.0.151: TA decoder-lock diagnostics. PCW Object Control owns the
    // parser format; ISP_TSP's duplicate Texture bit is observed only so live
    // logs can prove when the old replay path would have selected another type.
    std::uint64_t pvr_ta_objctrl_isp_texture_conflicts{};
    bool pvr_ta_objctrl_first_conflict_logged{};
    std::uint32_t pvr_last_global_pcw{};
    std::uint32_t pvr_last_global_isp{};
    std::uint32_t pvr_last_global_tsp{};
    std::uint32_t pvr_last_global_tcw{};
    std::uint64_t pvr_surface_sequence{};
    std::uint64_t pvr_wireframe_triangles{};
    std::uint64_t pvr_wireframe_pixels{};
    std::uint64_t pvr_source_filter_skips{};
    std::array<std::unordered_map<std::uint32_t, std::uint64_t>, 4> pvr_objset_pc_hist{};
    std::array<std::uint32_t, 4> pvr_objset_top_pc{};
    std::array<std::uint64_t, 4> pvr_objset_top_pc_count{};
    std::array<std::uint32_t, 4> pvr_objset_top_pref_address{};
    std::array<std::uint32_t, 4> pvr_objset_top_pref_target{};
    std::uint8_t pvr_last_packet_source{};
    std::uint32_t pvr_last_packet_source_pc{};
    bool pvr_objset_first_logged{};
    std::array<std::uint64_t, 4> pvr_user_clip_mode_triangles{};
    std::uint64_t pvr_user_clip_pixels_rejected{};
    std::uint64_t pvr_user_clip_missing_rect{};
    std::uint64_t pvr_bad_vertices{};
    std::uint64_t pvr_bad_nonfinite_vertices{};
    std::uint64_t pvr_bad_extreme_vertices{}; // retained for old probe telemetry; live rejection is non-finite only
    // 0.0.154: Flycast accepts finite TA coordinates regardless of screen
    // magnitude and leaves clipping to the renderer. Count those formerly-
    // rejected large vertices so the next log can prove the path is active.
    std::uint64_t pvr_large_finite_vertices_accepted{};
    std::uint64_t pvr_probe_extreme_vertices_accepted{};
    bool pvr_bad_vertex_first_logged{};
    bool pvr_rendering_background{};
    std::uint64_t pvr_translucent_zwrite_suppressed{};
    std::uint64_t pvr_sprites{};
    std::uint64_t pvr_sprite_headers{};
    std::uint64_t pvr_sprite_vertex_packets{};
    std::uint64_t pvr_sprite_reused_header_vertices{};
    std::uint32_t pvr_sprite_vertices_since_header{};
    std::array<std::uint64_t, 8> pvr_sprite_tail_apparent_types{};
    std::uint64_t pvr_polygon_headers{};
    std::uint64_t pvr_vertices_without_header{};
    std::uint64_t pvr_invalid_ta_params{};
    std::uint64_t pvr_strip_starts{};
    std::uint64_t pvr_strip_eos{};
    std::uint64_t pvr_strip_completed{};
    std::uint64_t pvr_strip_short{};
    std::uint64_t pvr_strip_abandoned{};
    std::array<std::uint64_t, 4> pvr_short_bad_hist{}; // 0,1,2,3+ rejected vertices
    std::uint64_t pvr_short_bad_vertices{};
    bool pvr_short_bad_first_logged{};
    std::uint64_t pvr_texture_samples{};
    std::uint64_t pvr_bilinear_samples{};
    std::uint64_t pvr_paletted_samples{};
    std::uint64_t pvr_rtt_renders{};
    std::uint64_t pvr_frames{};
    std::uint64_t pvr_guest_render_starts{};
    std::uint64_t pvr_heuristic_render_starts{};
    std::uint64_t pvr_render_completes{};
    std::uint64_t pvr_vblanks{};
    std::uint64_t pvr_spg_status_reads{};
    // 0.0.192: SPG raster time is driven by guest SH-4 cycles, never by
    // SPG_STATUS read count. pvr_spg_epoch_cycles is reset when registers that
    // change raster timing are reprogrammed, matching Flycast CalculateSync().
    std::uint64_t pvr_spg_sh4_cycles{};
    std::uint64_t pvr_spg_epoch_cycles{};
    std::uint32_t pvr_spg_last_status{};
    std::uint32_t pvr_spg_last_scanline{};
    std::uint32_t pvr_spg_last_field{};
    std::uint32_t pvr_spg_last_line_cycles{};
    std::uint32_t pvr_spg_last_frame_cycles{};
    std::uint64_t pvr_spg_resyncs{};
    std::uint64_t pvr_asic_status_polls{};
    std::uint64_t pvr_page_flips{};
    std::uint64_t pvr_logical_frames{};
    std::uint64_t pvr_packet_limit{};

    bool aica_kos_hle{};
    bool aica_play_host{};
    bool aica_queue_ready{};
    std::string aica_wav_path;
    // Optional source-only CDDA capture. Unlike aica_wav_path this contains
    // exactly the decoded Red-Book PCM before AICA master volume/panning.
    std::string cdda_wav_path;
    std::vector<std::uint8_t> cdda_pcm;
    // Audio probes are diagnostic captures and must survive abrupt runner exits.
    // Keep a wall-clock snapshot cadence independent of guest/AICA timing.
    std::uint64_t audio_probe_flush_interval_ns{15000000000ull};
    std::uint64_t audio_probe_last_flush_ns{};
    std::uint64_t audio_probe_periodic_flushes{};
    std::array<AICAHostChannel, 64> aica_channels{};
    std::vector<std::uint8_t> aica_last_wave;
    std::uint64_t aica_commands{};
    std::uint64_t aica_channel_starts{};
    std::uint64_t aica_start_limit{};

    // Native AICA CPU path. The ARM7 starts held in reset and executes from
    // AICA RAM address 0 when SH-4 clears SNDREG 0x2c00 bit 0.
    dcrecomp::ARM7Context aica_arm{};
    bool aica_arm7_enabled{};
    bool aica_arm7_in_reset{true};
    std::uint64_t aica_arm7_slice{128u};
    std::uint64_t aica_arm7_boot_slice{32768u};
    std::uint64_t aica_arm7_slices{};
    std::uint64_t aica_arm7_faults{};
    std::uint64_t aica_arm7_idle_skipped_steps{};
    bool aica_arm7_idle_fast_forward{true};
    std::uint32_t aica_arm7_poll_pc{0xFFFFFFFFu};
    unsigned aica_arm7_poll_hits{};
    bool aica_arm7_polling{};
    std::uint64_t aica_arm7_poll_sh4_write_epoch{};
    std::uint64_t aica_sh4_aica_ram_write_epoch{};
    std::uint64_t aica_sh4_aica_ram_bytes_written{};
    std::uint64_t aica_arm7_release_count{};
    std::uint64_t aica_arm7_release_write_epoch{};
    std::uint64_t aica_arm7_release_bytes_written{};
    std::uint64_t aica_arm7_release_nonzero_bytes{};
    std::uint32_t aica_arm7_release_first_nonzero{0xFFFFFFFFu};
    std::array<std::uint32_t, 8> aica_arm7_release_vectors{};
    std::uint32_t aica_arm7_pc_high_water{};
    std::uint64_t aica_arm7_zero_opcodes{};
    std::uint64_t aica_arm7_zero_run{};
    std::uint64_t aica_arm7_zero_run_max{};
    std::string aica_arm7_last_detail;

    // AICA interrupt/timer state used by the native ARM7 path. Timer A/B/C
    // map to sound interrupt sources 6/7/8. The default divider mirrors the
    // 44.1 kHz base timer clock relative to the AICA ARM clock when one
    // interpreted instruction is treated as one scheduling clock.
    std::array<AICATimerState, 3> aica_timers{};
    std::uint64_t aica_timer_base_div{1024u};
    // Timer clock scale is independent from the 44.1 kHz mixer clock. 1/1 is
    // the hardware-model default; ratios such as 3/4 are useful for timing
    // validation of legacy ARM players without resampling host audio.
    std::uint64_t aica_timer_rate_num{1u};
    std::uint64_t aica_timer_rate_den{1u};
    std::uint32_t aica_scieb{};
    std::uint32_t aica_scipd{};
    std::array<std::uint32_t, 3> aica_scilv{};
    std::uint32_t aica_mcieb{};
    std::uint32_t aica_mcipd{};
    std::uint32_t aica_fiq_code{};
    std::uint64_t aica_fiq_acks{};

    // Native AICA wavetable/mixer state. Slots are programmed by the ARM7
    // through the real 0x00800000 register window; the host mixer consumes
    // those registers at 44.1 kHz instead of translating KOS commands.
    std::array<AICANativeSlot, 64> aica_native_slots{};
    // 0.0.126: avoid scanning all 64 hardware slots for every 44.1 kHz PCM
    // frame when only a handful of voices are active. Bits are maintained by
    // the same key-on/end/release transitions that already own slot.playing.
    std::uint64_t aica_native_active_mask{};
    std::uint8_t aica_monitor_slot{};
    // Cache the global master-volume transform. The old mixer re-read the
    // register map and called exp2() once per PCM frame even when MVOL never
    // changed (normally 0xF for KOS/ChuChu).
    std::uint32_t aica_master_reg{};
    double aica_master_gain{};
    bool aica_master_mono{};
    std::vector<std::uint8_t> aica_native_pcm; // stereo signed PCM16, no WAV header
    std::uint64_t aica_native_frames{};
    std::uint64_t aica_native_nonzero_frames{};
    std::uint64_t aica_native_audio_step_accumulator{};
    std::uint64_t aica_native_arm_steps_per_frame{512u};
    std::uint64_t aica_native_capture_limit_frames{};
    std::uint32_t aica_native_unsupported_formats{};
    std::uint64_t aica_native_bad_reads{};
    std::uint64_t aica_native_release_events{};
    // 0.0.91 host-paced native mixer. The ARM7 firmware continues to update
    // slot registers, but PCM production no longer depends on interpreting
    // 22.5792 MHz worth of ARM instructions in wall time. This is what keeps
    // the 44.1 kHz WinMM stream continuous while the guest is still below
    // full speed.
    std::uint64_t aica_host_mix_anchor_ns{};
    std::uint64_t aica_host_mix_fraction{};
    std::uint64_t aica_host_mix_frames{};
    std::uint64_t aica_host_mix_dropped_frames{};
    std::uint64_t aica_host_mix_max_batch{};
    bool aica_host_mix_enabled{};

    // Legacy SH-4 -> ARM7 instruction-count scheduler retained for regression
    // comparison. 0.0.34 added a common Dreamcast device-time scheduler below;
    // new live helpers no longer use this ratio as their primary clock.
    bool aica_continuous_clock{true};
    std::uint64_t aica_sh4_instruction_accumulator{};
    std::uint64_t aica_sh4_instructions_per_arm{4u};

    // Common Dreamcast device timeline. SH-4 work advances this clock in
    // estimated I-clock cycles. PVR/VBlank is scheduled exclusively from guest
    // SH-4 time so a slow native host cannot inject interrupts too early. Live
    // host synchronization is reserved for AICA/audio progression.
    bool device_clock_enabled{};
    bool device_clock_host_sync{};
    std::uint64_t device_clock_sh4_hz{200000000u};
    std::uint64_t device_clock_aica_hz{22579200u};
    std::uint64_t device_clock_sh4_cycles{};
    std::uint64_t device_clock_aica_fraction{};
    std::uint64_t device_clock_aica_steps{};
    std::uint64_t device_clock_pvr_target_cycles{};
    std::uint64_t device_clock_pvr_fraction{};
    bool device_clock_pvr_started{};
    std::uint64_t device_clock_pvr_anchors{};
    std::uint64_t device_clock_idle_cycles{};
    std::uint64_t device_clock_host_anchor_ns{};
    std::uint64_t device_clock_host_target_steps{};
    std::uint64_t device_clock_host_syncs{};
    // 0.0.199: host AICA synchronization is deadline-driven instead of a
    // fixed bitmask crossing. 524288 SH-4 cycles = ~2.62 ms at 200 MHz;
    // explicit syncs (Present/sleep) re-arm this deadline so periodic work
    // cannot immediately repeat it.
    std::uint64_t device_clock_host_sync_quantum_cycles{524288u};
    std::uint64_t device_clock_host_next_sync_cycle{};
    std::uint64_t device_clock_host_catchup_steps{};
    std::uint64_t device_clock_host_catchup_ns{};
    std::uint64_t device_clock_host_dropped_steps{};
    // 0.0.91: host AICA firmware scheduling distinguishes useful ARM7 work
    // from elapsed timer/idle clocks. Timer-only spans can be fast-forwarded
    // to the next enabled FIQ instead of either interpreting millions of wait
    // instructions or silently dropping the AICA timeline. The catch-up limit
    // now caps *executed* ARM7 scheduler steps per host sync, not elapsed AICA
    // time. 4096 is deliberately the same conservative execution quantum used
    // by the pre-0.0.90 scheduler, while the timer fast-forward path keeps it
    // from starving WinMM.
    std::uint64_t device_clock_host_max_catchup_steps{4096u};
    std::uint64_t device_clock_host_fast_forward_steps{};
    std::uint64_t device_clock_host_timer_events{};
    std::uint64_t device_clock_host_active_slices{};
    std::uint64_t device_clock_host_last_sh4_write_epoch{};
    std::uint64_t device_clock_pvr_ticks{};

    // Performance mode: amortize generated basic-block timing bookkeeping over
    // a small guest-cycle quantum. 64 cycles = 0.32 us at 200 MHz.
    std::uint32_t sh4_tick_batch_cycles{1u};
    std::uint64_t sh4_tick_pending_cycles{};
    std::uint64_t sh4_tick_calls{};
    std::uint64_t sh4_tick_full_calls{};

    // Live PVR/AICA bridge. SH-4 instruction count is not a clock: a light
    // video frame can execute far fewer guest instructions than a heavy one
    // while the real AICA still advances continuously. In frame-synced live
    // mode this bridge guarantees the minimum 44.1 kHz AICA progression owed
    // by each displayed frame. SH-4 scheduling remains active and may run
    // ahead; this path only fills deficits, so bootstrap/guest communication
    // is preserved while host frame pacing can no longer starve audio.
    bool aica_pvr_sync{};
    bool aica_pvr_sync_started{};
    std::uint64_t aica_pvr_sync_target_frames{};
    std::uint64_t aica_pvr_sync_fraction{};
    std::uint64_t aica_pvr_sync_topups{};
    std::uint64_t aica_pvr_sync_frames_added{};

    // Maple controller model. 0.0.35 exposes one standard controller on
    // port A / unit 0, can source state from host keyboard/XInput/DirectInput on Windows,
    // and also implements DEVINFO/GETCOND packets for the low-level DMA path.
    bool maple_host_input{};
    bool maple_controller_present{true};
    MapleHostController maple_controller{};
    // 0.1.0: optional user controller profile. When no profile is loaded the
    // backend-neutral keyboard + XInput/DirectInput mapping is used automatically.
    bool maple_controller_profile_loaded{};
    std::string maple_controller_profile_path;
    std::string maple_controller_backend{"auto"};
    std::uint32_t maple_controller_device{};
    float maple_controller_deadzone{0.18f};
    std::unordered_map<std::string, std::string> maple_controller_bindings;
    // 0.0.62: a real Maple storage subdevice on A1. Retail Katana titles
    // enumerate the VMU through low-level Maple DMA, so merely exposing a
    // controller leaves save-state transitions waiting forever on a device
    // that never answers. The flash image is persisted in the runner CWD.
    bool maple_vmu_present{true};
    bool maple_vmu_initialized{};
    std::array<std::uint8_t, 128u * 1024u> maple_vmu_flash{};
    std::string maple_vmu_path{"dreamcast_vmu_a1.bin"};
    std::uint64_t maple_vmu_devinfo{};
    std::uint64_t maple_vmu_minfo{};
    std::uint64_t maple_vmu_reads{};
    std::uint64_t maple_vmu_writes{};
    std::uint64_t maple_vmu_syncs{};
    std::uint64_t maple_vmu_persist_failures{};
    std::uint32_t maple_dma_address{};
    std::uint32_t maple_enable{};
    std::uint32_t maple_state{};
    std::uint64_t maple_dma_runs{};
    std::uint64_t maple_dma_frames{};
    // Descriptor-pattern telemetry. Only START descriptors contain the
    // receive pointer + Maple packet. Retail Katana DMA lists also contain
    // one-word NOP/reset/occupy descriptors which must never be parsed as
    // transfers.
    std::uint64_t maple_desc_start{};
    std::uint64_t maple_desc_nop{};
    std::uint64_t maple_desc_other{};
    std::uint64_t maple_recv_fixups{};
    std::uint64_t maple_dma_irqs{};
    std::array<std::uint32_t, 3> holly_pending{};
    // Holly mask banks: [0]=IRQ13/level2 (0x6910), [1]=IRQ11/level4
    // (0x6920), [2]=IRQ9/level6 (0x6930); second index is NRM/EXT/ERR.
    std::array<std::array<std::uint32_t, 3>, 3> holly_masks{};
    std::uint64_t holly_irq_raised{};
    std::uint64_t holly_irq_taken{};
    std::uint32_t holly_last_intevt{};
    std::array<std::uint64_t, 3> holly_ack_writes{};
    std::uint64_t holly_vblank_ack_writes{};
    std::uint32_t holly_last_ack_a_value{};
    std::uint32_t holly_last_ack_a_pc{};

    // SH-4 DMAC channel 2 + System Bus C2 DMA path used by Katana/PVR.
    std::uint32_t dmac_sar2{};
    std::uint32_t dmac_dar2{};
    std::uint32_t dmac_dmatcr2{};
    std::uint32_t dmac_chcr2{};
    std::uint32_t dmac_dmaor{0x00008201u};
    std::uint32_t sb_c2dstat{};
    std::uint32_t sb_c2dlen{};
    std::uint32_t sb_c2dst{};
    std::uint64_t ch2_dma_runs{};
    std::uint64_t ch2_dma_bytes{};
    std::uint64_t ch2_dma_ta{};
    std::uint64_t ch2_dma_texture{};

    // G2 DMA channels at 0x005F7800. Channel 0 is the SPU/AICA path; the
    // remaining channels target expansion/G2 devices. Transfers complete
    // synchronously in the host model but preserve the hardware-visible
    // start/enable completion state and Holly event bits.
    std::array<G2DMAChannel, 4> g2_dma{};
    std::uint64_t g2_dma_runs{};
    std::uint64_t g2_dma_bytes{};
    std::array<std::uint64_t, 4> g2_dma_channel_runs{};
    std::array<std::uint64_t, 4> g2_dma_channel_bytes{};
    std::uint32_t g2_last_channel{};
    std::uint32_t g2_last_g2_addr{};
    std::uint32_t g2_last_sh4_addr{};
    std::uint32_t g2_last_size{};
    std::uint32_t g2_last_dir{};

    // Commercial GD-ROM BIOS HLE. DCR_DISC_MAP_V2 describes the physical
    // track geometry while the Katana-facing sector payload remains 2048
    // logical bytes. This supports raw 2048 tracks and Mode2/2336 or
    // Mode1/Mode2 2352 containers without embedding commercial disc data.
    std::string gd_disc_path;
    // Keep the mapped CDI open. CDDA consumes 75 raw sectors per second;
    // reopening the image for every 2352-byte sector would turn correct music
    // playback into avoidable host I/O churn. All accesses happen on the
    // emulation/audio-production thread, so a single seekable stream is enough.
    std::ifstream gd_disc_stream;
    std::uint64_t gd_iso_base{}; // V1 compatibility only
    std::uint32_t gd_fad_base{150u};
    std::uint32_t gd_sector_size{2048u}; // logical guest sector size
    std::vector<GDDiscTrack> gd_tracks;
    std::uint32_t gd_boot_track{};
    bool gd_disc_ready{};
    GDHleState gd{};
    std::uint64_t gd_requests{};
    std::uint64_t gd_execs{};
    std::uint64_t gd_sector_reads{};
    std::uint64_t gd_bytes_read{};

    // AICA RTC (physical 0x00710000-0x0071000B). Commercial Katana startup
    // reads this directly through P1/P2 aliases before higher-level time APIs.
    std::uint32_t aica_rtc_seconds{1577836800u};
    std::uint32_t aica_rtc_write_latch{};
    bool aica_rtc_write_enabled{};

    std::uint64_t maple_devinfo_responses{};
    std::uint64_t maple_getcond_responses{};
    std::uint64_t maple_enum_calls{};
    std::uint64_t maple_status_polls{};
    std::uint64_t maple_host_polls{};
    std::uint64_t maple_xinput_polls{};
    std::uint64_t maple_winmm_polls{};
    std::uint64_t maple_keyboard_polls{};
    bool maple_input_focused{true};
    std::uint64_t maple_unfocused_polls{};
    std::uint16_t maple_keyboard_buttons{};
    std::uint16_t maple_xinput_buttons{};
    std::uint16_t maple_winmm_buttons{};
    std::string maple_host_backend{"none"};
    std::uint32_t maple_host_device{};
    std::uint16_t maple_buttons_seen_interval{};
    std::uint64_t maple_input_changes{};
    // Keep the most recent low-level Maple request visible in heartbeats.
    // This is especially useful when a retail title starts probing VMU units
    // after the controller path is already known-good.
    std::uint8_t maple_last_cmd{};
    std::uint8_t maple_last_dst{};
    std::uint8_t maple_last_src{};
    std::int8_t maple_last_unit{-1};
    std::uint32_t maple_last_function{};
    std::uint64_t maple_noncontroller_frames{};
    std::uint32_t maple_guest_device{0x8C7FF000u};
    std::uint32_t maple_guest_state{0x8C7FF100u};

    // Deterministic probe state retained for regression/smoke helpers.
    std::uint32_t probe_controller_poll{};
    bool probe_controller_a_then_start_lowlevel{};
    std::uint32_t probe_start_burst_poll{};
    std::uint32_t probe_start_burst_presses{};
    bool probe_controller_start_burst_lowlevel{};
    std::uint32_t probe_sfx_load_count{};

    std::uint32_t guest_heap_next{0x8C800000u};
    std::uint32_t guest_heap_end{0x8CE00000u};
    std::unordered_map<std::uint32_t, std::uint32_t> guest_alloc_sizes;
    std::uint32_t current_pc{};
    bool trace_calls{};
    bool trace_history_enabled{};
    // 0.0.207 diagnostic-only SH-4 ABI checker. When enabled the fast dynamic
    // dispatch path is disabled so every indirect call can verify callee-saved
    // R8-R14 and SP on an ordinary return. Non-local/context-switch returns are
    // deliberately skipped.
    bool sh4_abi_audit{};
    std::uint64_t sh4_abi_checks{};
    std::uint64_t sh4_abi_violations{};
    // Retain a short call history only when explicitly requested. Commercial bootstrap code can make
    // tens of thousands of tiny helper calls (for example while decoding the
    // IP.BIN graphics), so printing every CALL/RET can dominate execution.
    // The rolling history gives useful crash context without console spam.
    std::array<TraceCallEvent, 32> trace_history{};
    std::size_t trace_history_next{};
    std::size_t trace_history_size{};
    std::uint64_t trace_call_count{};
    bool commercial_boot_active{};
    bool ct2_compat{};
    // Set when a device event redirects SH-4 execution to VBR+0x600. Generated
    // nested C++ calls propagate this back to the top-level dispatcher so a
    // host call/return cannot overwrite the asynchronous guest PC.
    bool sh4_async_redirect{};

    // Host-only live diagnostics. The status window is intentionally separate
    // from PVR so direct-framebuffer/audio demos still have visible liveness
    // without coupling AICA timing to presentation.
    bool host_status_window_enabled{};
    bool diag_heartbeat_enabled{};
    std::uint64_t diag_heartbeat_interval_ns{1000000000ull};
    std::uint64_t diag_heartbeat_cycle_accum{};
    std::uint64_t diag_heartbeat_last_ns{};
    std::uint64_t diag_heartbeat_count{};
    std::uint64_t diag_heartbeat_prev_ns{};
    std::uint64_t diag_heartbeat_prev_flips{};
    std::uint64_t bfont_fast_calls{};

    // 0.0.90 low-overhead performance profiler. Profiling is opt-in and
    // samples one full runtime tick every N calls so steady_clock/hash-map
    // instrumentation does not become the bottleneck we are trying to measure.
    bool perf_profile_enabled{};
    std::uint32_t perf_sample_stride{67u};
    std::uint32_t perf_sample_countdown{67u};
    std::uint64_t perf_start_ns{};
    std::uint64_t perf_tick_counter{};
    std::uint64_t perf_samples{};
    std::uint64_t perf_tick_sampled_ns{};
    std::uint64_t perf_host_ui_sampled_ns{};
    std::uint64_t perf_tmu_gd_sampled_ns{};
    std::uint64_t perf_device_sampled_ns{};
    // 0.0.123: split the old mixed DEVICE bucket. It included both guest PVR
    // VBlank scheduling and the occasional wall-clock AICA/GD host sync, so it
    // was not valid evidence that ARM7 itself was the bottleneck. These
    // counters are sampled only when --perf-profile is enabled.
    std::uint64_t perf_pvr_clock_sampled_ns{};
    std::uint64_t perf_host_sync_sampled_ns{};
    std::uint64_t perf_irq_sampled_ns{};
    std::unordered_map<std::uint32_t, std::uint64_t> perf_pc_samples;

    DCRuntime();
    void register_target(std::uint32_t address, RecompiledFunction function);
};

// 0.0.96: the generated SH-4 executes an enormous number of basic blocks.
// Keep the cheap cycle accumulator in the generated translation unit so the
// common (< batch) path does not cross into dc_runtime.cpp millions of times
// per second. Only the uncommon batch boundary enters the full scheduler.
// The normal commercial runner deliberately retains the proven 256-cycle
// quantum from 0.0.94; this optimization changes host call overhead, not the
// guest timing granularity.
bool dc_runtime_tick_full(SH4Context& ctx, DCRuntime& runtime, std::uint64_t sh4_cycles);

#if defined(_MSC_VER)
#define DCR_SH4_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define DCR_SH4_FORCE_INLINE inline __attribute__((always_inline))
#else
#define DCR_SH4_FORCE_INLINE inline
#endif

#if defined(DCR_LIGHTWEIGHT_CURRENT_PC)
#define DCR_SYNC_CURRENT_PC(runtime, ctx) ((void)0)
#else
#define DCR_SYNC_CURRENT_PC(runtime, ctx) ((runtime).current_pc = (ctx).pc)
#endif

DCR_SH4_FORCE_INLINE bool dc_runtime_tick_fast(SH4Context& ctx, DCRuntime& runtime, std::uint64_t sh4_cycles) noexcept {
#if !defined(DCR_DISABLE_HOT_TICK_CALL_COUNTER)
    ++runtime.sh4_tick_calls;
#endif
    runtime.sh4_tick_pending_cycles += sh4_cycles;
#if defined(DCR_FIXED_SH4_TICK_BATCH)
    constexpr std::uint64_t batch = static_cast<std::uint64_t>(DCR_FIXED_SH4_TICK_BATCH);
#else
    const std::uint64_t batch = runtime.sh4_tick_batch_cycles != 0u ? runtime.sh4_tick_batch_cycles : 1u;
#endif
    if (runtime.sh4_tick_pending_cycles < batch) return false;
    const std::uint64_t accumulated = runtime.sh4_tick_pending_cycles;
    runtime.sh4_tick_pending_cycles = 0u;
    ++runtime.sh4_tick_full_calls;
    return dc_runtime_tick_full(ctx, runtime, accumulated);
}

#undef DCR_SH4_FORCE_INLINE

std::uint8_t dc_read8(DCRuntime& runtime, std::uint32_t address);
std::uint16_t dc_read16(DCRuntime& runtime, std::uint32_t address);
std::uint32_t dc_read32(DCRuntime& runtime, std::uint32_t address);
std::uint32_t dc_relocate_pc_address(const DCRuntime& runtime, std::uint32_t source_address, std::uint32_t address);
std::uint32_t dc_load_pc_literal32(DCRuntime& runtime, std::uint32_t source_address, std::uint32_t storage_address, std::uint32_t baked_value);
std::uint32_t dc_load_pc_literal16(DCRuntime& runtime, std::uint32_t source_address, std::uint32_t storage_address, std::uint16_t baked_value);
void dc_write8(DCRuntime& runtime, std::uint32_t address, std::uint8_t value);
void dc_write16(DCRuntime& runtime, std::uint32_t address, std::uint16_t value);
void dc_write32(DCRuntime& runtime, std::uint32_t address, std::uint32_t value);

// 0.0.112 low-memory hot path: keep the optimized SDRAM/Store-Queue logic
// compiled once in dc_runtime.cpp instead of force-inlining it into thousands
// of generated SH-4 call sites. This preserves the fast address classifier
// while preventing MSVC /O2 from exploding optimizer IR/RAM usage.
std::uint32_t dc_read32_hot(DCRuntime& runtime, std::uint32_t address);
void dc_write32_hot(DCRuntime& runtime, std::uint32_t address, std::uint32_t value);
// 0.0.115: isolated A/B experiment retained from 0.0.113. SZ=1 FMOV
// moves two adjacent 32-bit lanes; classify the address once per 64-bit move.
std::uint64_t dc_read64_fmov_hot(DCRuntime& runtime, std::uint32_t address);
void dc_write64_fmov_hot(DCRuntime& runtime, std::uint32_t address, std::uint64_t bits);
void dc_load_bytes(DCRuntime& runtime, std::uint32_t address, const std::uint8_t* data, std::size_t size);
void dc_zero_bytes(DCRuntime& runtime, std::uint32_t address, std::size_t size);
void dc_setup_commercial_boot(SH4Context& ctx, DCRuntime& runtime);
void dc_gdrom_load_disc_map(DCRuntime& runtime, const std::string& path);
void dc_gdrom_tick(DCRuntime& runtime, std::uint64_t sh4_cycles);
void dc_gdrom_sync_host(DCRuntime& runtime, std::uint64_t now_ns);
void dc_gdrom_print_stats(const DCRuntime& runtime);
void dc_trace_print_history(const DCRuntime& runtime, std::ostream& out);
void dc_write_sr(SH4Context& ctx, std::uint32_t value);
void dc_pref(DCRuntime& runtime, std::uint32_t address, std::uint32_t source_address);
void dc_pref_ta_native(DCRuntime& runtime, std::uint32_t address, std::uint32_t source_address, bool flow_site = false);
void dc_sq_ta_replay_captures(DCRuntime& runtime, const SQTAFusedCapture* captures, std::size_t count);
bool dc_pref_ta_fused_packet(DCRuntime& runtime, std::uint32_t address, std::uint32_t source_address,
                             const SQTAFusedCapture* captures, std::size_t count);
void dc_pvr_set_dump_path(DCRuntime& runtime, const std::string& path);
void dc_pvr_dump_ppm(DCRuntime& runtime, const std::string& path);
void dc_pvr_enable_window(DCRuntime& runtime, const std::string& title, std::uint32_t scale = 1u, std::uint64_t present_interval_packets = 256u, std::uint32_t throttle_ms = 0u, std::uint32_t target_fps = 60u);
void dc_pvr_present_window(DCRuntime& runtime);
void dc_pvr_close_window(DCRuntime& runtime);
void dc_host_status_enable(DCRuntime& runtime, const std::string& title);
void dc_host_status_close(DCRuntime& runtime);
void dc_diag_heartbeat_enable(DCRuntime& runtime, std::uint64_t milliseconds = 1000u);
void dc_perf_set_profile(DCRuntime& runtime, bool enabled, std::uint32_t sample_stride = 67u);
void dc_perf_print_stats(const DCRuntime& runtime);
void dc_pvr_print_stats(const DCRuntime& runtime);
void dc_pvr_set_frame_sync(DCRuntime& runtime, bool enabled);
void dc_pvr_set_profile(DCRuntime& runtime, bool enabled);
void dc_pvr_refresh_fastpaths(DCRuntime& runtime);
bool dc_pvr_vertex_decoder_selftest();
bool dc_pvr_ta_staging_selftest();
bool dc_pvr_texture_dirty_region_selftest();
void dc_maple_enable_host_input(DCRuntime& runtime, bool enabled);
bool dc_maple_load_controller_profile(DCRuntime& runtime, const std::string& path);
void dc_maple_set_controller_present(DCRuntime& runtime, bool present);
void dc_maple_set_controller_state(DCRuntime& runtime, std::uint32_t buttons,
                                   std::uint8_t ltrig = 0u, std::uint8_t rtrig = 0u,
                                   std::int8_t joyx = 0, std::int8_t joyy = 0,
                                   std::int8_t joy2x = 0, std::int8_t joy2y = 0);
void dc_maple_poll_host(DCRuntime& runtime);
void dc_maple_refresh_guest_state(DCRuntime& runtime);
void dc_maple_print_stats(const DCRuntime& runtime);
void dc_aica_enable_kos_hle(DCRuntime& runtime, bool enabled);
void dc_aica_enable_arm7(DCRuntime& runtime, bool enabled, std::uint64_t instructions_per_slice = 128u);
void dc_aica_set_arm7_boot_slice(DCRuntime& runtime, std::uint64_t instructions);
void dc_aica_set_timer_div(DCRuntime& runtime, std::uint64_t arm_steps_per_44k_tick);
void dc_aica_set_timer_rate(DCRuntime& runtime, std::uint64_t numerator, std::uint64_t denominator);
void dc_aica_set_sh4_div(DCRuntime& runtime, std::uint64_t sh4_instructions_per_arm);
void dc_aica_set_pvr_sync(DCRuntime& runtime, bool enabled);
void dc_device_clock_enable(DCRuntime& runtime, bool enabled, bool host_sync = false);
void dc_device_clock_set_rates(DCRuntime& runtime, std::uint64_t sh4_hz, std::uint64_t aica_hz);
void dc_device_clock_sync_host(DCRuntime& runtime);
void dc_device_clock_advance_ms(DCRuntime& runtime, std::uint64_t milliseconds);
void dc_aica_set_capture_ms(DCRuntime& runtime, std::uint64_t milliseconds);
void dc_aica_seed_kos_defaults(DCRuntime& runtime);
std::uint64_t dc_aica_run_arm7(DCRuntime& runtime, std::uint64_t max_instructions);
bool dc_runtime_tick(SH4Context& ctx, DCRuntime& runtime, std::uint64_t sh4_cycles);
void dc_aica_set_wav_path(DCRuntime& runtime, const std::string& path);
void dc_gdrom_set_cdda_wav_path(DCRuntime& runtime, const std::string& path);
void dc_gdrom_flush_cdda_wav(DCRuntime& runtime);
void dc_audio_probe_periodic_flush(DCRuntime& runtime, std::uint64_t now_ns);
void dc_aica_set_host_playback(DCRuntime& runtime, bool enabled);
void dc_aica_flush_wav(DCRuntime& runtime);
void dc_aica_print_stats(const DCRuntime& runtime);
void dc_gdrom_mix_cdda_frame(DCRuntime& runtime, double& mix_l, double& mix_r);
const char* gd_cdda_scramble_name(const GDHleState& gd);
const char* gd_cdda_order_name(const GDHleState& gd);
std::string dc_read_c_string(DCRuntime& runtime, std::uint32_t address, std::size_t max_length = 65536);
[[noreturn]] void dc_unimplemented(std::uint32_t source_address, const char* operation);

// 0.0.112 low-memory FPU helpers. 0.0.109/0.0.110 placed these bodies in
// dc_runtime.hpp with __forceinline, which made MSVC duplicate/optimize them at
// enormous numbers of generated call sites and pushed compile RAM toward the
// system limit. Compile one optimized implementation in dc_runtime.cpp instead.
std::uint32_t dc_get_fr_bits(const SH4Context& ctx, std::uint8_t index);
void dc_set_fr_bits(SH4Context& ctx, std::uint8_t index, std::uint32_t bits);
std::uint32_t dc_get_xf_bits(const SH4Context& ctx, std::uint8_t index);
void dc_set_xf_bits(SH4Context& ctx, std::uint8_t index, std::uint32_t bits);
float dc_get_fr_float(const SH4Context& ctx, std::uint8_t index);
void dc_set_fr_float(SH4Context& ctx, std::uint8_t index, float value);
std::uint64_t dc_get_dr_bits(const SH4Context& ctx, std::uint8_t even_index);
void dc_set_dr_bits(SH4Context& ctx, std::uint8_t even_index, std::uint64_t bits);
std::uint64_t dc_get_xd_bits(const SH4Context& ctx, std::uint8_t even_index);
void dc_set_xd_bits(SH4Context& ctx, std::uint8_t even_index, std::uint64_t bits);
std::uint64_t dc_get_fmov64_bits(const SH4Context& ctx, std::uint8_t encoded_index);
void dc_set_fmov64_bits(SH4Context& ctx, std::uint8_t encoded_index, std::uint64_t bits);
double dc_get_dr_double(const SH4Context& ctx, std::uint8_t even_index);
void dc_set_dr_double(SH4Context& ctx, std::uint8_t even_index, double value);
bool dc_fpu_double_precision(const SH4Context& ctx);
void dc_sync_host_fpu_mode(std::uint32_t fpscr);
void dc_write_fpscr(SH4Context& ctx, std::uint32_t value);
void dc_require_fpu_round_nearest(const SH4Context& ctx, std::uint32_t source_address);
void dc_require_fmov32(const SH4Context& ctx, std::uint32_t source_address);
std::uint32_t dc_ftrc_to_u32(double value);
void dc_trace_record(DCRuntime& runtime, std::uint32_t caller, std::uint32_t target, std::uint32_t r8, std::uint32_t sp);
void call_recompiled(SH4Context& ctx, DCRuntime& runtime, std::uint32_t target);

// 0.0.96 selective dynamic-dispatch fast path. The global dispatch cache was
// already hitting almost all retail targets, but DynamicCall still crossed
// translation units just to discover that hit. Keep exact crash history and
// return/async-redirect semantics while resolving the common hit inline.
#if defined(_MSC_VER)
#define DCR_DISPATCH_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define DCR_DISPATCH_FORCE_INLINE inline __attribute__((always_inline))
#else
#define DCR_DISPATCH_FORCE_INLINE inline
#endif

DCR_DISPATCH_FORCE_INLINE void dc_trace_record_inline(
    DCRuntime& runtime, std::uint32_t caller, std::uint32_t target,
    std::uint32_t r8, std::uint32_t sp) noexcept {
    if (!runtime.trace_history_enabled) return;
    runtime.trace_history[runtime.trace_history_next] = TraceCallEvent{caller, target, r8, sp};
    runtime.trace_history_next = (runtime.trace_history_next + 1u) % runtime.trace_history.size();
    if (runtime.trace_history_size < runtime.trace_history.size()) ++runtime.trace_history_size;
    ++runtime.trace_call_count;
}

DCR_DISPATCH_FORCE_INLINE void dc_refresh_dispatch_ready(DCRuntime& runtime) noexcept {
    const bool context_clear = !runtime.trace_calls && !runtime.sh4_abi_audit &&
        runtime.active_relocation_source == 0u && runtime.active_relocation_target == 0u &&
        runtime.pvr_scene_begin_target == 0u && runtime.pvr_scene_begin_txr_target == 0u &&
        runtime.pvr_scene_begin_rtt_target == 0u;
    runtime.fast_dynamic_dispatch_ready = runtime.fast_dispatch_enabled && context_clear;
    runtime.direct_dispatch_ready = runtime.direct_dispatch_enabled && context_clear;
    ++runtime.dispatch_ready_refreshes;
}

DCR_DISPATCH_FORCE_INLINE void dc_call_dynamic_fast(
    SH4Context& ctx, DCRuntime& runtime, std::uint32_t target) {
    if (runtime.fast_dynamic_dispatch_ready) {
        const std::uint32_t requested_target = target;
        auto& cached = runtime.dispatch_cache[
            (requested_target >> 1u) & (DCRuntime::kDispatchCacheSize - 1u)];
        if (cached.requested == requested_target && cached.function != nullptr &&
            cached.relocation_source == 0u && cached.relocation_target == 0u) {
            DCR_HOT_METRIC_INC(runtime.dispatch_cache_hits);
            DCR_HOT_METRIC_INC(runtime.fast_dispatch_calls);
            DCR_HOT_METRIC_INC(runtime.inline_dynamic_dispatch_hits);
            const std::uint32_t normal_return_pc = ctx.pr;
            dc_trace_record_inline(runtime, ctx.pc, requested_target, ctx.r[8], ctx.r[15]);
            target = cached.target;
            ctx.pc = target;
            cached.function(ctx, runtime);
            if (!runtime.sh4_async_redirect && ctx.pc == target) ctx.pc = normal_return_pc;
            return;
        }
    }
    ++runtime.inline_dynamic_dispatch_fallbacks;
    call_recompiled(ctx, runtime, target);
}

#undef DCR_DISPATCH_FORCE_INLINE

[[noreturn]] void dc_unimplemented(std::uint32_t source_address, const char* operation);

} // namespace dcrecomp_generated
