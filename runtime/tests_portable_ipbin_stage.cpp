// Regression: portable combined BOOTSTRAP.BIN must preserve IP.BIN while
// scrambling ONLY the executable at 0x8C010000 (regression from cpp38e).
#include "dc_runtime.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace dcrecomp_generated;
namespace {
void check(bool pass, const char* what) {
    if (!pass) throw std::runtime_error(what);
}
void run_case(bool combined, bool scramble, bool raw_track = false) {
    constexpr std::size_t kIpbin = 0x8000u;
    constexpr std::size_t kExe = 16384u;
    const auto base = combined ? 0x8C008000u : 0x8C010000u;
    std::vector<std::uint8_t> boot((combined ? kIpbin : 0u) + kExe);
    for (std::size_t i = 0; i < boot.size(); ++i)
        boot[i] = static_cast<std::uint8_t>((i * 17u + (i >> 5u) * 7u) & 255u);
    const auto path = std::filesystem::temp_directory_path() /
        (combined ? "dcr_combined_ipbin_regression.bin" : "dcr_exe_only_regression.bin");
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(boot.data()),
                   static_cast<std::streamsize>(boot.size()));
    }
#ifdef _WIN32
    _putenv_s("DCR_EXTERNAL_RAW_IMAGE", path.string().c_str());
    _putenv_s("DCR_EXTERNAL_RAW_BASE", combined ? "0x8C008000" : "0x8C010000");
#else
    setenv("DCR_EXTERNAL_RAW_IMAGE", path.string().c_str(), 1);
    setenv("DCR_EXTERNAL_RAW_BASE", combined ? "0x8C008000" : "0x8C010000", 1);
#endif
    DCRuntime runtime;
    runtime.commercial_boot_active = true;
    runtime.commercial_boot_stage_mode = scramble ? 2u : 1u;
    if (raw_track) {
        GDDiscTrack track{};
        track.sector_size = 2336u;
        track.user_offset = 8u;
        runtime.gd_tracks.push_back(track);
        runtime.gd_boot_track = 0u;
        runtime.gd_disc_ready = true;
    }
    SH4Context ctx{};
    dc_setup_commercial_boot(ctx, runtime);
    const std::size_t stage_off = (base & 0x1FFFFFFFu) - DCRuntime::kMainRamPhysicalBase;
    const std::size_t exe_off = stage_off + (combined ? kIpbin : 0u);
    check(runtime.commercial_boot_image_address == 0x8C010000u, "wrong executable staging address");
    check(runtime.commercial_boot_image_size == kExe, "wrong executable staging size");
    if (combined) {
        check(std::equal(boot.begin(), boot.begin() + kIpbin, runtime.main_ram.begin() + stage_off),
              "IP.BIN changed: retail bootstrap must never be scrambled");
    }
    const auto exe_same = std::equal(boot.begin() + (combined ? kIpbin : 0u),
                                     boot.end(), runtime.main_ram.begin() + exe_off);
    const bool expect_scrambled = scramble || (combined && raw_track);
    check(expect_scrambled ? !exe_same : exe_same,
          "executable scrambling/plain portable correction mismatch");
    std::filesystem::remove(path);
    std::cout << "[PASS] combined=" << combined << " scrambled=" << scramble
              << " IP.BIN=" << (combined ? "preserved" : "not present")
              << " raw-track=" << raw_track
              << " exe-size=" << runtime.commercial_boot_image_size << "\n";
}
}
int main() {
    try {
        run_case(true, true);
        run_case(true, false);
        run_case(false, true);
        run_case(false, false);
        // Original Go portable forces plain; a raw CDI track with combined
        // IP.BIN must instead apply scrambled to executable bytes only.
        run_case(true, false, true);
        // The override must NOT trigger on an executable-only image.
        run_case(false, false, true);
        std::cout << "[PASS] portable retail IP.BIN split staging: 6/6\n";
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return 1;
    }
    return 0;
}
