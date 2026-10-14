#pragma once
// Host-only swapchain sizing; independent of the Dreamcast SH-4/PVR ABI.
// When the HWND is not ready, fall back to the requested size instead of
// permanently creating a 1x1 DXGI swap chain.
#include <algorithm>
#include <cstdint>

namespace dcr_video {
struct Extent { std::uint32_t width{}, height{}; };

constexpr Extent choose_swap_extent(int client_w, int client_h,
                                    int requested_w, int requested_h,
                                    std::uint32_t old_w, std::uint32_t old_h) {
    if (client_w >= 64 && client_h >= 64)
        return {static_cast<std::uint32_t>(std::clamp(client_w, 64, 7680)),
                static_cast<std::uint32_t>(std::clamp(client_h, 64, 4320))};
    if (old_w >= 64 && old_h >= 64) return {old_w, old_h};
    return {static_cast<std::uint32_t>(requested_w >= 64 ? requested_w : 640),
            static_cast<std::uint32_t>(requested_h >= 64 ? requested_h : 480)};
}
} // namespace dcr_video
