#include "dc_native_overrides.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace dcrecomp_generated {
namespace {

std::uint32_t printf_arg(const SH4Context& ctx, DCRuntime& runtime, std::size_t index) {
    // SH-4/KOS ABI: R4 is the format string; integer/pointer varargs start in
    // R5-R7 and spill to the caller stack. This is sufficient for diagnostic
    // printf traffic in the KOS corpus and keeps unsupported formatting from
    // becoming an execution blocker.
    if (index < 3) return ctx.r[5 + index];
    return dc_read32(runtime, ctx.r[15] + static_cast<std::uint32_t>((index - 3u) * 4u));
}

std::uint64_t printf_arg64(const SH4Context& ctx, DCRuntime& runtime, std::size_t& index) {
    const std::uint64_t lo = printf_arg(ctx, runtime, index++);
    const std::uint64_t hi = printf_arg(ctx, runtime, index++);
    return lo | (hi << 32u);
}

void native_dbglog(SH4Context& ctx, DCRuntime&) {
    // Temporary HLE service for early hardware bootstrap. Full KOS dbglog runs
    // through newlib varargs and device backends that are orthogonal to PVR MMIO.
    ctx.r[0] = 0u;
}

void native_pvr_wait_ready(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }
void native_pvr_check_ready(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }
void native_pvr_wait_render_done(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }
void native_pvr_get_stats(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint32_t stat = ctx.r[4];
    if (stat == 0u) { ctx.r[0] = static_cast<std::uint32_t>(-1); return; }
    const std::uint64_t frame_ns = 1000000000ull / std::max<std::uint64_t>(1u, runtime.pvr_window_target_fps);
    const auto put64 = [&](std::uint32_t off, std::uint64_t value) {
        dc_write32(runtime, stat + off + 0u, static_cast<std::uint32_t>(value));
        dc_write32(runtime, stat + off + 4u, static_cast<std::uint32_t>(value >> 32u));
    };
    put64(0u, frame_ns);   // frame_last_time
    put64(8u, 0u);         // reg_last_time
    put64(16u, 0u);        // rnd_last_time
    put64(24u, 0u);        // buf_last_time
    dc_write32(runtime, stat + 32u, static_cast<std::uint32_t>(runtime.pvr_page_flips));
    dc_write32(runtime, stat + 36u, static_cast<std::uint32_t>(runtime.pvr_vblanks));
    dc_write32(runtime, stat + 40u, 0u);
    dc_write32(runtime, stat + 44u, 0u);
    const float fps = static_cast<float>(runtime.pvr_window_target_fps ? runtime.pvr_window_target_fps : 60u);
    std::uint32_t fps_bits = 0u;
    std::memcpy(&fps_bits, &fps, sizeof(fps_bits));
    dc_write32(runtime, stat + 48u, fps_bits);
    dc_write32(runtime, stat + 52u, runtime.pvr_ended_list_mask);
    ctx.r[0] = 0u;
}
void native_pvr_shutdown(SH4Context& ctx, DCRuntime& runtime) {
    // The hardware shutdown path clears the full 8 MiB VRAM through SH-4
    // store queues. Replaying that cleanup loop in generated C++ is extremely
    // expensive and has no observable value once the native process exits.
    runtime.pvr_render_pending = false;
    runtime.pvr_render_busy = false;
    runtime.pvr_render_completed = false;
    runtime.pvr_sprite_mode = false;
    runtime.pvr_sprite_partial_size = 0u;
    runtime.pvr_sprite_vertices_since_header = 0u;
    ctx.r[0] = 0u;
}
void native_vblank_handler_add(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 1u; }
void native_asic_evt_set_handler(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }
void native_asic_evt_enable(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }

void native_mutex_lock(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }
void native_mutex_unlock(SH4Context& ctx, DCRuntime&) { ctx.r[0] = 0u; }

std::uint64_t native_probe_time_ns() {
    static const auto start = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count());
}
void native_return_u64(SH4Context& ctx, std::uint64_t value) { ctx.r[0] = static_cast<std::uint32_t>(value); ctx.r[1] = static_cast<std::uint32_t>(value >> 32); }
void native_timer_ns(SH4Context& ctx, DCRuntime&) { native_return_u64(ctx, native_probe_time_ns()); }
void native_timer_us(SH4Context& ctx, DCRuntime&) { native_return_u64(ctx, native_probe_time_ns() / 1000ull); }
void native_timer_ms(SH4Context& ctx, DCRuntime&) { native_return_u64(ctx, native_probe_time_ns() / 1000000ull); }
void native_timer_spin_sleep(SH4Context& ctx, DCRuntime& runtime) {
    // KOS uses this during hardware bootstrap (notably snd_init()). Treat the
    // delay as guest elapsed time so ARM7/AICA keep running while SH-4 waits.
    dc_device_clock_advance_ms(runtime, ctx.r[4]);
    ctx.r[0] = 0u;
}
void native_timer_spin_delay_ns(SH4Context& ctx, DCRuntime&) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(ctx.r[4] & 0xFFFFu));
    ctx.r[0] = 0u;
}
void native_time(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint64_t seconds = native_probe_time_ns() / 1000000000ull;
    if (ctx.r[4] != 0u) { dc_write32(runtime, ctx.r[4], static_cast<std::uint32_t>(seconds)); dc_write32(runtime, ctx.r[4] + 4u, static_cast<std::uint32_t>(seconds >> 32)); }
    native_return_u64(ctx, seconds);
}

