# DreamcastRecomp 0.0.62 validation

## Commercial closure

- Supplied ChuChu Rocket CDI: 2,349 reachable functions.
- 174,530 reachable SH-4 instructions are known.
- Unknown SH-4: 0.
- `RAW_SH4=0`.
- Closure/code-discovery behavior is unchanged from accepted 0.0.61.

## Regression

- Full CTest target: 47/47 PASS.
- VMU/Maple save path remains unchanged from the accepted build that reached the real main menu.

## PVR correctness and performance

- Output remains 640x480 software rasterization with the existing bilinear path.
- Decoded texture cache uses per-4-KiB VRAM page generations and a separate palette epoch.
- Controlled 12-frame framebuffer hash: 0.0.61 = `e8de85af5ec47e46`; 0.0.62 cache-only = `e8de85af5ec47e46`.
- A faster span/scanline experiment that changed the framebuffer was rejected and is not in the release.
- Synthetic full-screen RGB565 twiddled/bilinear quad: ~30–32 ms/frame on 0.0.61 versus ~9.7 ms/frame on 0.0.62.
- Supplied-CDI, 1,000 TA packets, same local Clang Debug environment:
  - 0.0.61 raster: ~2,987.9 ms.
  - 0.0.62 raster: ~1,301.9 ms.
  - reduction: ~56%.
  - texture cache: 298 hits / 6 misses / 6 decodes / 393,216 decoded texels.
- End-to-end Debug wall time is not a meaningful Windows FPS comparison because generated SH-4 dominates without Release optimization; the commercial Windows launcher builds Release.

## Runtime telemetry

- Heartbeat: `pvr-tcache=hits/misses/decodes`.
- Shutdown PVR stats: `tex-cache=hits/misses/decodes/decoded-texels`.

## Final commercial generation

- Final 0.0.62 emitter regenerated the supplied CDI at 2,349 functions / 174,530 known SH-4 / 0 unknown / `RAW_SH4=0`.
- Final generated commercial target compiled and linked successfully with Clang 17 Debug (`dreamcast_program`).
- Final 1,000-packet acceptance probe reported `tex-cache=298/6/6/393216` and ~1,326 ms raster time; this is consistent with the earlier paired ~1,302 ms optimized measurement. The packet-limit termination is intentional.
