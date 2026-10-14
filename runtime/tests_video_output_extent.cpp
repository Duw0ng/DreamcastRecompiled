#include "dcr_video_output_extent.hpp"
#include <cassert>
int main() {
    using dcr_video::choose_swap_extent;
    auto first = choose_swap_extent(0, 0, 1920, 1080, 0, 0);
    assert(first.width == 1920 && first.height == 1080);
    first = choose_swap_extent(1, 1, 1280, 960, 0, 0);
    assert(first.width == 1280 && first.height == 960);
    auto stable = choose_swap_extent(0, 0, 1920, 1080, 1280, 720);
    assert(stable.width == 1280 && stable.height == 720);
    auto resized = choose_swap_extent(1440, 900, 1920, 1080, 1920, 1080);
    assert(resized.width == 1440 && resized.height == 900);
    auto native = choose_swap_extent(0, 0, 0, 0, 0, 0);
    assert(native.width == 640 && native.height == 480);
    auto max = choose_swap_extent(16000, 9000, 1920, 1080, 0, 0);
    assert(max.width == 7680 && max.height == 4320);
    return 0;
}