void native_thd_sleep(SH4Context& ctx, DCRuntime& runtime) {
    // KOS thd_sleep() is expressed in milliseconds. A sleeping SH-4 thread
    // must not freeze independent devices: advance/synchronize the common
    // Dreamcast timeline so ARM7/AICA firmware gets the same elapsed time.
    dc_device_clock_advance_ms(runtime, ctx.r[4]);
    ctx.r[0] = 0u;
}

void native_assert_handler_default(SH4Context& ctx, DCRuntime& runtime) {
    const std::string file = ctx.r[4] ? dc_read_c_string(runtime, ctx.r[4], 4096) : std::string("<null>");
    const std::uint32_t line = ctx.r[5];
    const std::string expr = ctx.r[6] ? dc_read_c_string(runtime, ctx.r[6], 4096) : std::string("<null>");
    const std::string msg = ctx.r[7] ? dc_read_c_string(runtime, ctx.r[7], 4096) : std::string();
    const std::uint32_t func_ptr = dc_read32(runtime, ctx.r[15]);
    const std::string func = func_ptr ? dc_read_c_string(runtime, func_ptr, 4096) : std::string("<unknown>");
    std::ostringstream out;
    out << "KOS assertion failed: " << expr << " at " << file << ':' << line << " in " << func;
    if (!msg.empty()) out << ": " << msg;
    throw std::runtime_error(out.str());
}


void native_maple_enum_count(SH4Context& ctx, DCRuntime& runtime) {
    ++runtime.maple_enum_calls;
    ctx.r[0] = runtime.maple_controller_present ? 1u : 0u;
}

void native_maple_enum_dev(SH4Context& ctx, DCRuntime& runtime) {
    ++runtime.maple_enum_calls;
    const int port = static_cast<int>(ctx.r[4]);
    const int unit = static_cast<int>(ctx.r[5]);
    if (runtime.maple_host_input) dc_maple_poll_host(runtime);
    ctx.r[0] = runtime.maple_controller_present && port == 0 && unit == 0
             ? runtime.maple_guest_device : 0u;
}

void native_maple_enum_type(SH4Context& ctx, DCRuntime& runtime) {
    ++runtime.maple_enum_calls;
    const std::uint32_t index = ctx.r[4];
    const std::uint32_t functions = ctx.r[5];
    if (runtime.maple_host_input) dc_maple_poll_host(runtime);
    const bool type_ok = functions == 0xFFFFFFFFu || (functions & 0x01000000u) != 0u;
    ctx.r[0] = runtime.maple_controller_present && index == 0u && type_ok
             ? runtime.maple_guest_device : 0u;
}

void native_maple_dev_status(SH4Context& ctx, DCRuntime& runtime) {
    ++runtime.maple_status_polls;
    if (runtime.maple_host_input) dc_maple_poll_host(runtime);
    if (!runtime.maple_controller_present || ctx.r[4] != runtime.maple_guest_device) {
        ctx.r[0] = 0u;
        return;
    }
    dc_maple_refresh_guest_state(runtime);
    ctx.r[0] = runtime.maple_guest_state;
}

void native_maple_addr(SH4Context& ctx, DCRuntime&) {
    const int port = static_cast<int>(ctx.r[4]);
    const int unit = static_cast<int>(ctx.r[5]);
    if (port < 0 || port >= 4 || unit < 0 || unit >= 6) {
        ctx.r[0] = 0u;
        return;
    }
    std::uint32_t addr = static_cast<std::uint32_t>((port & 3) << 6);
    if (unit == 0) addr |= 0x20u;
    else addr |= 1u << (unit - 1);
    ctx.r[0] = addr;
}

