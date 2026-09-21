#include "generated_program.hpp"
#include "dc_image.hpp"
#include "dc_native_overrides.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

class DCRSessionTeeBuf final : public std::streambuf {
public:
    DCRSessionTeeBuf(std::streambuf* console, std::streambuf* file) : console_(console), file_(file) {}
protected:
    int overflow(int ch) override {
        if (ch == traits_type::eof()) return traits_type::not_eof(ch);
        const char c = static_cast<char>(ch);
        if (console_ && console_->sputc(c) == traits_type::eof()) return traits_type::eof();
        if (file_ && file_->sputc(c) == traits_type::eof()) return traits_type::eof();
        return ch;
    }
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        const auto a = console_ ? console_->sputn(s, n) : n;
        const auto b = file_ ? file_->sputn(s, n) : n;
        return (a < b) ? a : b;
    }
    int sync() override {
        int rc = 0;
        if (console_ && console_->pubsync() != 0) rc = -1;
        if (file_ && file_->pubsync() != 0) rc = -1;
        return rc;
    }
private:
    std::streambuf* console_{};
    std::streambuf* file_{};
};

static std::string dcr_make_session_log_path() {
    std::error_code ec;
    std::filesystem::create_directories("logs", ec);
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream out;
    if (!ec) out << "logs/";
    out << "DreamcastRecomp_v0.1.1_session_" << std::put_time(&tm, "%Y%m%d-%H%M%S") << ".log";
    return out.str();
}

class DCRSessionLogGuard final {
public:
    DCRSessionLogGuard() : path_(dcr_make_session_log_path()), file_(path_, std::ios::out | std::ios::trunc), old_cout_(std::cout.rdbuf()), old_cerr_(std::cerr.rdbuf()), tee_(old_cout_, file_ ? file_.rdbuf() : nullptr) {
        if (file_) { std::cout.rdbuf(&tee_); std::cerr.rdbuf(&tee_); }
    }
    ~DCRSessionLogGuard() {
        std::cout.flush(); std::cerr.flush();
        std::cout.rdbuf(old_cout_); std::cerr.rdbuf(old_cerr_);
        if (file_) file_.flush();
    }
    bool enabled() const { return static_cast<bool>(file_); }
    const std::string& path() const { return path_; }
private:
    std::string path_;
    std::ofstream file_;
    std::streambuf* old_cout_{};
    std::streambuf* old_cerr_{};
    DCRSessionTeeBuf tee_;
};

