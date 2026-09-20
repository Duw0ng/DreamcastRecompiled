# ChuChu Rocket! checkpoint - 0.0.62

## Accepted retail state

The Windows 0.0.61 acceptance run crosses the persistent A1 VMU save path and reaches the real ChuChu Rocket! main menu plus Mode Select. This is the first checkpoint where menu-by-menu retail compatibility testing is practical. The user also reached the next selected Mode Select screen. Selecting **Options** with A exposes another compatibility crash, but its exact target/log is intentionally deferred until the faster build can be exercised broadly.

The working storage path is preserved unchanged. Earlier acceptance already showed `vmu=A1/1/1/28/20/5`, proving real DEVINFO/media-info traffic, 28 reads, 20 phased writes and five syncs before entering the main menu. Keep `dreamcast_vmu_a1.bin` between runs.

## 0.0.62 performance focus

The dominant remaining test-time cost is the software PVR. ChuChu's menu workload is overwhelmingly bilinear texture sampling; the old sampler repeatedly decoded the same VRAM texels for every output pixel. 0.0.62 adds a decoded ARGB texture cache with 4 KiB VRAM-page generation tracking, preserving the existing 640x480 software renderer and bilinear output.

Validation highlights:

- controlled framebuffer hash: identical to 0.0.61 (`e8de85af5ec47e46`);
- synthetic 640x480 RGB565 bilinear quad: ~30–32 ms/frame -> ~9.7 ms/frame;
- supplied-CDI 1,000-TA-packet accumulated raster: ~2,987.9 ms -> ~1,301.9 ms;
- final cache reuse in that CDI slice: 298 hits / 6 misses / 6 decodes;
- closure unchanged at 2,349 functions / 174,530 known SH-4 / 0 unknown / `RAW_SH4=0`.

Heartbeats expose `pvr-tcache=hits/misses/decodes` so the Windows Release run can show whether the same reuse occurs through the full boot/menu path.

## Next acceptance

Use the existing VMU image, measure time/FPS from boot to Mode Select, then traverse every menu entry and retain the exact `[DreamcastRecomp ERROR]` plus last-32-call trace for each distinct crash. The next compatibility release should recover those state families structurally rather than seed individual ChuChu addresses.