void native_maple_dev_valid(SH4Context& ctx, DCRuntime& runtime) {
    const int port = static_cast<int>(ctx.r[4]);
    const int unit = static_cast<int>(ctx.r[5]);
    ctx.r[0] = runtime.maple_controller_present && port == 0 && unit == 0 ? 1u : 0u;
}

void native_probe_skip_audio(SH4Context& ctx, DCRuntime&) {
    std::cout << "[DreamcastRecomp probe] skipping AICA music bootstrap\n";
    ctx.r[0] = 0u;
}

void native_probe_no_input(SH4Context& ctx, DCRuntime&) {
    // No controller attached: enough for graphics-only homebrew probes.
    ctx.r[0] = 0u;
}

constexpr std::uint32_t kProbeMapleDevice = 0x8C7FF000u;
constexpr std::uint32_t kProbeMapleState = 0x8C7FF100u;

void native_probe_controller_enum(SH4Context& ctx, DCRuntime&) {
    // A non-null opaque device handle. maple_dev_status is also overridden in
    // this explicit probe mode, so no maple_device_t internals are fabricated.
    ctx.r[0] = kProbeMapleDevice;
}

void native_probe_controller_status(SH4Context& ctx, DCRuntime& runtime) {
    // cont_state_t begins with uint32_t buttons followed by six int fields.
    // Sequence: A press -> release -> ~long deterministic idle window -> START press. This
    // gives asynchronous device firmware enough virtual time to process the
    // application action before the deterministic probe exits.
    dc_zero_bytes(runtime, kProbeMapleState, 28u);
    if (runtime.probe_controller_poll > 0u && runtime.probe_controller_poll < 64u)
        dc_device_clock_advance_ms(runtime, 1u); // deterministic device time for async AICA firmware
    const std::uint32_t buttons = runtime.probe_controller_poll == 0u ? (1u << 2)
                                : runtime.probe_controller_poll >= 64u ? (1u << 3)
                                : 0u;
    dc_write32(runtime, kProbeMapleState, buttons);
    ++runtime.probe_controller_poll;
    ctx.r[0] = kProbeMapleState;
}

void native_probe_controller_start_burst_status(SH4Context& ctx, DCRuntime& runtime) {
    // 0.0.163 commercial smoke helper: wait for the title to begin polling the
    // high-level KOS controller API, then issue six short START pulses separated
    // by long release windows. This is intentionally opt-in and never changes
    // the normal commercial runner. Once the finite burst is complete, hand
    // control back to the regular host-input implementation.
    constexpr std::uint32_t kInitialReleasePolls = 120u;
    constexpr std::uint32_t kPulsePeriodPolls = 150u;
    constexpr std::uint32_t kPulseHoldPolls = 5u;
    constexpr std::uint32_t kMaxPulses = 6u;
    const std::uint32_t poll = runtime.probe_start_burst_poll++;
    if (poll < kInitialReleasePolls) {
        dc_zero_bytes(runtime, kProbeMapleState, 28u);
        ctx.r[0] = kProbeMapleState;
        return;
    }
    const std::uint32_t q = poll - kInitialReleasePolls;
    const std::uint32_t pulse = q / kPulsePeriodPolls;
    if (pulse >= kMaxPulses) {
        native_maple_dev_status(ctx, runtime);
        return;
    }
    const std::uint32_t phase = q % kPulsePeriodPolls;
    dc_zero_bytes(runtime, kProbeMapleState, 28u);
    if (phase < kPulseHoldPolls) {
        dc_write32(runtime, kProbeMapleState, 1u << 3);
        if (phase == 0u) {
            ++runtime.probe_start_burst_presses;
            std::cout << "[DreamcastRecomp smoke] START pulse "
                      << runtime.probe_start_burst_presses << "/" << kMaxPulses << "\n";
        }
    }
    ctx.r[0] = kProbeMapleState;
}

constexpr std::uint32_t kProbeSfxBase = 0x8C7FE000u;
constexpr std::uint32_t kProbeSfxSampleBase = 0x00030000u;