#ifdef _WIN32
static dcrecomp_generated::DCRuntime* g_dcr_crash_runtime = nullptr;
static LONG WINAPI dcr_unhandled_exception_filter(EXCEPTION_POINTERS* ep) {
    const unsigned long code = (ep && ep->ExceptionRecord) ? ep->ExceptionRecord->ExceptionCode : 0ul;
    const void* address = (ep && ep->ExceptionRecord) ? ep->ExceptionRecord->ExceptionAddress : nullptr;
    std::cerr << "\n[DreamcastRecomp HOST FATAL] Windows exception=0x" << std::hex << std::uppercase << code
              << " | host-address=" << address;
    if (g_dcr_crash_runtime) {
        std::cerr << " | guest-pc=0x" << std::setw(8) << std::setfill('0') << g_dcr_crash_runtime->current_pc
                  << " | aica-pc=0x" << std::setw(8) << g_dcr_crash_runtime->aica_arm.r[15]
                  << " | aica-frames=" << std::dec << g_dcr_crash_runtime->aica_native_frames;
    }
    std::cerr << "\n";
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char** argv) {
    using namespace dcrecomp_generated;
    DCRSessionLogGuard dcr_session_log;
    if (dcr_session_log.enabled()) std::cout << "[DCR LOG] session-log=" << dcr_session_log.path() << "\n";
    DCRuntime runtime;
#ifdef _WIN32
    g_dcr_crash_runtime = &runtime;
    SetUnhandledExceptionFilter(dcr_unhandled_exception_filter);
#endif
    try {
        SH4Context ctx{};
        std::vector<std::uint32_t> peek8, peek16, peek32;
        bool probe_skip_audio = false;
        bool probe_no_input = false;
        bool probe_default_video = false;
        bool probe_controller_a_then_start = false;
        bool probe_controller_start_burst = false;
        bool maple_host_input = false;
        std::string controller_profile;
        bool commercial_boot = false;
        bool direct_game_entry = false;
        std::uint32_t direct_game_pc = 0x8C010000u;
        std::uint64_t commercial_continuation_limit = 0u;
        std::uint64_t commercial_stages = 0u;
        std::string disc_map;
        bool host_window = false;
        std::uint64_t diag_heartbeat_ms = 0u;
        bool pvr_window = false;
        bool pvr_frame_sync = false;
        bool pvr_profile = false;
        bool perf_profile = false;
        std::uint32_t perf_sample_stride = 67u;
        bool pvr_mt = false;
        bool pvr_gpu = false;
        bool pvr_gpu_force_readback = false;
        bool pvr_render_done_scheduled = false;
        std::uint32_t sh4_tick_batch = 1u;
        bool fast_dispatch = false;
        bool direct_dispatch = false;
        bool aica_kos_hle = false;
        bool aica_arm7 = false;
        bool probe_kos_aica_defaults = false;
        bool aica_play = false;
        std::string aica_wav;
        std::string cdda_wav;
        std::uint32_t pvr_window_scale = 1u;
        std::uint64_t pvr_window_interval = 256u;
        std::uint32_t pvr_window_throttle_ms = 0u;
        std::uint32_t pvr_window_fps = 60u;
        std::uint64_t aica_arm7_slice = 128u;
        std::uint64_t aica_arm7_boot = 32768u;
        std::uint64_t aica_timer_div = 1024u;
        std::uint64_t aica_timer_rate_num = 1u;
        std::uint64_t aica_timer_rate_den = 1u;
        std::uint64_t aica_sh4_div = 4u;
        bool aica_pvr_sync = false;
        bool device_clock = false;
        bool device_clock_host = false;
        std::uint64_t device_clock_host_max_catchup = 4096u;
        std::uint64_t aica_capture_ms = 0u;

        load_embedded_elf_image(runtime);
        register_recompiled_program(runtime);
        register_native_overrides(runtime);

        constexpr std::uint32_t kStackTop = 0x8CFFF000u;
        constexpr std::uint32_t kHostReturnSentinel = 0xFFFFFFFFu;
        ctx.r[15] = kStackTop;
        ctx.pr = kHostReturnSentinel;
        ctx.pc = 0x8C010000u;
        for (int ai = 1; ai < argc; ++ai) {
            const std::string pre = argv[ai];
            if (pre == "--commercial-boot") { commercial_boot = true; runtime.commercial_boot_active = true; }
            else if (pre == "--ct2-compat") runtime.ct2_compat = true;
        }
        if (commercial_boot) { dc_setup_commercial_boot(ctx, runtime); ctx.pc = 0x8C010000u; }
        for (int i = 1; i < argc; ++i) {
            std::string key = argv[i];
            const auto eq = key.find('=');
            auto read_value = [&]() -> std::string { if (eq != std::string::npos) return key.substr(eq + 1); if (++i >= argc) throw std::runtime_error("Missing option value"); return argv[i]; };
            if (key == "--probe-skip-audio") {
                probe_skip_audio = true;
            } else if (key == "--probe-no-input") {
                probe_no_input = true;
            } else if (key == "--probe-video-default") {
                probe_default_video = true;
            } else if (key == "--probe-controller-a-then-start") {
                probe_controller_a_then_start = true;
            } else if (key == "--probe-controller-start-burst") {
                probe_controller_start_burst = true;
            } else if (key == "--maple-host-input") {
                maple_host_input = true;
            } else if (key.rfind("--controller-profile=", 0) == 0) {
                controller_profile = key.substr(21);
                maple_host_input = true;
            } else if (key == "--trace-calls") {
                runtime.trace_calls = true;
            } else if (key == "--sh4-abi-audit") {
                runtime.sh4_abi_audit = true;
            } else if (key == "--commercial-boot") {
                commercial_boot = true; runtime.commercial_boot_active = true; // setup already applied in pre-scan
            } else if (key == "--ct2-compat") {
                runtime.ct2_compat = true;
            } else if (key.rfind("--direct-game-entry", 0) == 0) {
                direct_game_entry = true; direct_game_pc = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--commercial-continuation-limit", 0) == 0) {
                commercial_continuation_limit = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--disc-map", 0) == 0) {
                disc_map = read_value();
            } else if (key == "--host-window") {
                host_window = true;
            } else if (key.rfind("--diag-heartbeat-ms", 0) == 0) {
                diag_heartbeat_ms = std::stoull(read_value(), nullptr, 0);
            } else if (key == "--pvr-window") {
                pvr_window = true;
            } else if (key == "--pvr-frame-sync") {
                pvr_frame_sync = true;
            } else if (key == "--pvr-profile") {
                pvr_profile = true;
            } else if (key == "--perf-profile") {
                perf_profile = true;
            } else if (key.rfind("--perf-sample-stride", 0) == 0) {
                perf_sample_stride = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key == "--pvr-mt") {
                pvr_mt = true;
            } else if (key == "--pvr-gpu") {
                pvr_gpu = true;
            } else if (key == "--pvr-gpu-readback") {
                pvr_gpu = true; pvr_gpu_force_readback = true;
            } else if (key == "--pvr-render-done-scheduled") {
                pvr_render_done_scheduled = true;
            } else if (key == "--fast-dispatch") {
                fast_dispatch = true;
            } else if (key == "--direct-dispatch") {
                direct_dispatch = true;
            } else if (key.rfind("--sh4-tick-batch", 0) == 0) {
                sh4_tick_batch = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key == "--aica-kos-hle") {
                aica_kos_hle = true;
            } else if (key == "--probe-kos-aica-defaults") {
                probe_kos_aica_defaults = true;
            } else if (key == "--aica-arm7") {
                aica_arm7 = true;
            } else if (key.rfind("--aica-arm7-slice", 0) == 0) {
                aica_arm7_slice = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--aica-arm7-boot", 0) == 0) {
                aica_arm7_boot = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--aica-timer-div", 0) == 0) {
                aica_timer_div = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--aica-timer-rate", 0) == 0) {
                const std::string ratio = read_value();
                const auto slash = ratio.find('/');
                if (slash == std::string::npos) { aica_timer_rate_num = std::stoull(ratio, nullptr, 0); aica_timer_rate_den = 1u; }
                else { aica_timer_rate_num = std::stoull(ratio.substr(0, slash), nullptr, 0); aica_timer_rate_den = std::stoull(ratio.substr(slash + 1), nullptr, 0); }
                if (aica_timer_rate_num == 0u || aica_timer_rate_den == 0u) throw std::runtime_error("--aica-timer-rate requires non-zero N/D");
            } else if (key.rfind("--aica-sh4-div", 0) == 0) {
                aica_sh4_div = std::stoull(read_value(), nullptr, 0);
            } else if (key == "--aica-pvr-sync") {
                aica_pvr_sync = true;
            } else if (key == "--device-clock") {
                device_clock = true;
            } else if (key == "--device-clock-host") {
                device_clock = true; device_clock_host = true;
            } else if (key.rfind("--device-clock-host-max-catchup", 0) == 0) {
                device_clock_host_max_catchup = std::stoull(read_value(), nullptr, 0);
                if (device_clock_host_max_catchup == 0u) throw std::runtime_error("--device-clock-host-max-catchup must be > 0");
            } else if (key.rfind("--aica-capture-ms", 0) == 0) {
                aica_capture_ms = std::stoull(read_value(), nullptr, 0);
            } else if (key == "--aica-play") {
                aica_play = true;
            } else if (key.rfind("--aica-wav", 0) == 0) {
                aica_wav = read_value();
            } else if (key.rfind("--cdda-wav", 0) == 0) {
                cdda_wav = read_value();
            } else if (key.rfind("--aica-stop-after-starts", 0) == 0) {
                runtime.aica_start_limit = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--pvr-window-scale", 0) == 0) {
                pvr_window_scale = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--pvr-window-interval", 0) == 0) {
                pvr_window_interval = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--pvr-window-throttle-ms", 0) == 0) {
                pvr_window_throttle_ms = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--pvr-window-fps", 0) == 0) {
                pvr_window_fps = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--pvr-dump", 0) == 0) {
                dc_pvr_set_dump_path(runtime, read_value());
            } else if (key.rfind("--pvr-stop-after-packets", 0) == 0) {
                runtime.pvr_packet_limit = std::stoull(read_value(), nullptr, 0);
            } else if (key.rfind("--memstr", 0) == 0) {
                const std::string spec = read_value();
                const auto colon = spec.find(':');
                if (colon == std::string::npos) throw std::runtime_error("--memstr expects ADDRESS:TEXT");
                const std::uint32_t address = static_cast<std::uint32_t>(std::stoul(spec.substr(0, colon), nullptr, 0));
                const std::string text = spec.substr(colon + 1);
                dc_load_bytes(runtime, address, reinterpret_cast<const std::uint8_t*>(text.c_str()), text.size() + 1u);
            } else if (key.rfind("--membin", 0) == 0) {
                const std::string spec = read_value();
                const auto colon = spec.find(':');
                if (colon == std::string::npos) throw std::runtime_error("--membin expects ADDRESS:PATH");
                const std::uint32_t address = static_cast<std::uint32_t>(std::stoul(spec.substr(0, colon), nullptr, 0));
                const std::string path = spec.substr(colon + 1);
                std::ifstream file(path, std::ios::binary);
                if (!file) throw std::runtime_error("Unable to open --membin file: " + path);
                file.seekg(0, std::ios::end);
                const auto end = file.tellg();
                if (end < 0) throw std::runtime_error("Unable to determine --membin file size");
                std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
                file.seekg(0, std::ios::beg);
                if (!bytes.empty()) file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                if (!file && !bytes.empty()) throw std::runtime_error("Unable to read complete --membin file");
                if (!bytes.empty()) dc_load_bytes(runtime, address, bytes.data(), bytes.size());
            } else if (key.rfind("--mem8", 0) == 0) {
                const std::string spec = read_value();
                const auto colon = spec.find(':');
                if (colon == std::string::npos) throw std::runtime_error("--mem8 expects ADDRESS:VALUE");
                dc_write8(runtime, static_cast<std::uint32_t>(std::stoul(spec.substr(0, colon), nullptr, 0)), static_cast<std::uint8_t>(std::stoul(spec.substr(colon + 1), nullptr, 0)));
            } else if (key.rfind("--mem16", 0) == 0) {
                const std::string spec = read_value();
                const auto colon = spec.find(':');
                if (colon == std::string::npos) throw std::runtime_error("--mem16 expects ADDRESS:VALUE");
                dc_write16(runtime, static_cast<std::uint32_t>(std::stoul(spec.substr(0, colon), nullptr, 0)), static_cast<std::uint16_t>(std::stoul(spec.substr(colon + 1), nullptr, 0)));
            } else if (key.rfind("--mem32", 0) == 0) {
                const std::string spec = read_value();
                const auto colon = spec.find(':');
                if (colon == std::string::npos) throw std::runtime_error("--mem32 expects ADDRESS:VALUE");
                const std::uint32_t address = static_cast<std::uint32_t>(std::stoul(spec.substr(0, colon), nullptr, 0));
                const std::uint32_t value = static_cast<std::uint32_t>(std::stoul(spec.substr(colon + 1), nullptr, 0));
                dc_write32(runtime, address, value);
            } else if (key.rfind("--peek8", 0) == 0) {
                peek8.push_back(static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key.rfind("--peek16", 0) == 0) {
                peek16.push_back(static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key.rfind("--peek32", 0) == 0) {
                peek32.push_back(static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key.rfind("--sr", 0) == 0) {
                dc_write_sr(ctx, static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else if (key.rfind("--gbr", 0) == 0) { ctx.gbr = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--vbr", 0) == 0) { ctx.vbr = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--ssr", 0) == 0) { ctx.ssr = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--spc", 0) == 0) { ctx.spc = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--sgr", 0) == 0) { ctx.sgr = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--dbr", 0) == 0) { ctx.dbr = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--fr", 0) == 0) {
                const std::string reg_text = key.substr(4, eq == std::string::npos ? std::string::npos : eq - 4);
                const int reg = std::stoi(reg_text);
                if (reg < 0 || reg > 15) throw std::runtime_error("Runner FPU register must be FR0-FR15");
                dc_set_fr_float(ctx, static_cast<std::uint8_t>(reg), std::stof(read_value()));
            } else if (key.rfind("--r", 0) == 0) {
                const std::string reg_text = key.substr(3, eq == std::string::npos ? std::string::npos : eq - 3);
                const int reg = std::stoi(reg_text);
                if (reg < 0 || reg > 15) throw std::runtime_error("Runner register must be R0-R15");
                ctx.r[reg] = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--fpul", 0) == 0) {
                ctx.fpul = static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0));
            } else if (key.rfind("--fpscr", 0) == 0) {
                dc_write_fpscr(ctx, static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)));
            } else { throw std::runtime_error("Unknown runner option: " + key); }
        }
        if (!disc_map.empty()) dc_gdrom_load_disc_map(runtime, disc_map);
        if (commercial_boot && disc_map.empty()) std::cerr << "[GDROM] warning: no --disc-map supplied; disc reads will stop at first data request\n";
        if (!controller_profile.empty() && !dc_maple_load_controller_profile(runtime, controller_profile))
            throw std::runtime_error("No se pudo cargar controller profile: " + controller_profile);
        if (maple_host_input) { dc_maple_enable_host_input(runtime, true); register_maple_host_overrides(runtime, true); }
        runtime.probe_controller_start_burst_lowlevel = probe_controller_start_burst;
        register_probe_overrides(runtime, probe_skip_audio, probe_no_input, probe_default_video, probe_controller_a_then_start, probe_controller_start_burst);
        if (aica_kos_hle) dc_aica_enable_kos_hle(runtime, true);
        if (aica_arm7) {
            dc_aica_set_arm7_boot_slice(runtime, aica_arm7_boot);
            dc_aica_set_timer_div(runtime, aica_timer_div);
            dc_aica_set_timer_rate(runtime, aica_timer_rate_num, aica_timer_rate_den);
            dc_aica_set_sh4_div(runtime, aica_sh4_div);
            dc_aica_set_pvr_sync(runtime, aica_pvr_sync);
            dc_aica_enable_arm7(runtime, true, aica_arm7_slice);
            if (probe_kos_aica_defaults) dc_aica_seed_kos_defaults(runtime);
            if (aica_capture_ms != 0u) dc_aica_set_capture_ms(runtime, aica_capture_ms);
        }
        if (!aica_wav.empty()) dc_aica_set_wav_path(runtime, aica_wav);
        if (!cdda_wav.empty()) dc_gdrom_set_cdda_wav_path(runtime, cdda_wav);
        if (aica_play) dc_aica_set_host_playback(runtime, true);
        if (pvr_frame_sync) dc_pvr_set_frame_sync(runtime, true);
        if (pvr_profile) dc_pvr_set_profile(runtime, true);
        if (perf_profile) dc_perf_set_profile(runtime, true, perf_sample_stride);
        runtime.pvr_gpu_enabled = pvr_gpu;
        runtime.pvr_gpu_force_readback = pvr_gpu_force_readback;
        runtime.pvr_render_done_scheduled_mode = pvr_render_done_scheduled;
        runtime.pvr_mt_enabled = pvr_mt;
        dc_pvr_refresh_fastpaths(runtime);
        runtime.sh4_tick_batch_cycles = sh4_tick_batch;
        runtime.fast_dispatch_enabled = fast_dispatch;
        runtime.direct_dispatch_enabled = direct_dispatch;
        dc_refresh_dispatch_ready(runtime);
        if (pvr_window) dc_pvr_enable_window(runtime, "DreamcastRecomp v0.1.1 Official - PVR live", pvr_window_scale, pvr_window_interval, pvr_window_throttle_ms, pvr_window_fps);
        runtime.device_clock_host_max_catchup_steps = device_clock_host_max_catchup;
        if (device_clock) dc_device_clock_enable(runtime, true, device_clock_host);
        if (diag_heartbeat_ms != 0u) dc_diag_heartbeat_enable(runtime, diag_heartbeat_ms);
        if (host_window) dc_host_status_enable(runtime, "DreamcastRecomp v0.1.1 Official - runner activo");

        std::uint32_t dcr_initial_target = 0x8C010000u;
        if (direct_game_entry) {
            if (!commercial_boot) throw std::runtime_error("--direct-game-entry requires --commercial-boot");
            for (auto& reg : ctx.r) reg = 0u;
            ctx.r[15] = 0x8C00F400u;
            ctx.pr = 0x8C00E0B2u;
            ctx.gbr = 0x8C000000u;
            ctx.fpul = 0u;
            dc_write_sr(ctx, 0x700000F0u);
            dc_write_fpscr(ctx, 0x00040000u);
            ctx.pc = direct_game_pc;
            dcr_initial_target = direct_game_pc;
            std::cerr << "[DCR DIRECT-GAME] bypass bootstrap; entry=0x" << std::hex << std::uppercase << direct_game_pc << std::dec << "\n";
        }
        std::cout << "DreamcastRecomp v0.1.1 Official native runner\n"
                     "===================================\n"
                     "Reachable functions: 1\n"
                     "Executing _main @ 0x8C010000u\n\n";

        runtime.sh4_async_redirect = false;
        call_recompiled(ctx, runtime, dcr_initial_target);
        if (commercial_boot) {
            while (ctx.pc != kHostReturnSentinel && (commercial_continuation_limit == 0u || commercial_stages < commercial_continuation_limit)) {
                const std::uint32_t next_pc = ctx.pc;
                if (runtime.trace_calls) std::cerr << "[BOOT] continue -> 0x" << std::hex << std::uppercase << next_pc << std::dec << "\n";
                runtime.sh4_async_redirect = false;
                call_recompiled(ctx, runtime, next_pc);
                ++commercial_stages;
            }
        }

        std::cout << "\n\n[DreamcastRecomp] returned to host"
                     << " | R0=" << ctx.r[0]
                     << " | FR0=" << dc_get_fr_float(ctx, 0u)
                     << " | SR=0x" << std::hex << std::uppercase << ctx.sr
                     << " | GBR=0x" << ctx.gbr
                     << " | PC=0x" << ctx.pc << std::dec << "\n";
        for (const auto address : peek8) std::cout << "[PEEK8] 0x" << std::hex << std::uppercase << address << " = 0x" << static_cast<unsigned>(dc_read8(runtime, address)) << std::dec << "\n";
        for (const auto address : peek16) std::cout << "[PEEK16] 0x" << std::hex << std::uppercase << address << " = 0x" << dc_read16(runtime, address) << std::dec << "\n";
        for (const auto address : peek32) std::cout << "[PEEK32] 0x" << std::hex << std::uppercase << address << " = 0x" << dc_read32(runtime, address) << std::dec << "\n";
        if (runtime.aica_arm7_enabled) dc_aica_flush_wav(runtime);
        if (runtime.perf_profile_enabled) dc_perf_print_stats(runtime);
        if (runtime.ta_packets || runtime.mmio_reads || runtime.mmio_writes) dc_pvr_print_stats(runtime);
        if (runtime.aica_kos_hle || runtime.aica_arm7_enabled) dc_aica_print_stats(runtime);
        if (runtime.maple_host_input || runtime.maple_dma_frames) dc_maple_print_stats(runtime);
        dc_gdrom_print_stats(runtime);
        if (!runtime.pvr_dump_path.empty()) dc_pvr_dump_ppm(runtime, runtime.pvr_dump_path);
        if (runtime.host_status_window_enabled) dc_host_status_close(runtime);
        if (!commercial_boot && ctx.pc != kHostReturnSentinel) {
            std::cerr << "[DreamcastRecomp ERROR] root function did not return to host sentinel\n";
            return 3;
        }
        if (commercial_boot && ctx.pc != kHostReturnSentinel && commercial_continuation_limit != 0u && commercial_stages >= commercial_continuation_limit) {
            dc_trace_print_history(runtime, std::cerr);
            std::cerr << "[DreamcastRecomp LIMIT] commercial execution reached continuation limit=" << commercial_continuation_limit << " at PC=0x" << std::hex << std::uppercase << ctx.pc << std::dec << "\n";
            return 4;
        }
        return 0;
    } catch (const std::exception& e) {
        const std::string dcr_error = e.what();
        if (runtime.host_status_window_enabled) dc_host_status_close(runtime);
        if (dcr_error == "Host status window closed by user") { if (runtime.aica_arm7_enabled) dc_aica_flush_wav(runtime); if (runtime.perf_profile_enabled) dc_perf_print_stats(runtime); if (runtime.maple_host_input || runtime.maple_dma_frames) dc_maple_print_stats(runtime); if (runtime.commercial_boot_active) { std::cout << "[DreamcastRecomp] Host status window closed by user during commercial execution.\n"; return 130; } std::cout << "[DreamcastRecomp] Host status window closed.\n"; return 0; }
        if (dcr_error == "PVR live window closed by user") { if (runtime.aica_arm7_enabled) dc_aica_flush_wav(runtime); if (runtime.perf_profile_enabled) dc_perf_print_stats(runtime); if (runtime.maple_host_input || runtime.maple_dma_frames) dc_maple_print_stats(runtime); std::cout << "[DreamcastRecomp] PVR live window closed.\n"; return 0; }
        if (std::string(e.what()) == "AICA native capture complete") {
            dc_aica_flush_wav(runtime);
            dc_aica_print_stats(runtime);
            if (runtime.perf_profile_enabled) dc_perf_print_stats(runtime);
            if (runtime.maple_host_input || runtime.maple_dma_frames) dc_maple_print_stats(runtime);
            std::cout << "[DreamcastRecomp] AICA native capture complete.\n";
            return 0;
        }
        if (runtime.aica_arm7_enabled) { try { dc_aica_flush_wav(runtime); } catch (...) {} dc_aica_print_stats(runtime); }
        if (runtime.perf_profile_enabled) dc_perf_print_stats(runtime);
        dc_gdrom_print_stats(runtime);
        dc_trace_print_history(runtime, std::cerr);
        std::cerr << "[DreamcastRecomp ERROR] " << e.what() << "\n";
        return 2;
    }
}