void native_probe_sfx_load(SH4Context& ctx, DCRuntime& runtime) {
    // Probe-only replacement for the romdisk file-loading part of snd_sfx_load.
    // The actual KOS snd_sfx_play/snd_sfx_play_ex/snd_sh4_to_aica path remains
    // recompiled SH-4 code. This lets the stock sound/sfx example run even
    // though the KOS startup/romdisk mount is not modeled yet.
    constexpr std::uint32_t rate = 22050u;
    constexpr std::uint32_t samples = 4410u; // 0.20 s
    const std::uint32_t slot = runtime.probe_sfx_load_count++ & 3u;
    const std::uint32_t sample_off = kProbeSfxSampleBase + slot * 0x4000u;
    const std::uint32_t effect = kProbeSfxBase + slot * 0x40u;
    if (sample_off + samples * 2u > runtime.aica_ram.size()) { ctx.r[0] = 0u; return; }
    for (std::uint32_t i = 0; i < samples; ++i) {
        // Four distinguishable square-wave pitches for the four example assets.
        const std::uint32_t period = 50u - slot * 6u;
        const std::int16_t v = ((i % period) < period / 2u) ? 9000 : -9000;
        runtime.aica_ram[sample_off + i * 2u + 0u] = static_cast<std::uint8_t>(v & 0xFF);
        runtime.aica_ram[sample_off + i * 2u + 1u] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(v) >> 8) & 0xFFu);
    }
    dc_zero_bytes(runtime, effect, 36u);
    dc_write32(runtime, effect + 0u, sample_off);  // locl
    dc_write32(runtime, effect + 4u, 0u);           // locr
    dc_write32(runtime, effect + 8u, samples);      // len in samples
    dc_write32(runtime, effect + 12u, rate);        // rate
    dc_write32(runtime, effect + 16u, 0u);          // used
    dc_write32(runtime, effect + 20u, 0u);          // AICA_SM_16BIT
    dc_write16(runtime, effect + 24u, 0u);          // mono
    ctx.r[0] = effect;
}

void native_probe_sfx_unload(SH4Context& ctx, DCRuntime&) {
    // Matching probe-only no-op: fake effect structs live in reserved guest RAM.
    ctx.r[0] = 0u;
}

void native_probe_vga_cable(SH4Context& ctx, DCRuntime&) {
    // Probe-only default: CT_VGA. Avoids touching SH-4 port A at 0xFF80002C
    // when a homebrew only needs a deterministic host video mode.
    ctx.r[0] = 0u;
}

std::uint32_t native_heap_alloc(DCRuntime& runtime, std::uint32_t size, std::uint32_t alignment = 8u) {
    if (size == 0u) size = 1u;
    const std::uint32_t mask = alignment - 1u;
    const std::uint32_t address = (runtime.guest_heap_next + mask) & ~mask;
    if (address > runtime.guest_heap_end || size > runtime.guest_heap_end - address) return 0u;
    runtime.guest_heap_next = address + size;
    runtime.guest_alloc_sizes[address] = size;
    return address;
}

void native_malloc(SH4Context& ctx, DCRuntime& runtime) {
    ctx.r[0] = native_heap_alloc(runtime, ctx.r[4]);
}

void native_calloc(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint64_t total64 = static_cast<std::uint64_t>(ctx.r[4]) * ctx.r[5];
    if (total64 > 0xFFFFFFFFull) { ctx.r[0] = 0u; return; }
    const std::uint32_t total = static_cast<std::uint32_t>(total64);
    const std::uint32_t address = native_heap_alloc(runtime, total);
    if (address != 0u) dc_zero_bytes(runtime, address, total);
    ctx.r[0] = address;
}

void native_free(SH4Context& ctx, DCRuntime& runtime) {
    runtime.guest_alloc_sizes.erase(ctx.r[4]);
    ctx.r[0] = 0u;
}

void native_realloc(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint32_t old_address = ctx.r[4];
    const std::uint32_t new_size = ctx.r[5];
    if (old_address == 0u) { ctx.r[0] = native_heap_alloc(runtime, new_size); return; }
    if (new_size == 0u) { runtime.guest_alloc_sizes.erase(old_address); ctx.r[0] = 0u; return; }
    const auto old_it = runtime.guest_alloc_sizes.find(old_address);
    const std::uint32_t old_size = old_it == runtime.guest_alloc_sizes.end() ? 0u : old_it->second;
    const std::uint32_t new_address = native_heap_alloc(runtime, new_size);
    if (new_address == 0u) { ctx.r[0] = 0u; return; }
    const std::uint32_t copy_size = std::min(old_size, new_size);
    for (std::uint32_t i = 0; i < copy_size; ++i) dc_write8(runtime, new_address + i, dc_read8(runtime, old_address + i));
    runtime.guest_alloc_sizes.erase(old_address);
    ctx.r[0] = new_address;
}

bool native_valid_alignment(std::uint32_t alignment) {
    return alignment != 0u && (alignment & (alignment - 1u)) == 0u;
}

void native_memalign(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint32_t alignment = ctx.r[4];
    ctx.r[0] = native_valid_alignment(alignment) ? native_heap_alloc(runtime, ctx.r[5], alignment) : 0u;
}

void native_aligned_alloc(SH4Context& ctx, DCRuntime& runtime) {
    native_memalign(ctx, runtime);
}

void native_malloc_r(SH4Context& ctx, DCRuntime& runtime) {
    // _reent* in R4, size in R5.
    ctx.r[0] = native_heap_alloc(runtime, ctx.r[5]);
}

void native_calloc_r(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint64_t total64 = static_cast<std::uint64_t>(ctx.r[5]) * ctx.r[6];
    if (total64 > 0xFFFFFFFFull) { ctx.r[0] = 0u; return; }
    const std::uint32_t total = static_cast<std::uint32_t>(total64);
    const std::uint32_t address = native_heap_alloc(runtime, total);
    if (address != 0u) dc_zero_bytes(runtime, address, total);
    ctx.r[0] = address;
}

void native_free_r(SH4Context& ctx, DCRuntime& runtime) {
    runtime.guest_alloc_sizes.erase(ctx.r[5]);
    ctx.r[0] = 0u;
}

void native_realloc_r(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint32_t old_address = ctx.r[5];
    const std::uint32_t new_size = ctx.r[6];
    if (old_address == 0u) { ctx.r[0] = native_heap_alloc(runtime, new_size); return; }
    if (new_size == 0u) { runtime.guest_alloc_sizes.erase(old_address); ctx.r[0] = 0u; return; }
    const auto old_it = runtime.guest_alloc_sizes.find(old_address);
    const std::uint32_t old_size = old_it == runtime.guest_alloc_sizes.end() ? 0u : old_it->second;
    const std::uint32_t new_address = native_heap_alloc(runtime, new_size);
    if (new_address == 0u) { ctx.r[0] = 0u; return; }
    const std::uint32_t copy_size = std::min(old_size, new_size);
    for (std::uint32_t i = 0; i < copy_size; ++i) dc_write8(runtime, new_address + i, dc_read8(runtime, old_address + i));
    runtime.guest_alloc_sizes.erase(old_address);
    ctx.r[0] = new_address;
}

void native_memalign_r(SH4Context& ctx, DCRuntime& runtime) {
    // _reent* in R4, alignment in R5, size in R6.
    const std::uint32_t alignment = ctx.r[5];
    ctx.r[0] = native_valid_alignment(alignment) ? native_heap_alloc(runtime, ctx.r[6], alignment) : 0u;
}

void native_syscall_font_address(SH4Context& ctx, DCRuntime&) {
    // Use the same read-only synthetic font-ROM aperture as the retail BIOS
    // vector HLE. This preserves the real guest address and avoids allocating a
    // second 128 KiB shadow in main RAM.
    ctx.r[0] = 0xA0100020u;
}

void native_syscall_font_lock(SH4Context& ctx, DCRuntime&) {
    ctx.r[0] = 0u; // BIOS syscall returns 0 when the font lock is acquired.
}

void native_syscall_font_unlock(SH4Context& ctx, DCRuntime&) {
    ctx.r[0] = 0u;
}

void native_bfont_draw_str_fast(SH4Context& ctx, DCRuntime& runtime) {
    // This remains a legacy KOS-only bootstrap fast path. Retail software that
    // dereferences FONTROM_ADDRESS now sees the lazily synthesized 12x24/24x24
    // font ROM above. Keep this HLE disabled as a renderer until its framebuffer
    // semantics are implemented independently of the raw-ROM compatibility path.
    ++runtime.bfont_fast_calls;
    ctx.r[0] = 0u;
}

struct NativeRomFile {
    std::uint32_t data_offset{};
    std::uint32_t size{};
    std::uint32_t pos{};
};

static std::unordered_map<std::uint32_t, NativeRomFile> g_native_rom_files;
static std::uint32_t g_native_rom_next_fd = 0x70000000u;
static std::uint32_t g_native_rom_base = 0u;
static std::uint32_t g_native_rom_size = 0u;

std::uint32_t native_rom_be32(DCRuntime& runtime, std::uint32_t off) {
    if (off > g_native_rom_size || 4u > g_native_rom_size - off) return 0u;
    const std::uint32_t a = g_native_rom_base + off;
    return (static_cast<std::uint32_t>(dc_read8(runtime, a + 0u)) << 24u) |
           (static_cast<std::uint32_t>(dc_read8(runtime, a + 1u)) << 16u) |
           (static_cast<std::uint32_t>(dc_read8(runtime, a + 2u)) << 8u) |
            static_cast<std::uint32_t>(dc_read8(runtime, a + 3u));
}

std::string native_rom_name(DCRuntime& runtime, std::uint32_t off) {
    std::string out;
    if (off >= g_native_rom_size) return out;
    for (std::uint32_t i = off; i < g_native_rom_size && out.size() < 255u; ++i) {
        const char c = static_cast<char>(dc_read8(runtime, g_native_rom_base + i));
        if (c == '\0') break;
        out.push_back(c);
    }
    return out;
}

std::uint32_t native_rom_align16(std::uint32_t v) { return (v + 15u) & ~15u; }

bool native_rom_validate(DCRuntime& runtime) {
    if (g_native_rom_base == 0u || g_native_rom_size < 32u) return false;
    static constexpr char kMagic[8] = {'-','r','o','m','1','f','s','-'};
    for (unsigned i=0;i<8;++i) if (dc_read8(runtime, g_native_rom_base + i) != static_cast<std::uint8_t>(kMagic[i])) return false;
    const auto full = native_rom_be32(runtime, 8u);
    return full >= 32u && full <= g_native_rom_size;
}

std::uint32_t native_rom_root(DCRuntime& runtime) {
    const auto volume = native_rom_name(runtime, 16u);
    return native_rom_align16(16u + static_cast<std::uint32_t>(volume.size()) + 1u);
}

std::uint32_t native_rom_find_child(DCRuntime& runtime, std::uint32_t dir, const std::string& wanted) {
    if (dir + 16u > g_native_rom_size) return 0u;
    std::uint32_t cur = native_rom_be32(runtime, dir + 4u) & ~15u;
    std::uint32_t guard = 0u;
    while (cur != 0u && cur + 16u <= g_native_rom_size && guard++ < 65536u) {
        if (native_rom_name(runtime, cur + 16u) == wanted) return cur;
        cur = native_rom_be32(runtime, cur + 0u) & ~15u;
    }
    return 0u;
}

std::uint32_t native_rom_find(DCRuntime& runtime, std::string path) {
    if (!native_rom_validate(runtime)) return 0u;
    if (path.rfind("/rd/", 0) == 0) path.erase(0, 4);
    else if (path == "/rd") path.clear();
    while (!path.empty() && path.front() == '/') path.erase(path.begin());
    std::uint32_t cur = native_rom_root(runtime);
    if (path.empty()) return cur;
    std::size_t begin = 0u;
    while (begin < path.size()) {
        const auto slash = path.find('/', begin);
        const auto part = path.substr(begin, slash == std::string::npos ? std::string::npos : slash - begin);
        if (!part.empty()) {
            cur = native_rom_find_child(runtime, cur, part);
            if (cur == 0u) return 0u;
        }
        if (slash == std::string::npos) break;
        begin = slash + 1u;
    }
    return cur;
}

bool native_rom_open_file(DCRuntime& runtime, const std::string& path, NativeRomFile& out_file) {
    const std::uint32_t hdr = native_rom_find(runtime, path);
    if (hdr == 0u) return false;
    const std::uint32_t next = native_rom_be32(runtime, hdr);
    if ((next & 7u) != 2u) return false; // ROMFH_REG
    const std::string name = native_rom_name(runtime, hdr + 16u);
    const std::uint32_t data = native_rom_align16(hdr + 16u + static_cast<std::uint32_t>(name.size()) + 1u);
    const std::uint32_t size = native_rom_be32(runtime, hdr + 8u);
    if (data > g_native_rom_size || size > g_native_rom_size - data) return false;
    out_file = {data, size, 0u};
    return true;
}

void native_romfs_open(SH4Context& ctx, DCRuntime& runtime) {
    NativeRomFile file{};
    if (!native_rom_open_file(runtime, dc_read_c_string(runtime, ctx.r[4]), file)) { ctx.r[0] = 0u; return; }
    const std::uint32_t fd = g_native_rom_next_fd++;
    g_native_rom_files[fd] = file;
    ctx.r[0] = fd;
}

void native_romfs_read(SH4Context& ctx, DCRuntime& runtime) {
    const auto it = g_native_rom_files.find(ctx.r[4]);
    if (it == g_native_rom_files.end()) { ctx.r[0] = static_cast<std::uint32_t>(-1); return; }
    auto& f = it->second;
    const std::uint32_t wanted = ctx.r[6];
    const std::uint32_t remaining = f.pos <= f.size ? f.size - f.pos : 0u;
    const std::uint32_t count = std::min(wanted, remaining);
    for (std::uint32_t i=0;i<count;++i)
        dc_write8(runtime, ctx.r[5] + i, dc_read8(runtime, g_native_rom_base + f.data_offset + f.pos + i));
    f.pos += count;
    ctx.r[0] = count;
}

void native_romfs_close(SH4Context& ctx, DCRuntime&) {
    g_native_rom_files.erase(ctx.r[4]);
    ctx.r[0] = 0u;
}

void native_rom_stdio_fopen(SH4Context& ctx, DCRuntime& runtime) {
    const std::string path = dc_read_c_string(runtime, ctx.r[4]);
    const std::string mode = ctx.r[5] ? dc_read_c_string(runtime, ctx.r[5], 32) : std::string("r");
    // Embedded ROMFS is read-only. Reject write/append/update modes rather than
    // pretending data was persisted.
    if (mode.find('w') != std::string::npos || mode.find('a') != std::string::npos || mode.find('+') != std::string::npos) {
        ctx.r[0] = 0u;
        return;
    }
    NativeRomFile file{};
    if (!native_rom_open_file(runtime, path, file)) { ctx.r[0] = 0u; return; }
    const std::uint32_t handle = native_heap_alloc(runtime, 32u, 8u);
    if (handle == 0u) { ctx.r[0] = 0u; return; }
    g_native_rom_files[handle] = file;
    ctx.r[0] = handle;
}

void native_rom_stdio_fread(SH4Context& ctx, DCRuntime& runtime) {
    const std::uint32_t dst = ctx.r[4];
    const std::uint32_t elem_size = ctx.r[5];
    const std::uint32_t elem_count = ctx.r[6];
    const std::uint32_t handle = ctx.r[7];
    auto it = g_native_rom_files.find(handle);
    if (it == g_native_rom_files.end() || elem_size == 0u) { ctx.r[0] = 0u; return; }
    auto& f = it->second;
    const std::uint64_t wanted64 = static_cast<std::uint64_t>(elem_size) * elem_count;
    const std::uint32_t remaining = f.pos <= f.size ? f.size - f.pos : 0u;
    const std::uint32_t wanted = static_cast<std::uint32_t>(std::min<std::uint64_t>(wanted64, 0xFFFFFFFFull));
    const std::uint32_t bytes = std::min(wanted, remaining);
    for (std::uint32_t i = 0; i < bytes; ++i)
        dc_write8(runtime, dst + i, dc_read8(runtime, g_native_rom_base + f.data_offset + f.pos + i));
    f.pos += bytes;
    ctx.r[0] = bytes / elem_size;
}

void native_rom_stdio_fclose(SH4Context& ctx, DCRuntime& runtime) {
    g_native_rom_files.erase(ctx.r[4]);
    runtime.guest_alloc_sizes.erase(ctx.r[4]);
    ctx.r[0] = 0u;
}

void native_rom_stdio_fseek(SH4Context& ctx, DCRuntime&) {
    auto it = g_native_rom_files.find(ctx.r[4]);
    if (it == g_native_rom_files.end()) { ctx.r[0] = static_cast<std::uint32_t>(-1); return; }
    auto& f = it->second;
    const std::int64_t offset = static_cast<std::int32_t>(ctx.r[5]);
    std::int64_t base = 0;
    switch (ctx.r[6]) {
        case 0u: base = 0; break;            // SEEK_SET
        case 1u: base = f.pos; break;        // SEEK_CUR
        case 2u: base = f.size; break;       // SEEK_END
        default: ctx.r[0] = static_cast<std::uint32_t>(-1); return;
    }
    const std::int64_t next = base + offset;
    if (next < 0 || next > static_cast<std::int64_t>(f.size)) { ctx.r[0] = static_cast<std::uint32_t>(-1); return; }
    f.pos = static_cast<std::uint32_t>(next);
    ctx.r[0] = 0u;
}

void native_rom_stdio_ftell(SH4Context& ctx, DCRuntime&) {
    const auto it = g_native_rom_files.find(ctx.r[4]);
    ctx.r[0] = it == g_native_rom_files.end() ? static_cast<std::uint32_t>(-1) : it->second.pos;
}

void native_printf(SH4Context& ctx, DCRuntime& runtime) {
    const std::string format = dc_read_c_string(runtime, ctx.r[4]);
    std::ostringstream rendered;
    std::size_t arg_index = 0;
    std::uint8_t fp_arg_index = 4u;

    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%' || i + 1 >= format.size()) {
            rendered << format[i];
            continue;
        }

        ++i;
        if (format[i] == '%') {
            rendered << '%';
            continue;
        }

        char fill = ' ';
        bool left = false;
        while (i < format.size()) {
            if (format[i] == '0') { fill = '0'; ++i; continue; }
            if (format[i] == '-') { left = true; ++i; continue; }
            if (format[i] == '+' || format[i] == ' ' || format[i] == '#') { ++i; continue; }
            break;
        }

        int width = 0;
        while (i < format.size() && format[i] >= '0' && format[i] <= '9') {
            width = width * 10 + (format[i] - '0');
            ++i;
        }

        int precision = -1;
        if (i < format.size() && format[i] == '.') {
            ++i;
            precision = 0;
            while (i < format.size() && format[i] >= '0' && format[i] <= '9') {
                precision = precision * 10 + (format[i] - '0');
                ++i;
            }
        }

        int long_count = 0;
        while (i < format.size() && format[i] == 'l') { ++long_count; ++i; }
        if (i < format.size() && (format[i] == 'z' || format[i] == 't' || format[i] == 'h')) {
            // size_t/ptrdiff_t are 32-bit in this SH-4 ABI. Treat h similarly
            // for diagnostics; integer promotion already widened the vararg.
            ++i;
        }
        if (i >= format.size()) {
            rendered << "<incomplete-format>";
            break;
        }

        const char spec = format[i];
        std::ostringstream value;
        if (left) value << std::left;
        if (width > 0) value << std::setw(width) << std::setfill(fill);
        if (precision >= 0) value << std::fixed << std::setprecision(precision);

        switch (spec) {
            case 's': {
                const auto arg = printf_arg(ctx, runtime, arg_index++);
                value << dc_read_c_string(runtime, arg);
                break;
            }
            case 'c': {
                const auto arg = printf_arg(ctx, runtime, arg_index++);
                value << static_cast<char>(arg & 0xFFu);
                break;
            }
            case 'd':
            case 'i': {
                if (long_count >= 2) value << static_cast<std::int64_t>(printf_arg64(ctx, runtime, arg_index));
                else value << static_cast<std::int32_t>(printf_arg(ctx, runtime, arg_index++));
                break;
            }
            case 'u': {
                if (long_count >= 2) value << printf_arg64(ctx, runtime, arg_index);
                else value << printf_arg(ctx, runtime, arg_index++);
                break;
            }
            case 'x':
            case 'X': {
                if (spec == 'x') value << std::hex << std::nouppercase;
                else value << std::hex << std::uppercase;
                if (long_count >= 2) value << printf_arg64(ctx, runtime, arg_index);
                else value << printf_arg(ctx, runtime, arg_index++);
                break;
            }
            case 'p': {
                const auto arg = printf_arg(ctx, runtime, arg_index++);
                value << "0x" << std::hex << std::uppercase << arg;
                break;
            }
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'e':
            case 'E': {
                // SH-4 varargs keep floating arguments in the FP argument bank
                // independently of integer/pointer R5-R7. In particular KOS's
                // texture_render diagnostic passes its computed FPS in DR4.
                double d = 0.0;
                if (fp_arg_index <= 14u) {
                    d = dc_get_dr_double(ctx, fp_arg_index);
                    fp_arg_index = static_cast<std::uint8_t>(fp_arg_index + 2u);
                } else {
                    const std::uint64_t bits = printf_arg64(ctx, runtime, arg_index);
                    std::memcpy(&d, &bits, sizeof(d));
                }
                value << d;
                break;
            }
            default: {
                // Logging must never stop a recompiled program merely because the
                // bootstrap formatter does not model an exotic conversion yet.
                const auto arg = printf_arg(ctx, runtime, arg_index++);
                value << "<%" << spec << ":0x" << std::hex << arg << ">";
                break;
            }
        }
        rendered << value.str();
    }

    const std::string text = rendered.str();
    std::cout << text;
    std::cout.flush();
    ctx.r[0] = static_cast<std::uint32_t>(text.size());
}

} // namespace

void register_native_overrides(DCRuntime& runtime) {
    runtime.register_target(0x8C010040u, &native_printf); // _printf
}

void register_maple_host_overrides(DCRuntime& runtime, bool enabled) {
    if (!enabled) return;
}

void register_probe_overrides(DCRuntime& runtime, bool skip_audio, bool no_input, bool default_video, bool controller_a_then_start, bool controller_start_burst) {
    runtime.probe_controller_a_then_start_lowlevel = controller_a_then_start;
    (void)skip_audio;
    (void)no_input; (void)controller_a_then_start; (void)controller_start_burst;
    (void)default_video;
}

} // namespace dcrecomp_generated
