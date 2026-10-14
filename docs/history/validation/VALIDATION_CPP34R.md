# Validation cpp34r — Network Fast Path + Submit Trace

## ABI / guest cache
- `runtime/include/dc_runtime.hpp`: SHA-256 `ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739` (identical to cpp34q/cpp34g ABI1).
- `src/codegen/cpp_emitter.cpp`: SHA-256 `e56050f0864d1c9272334d6e8a06214011383cbc641345f743286b7889444bf4` (identical to cpp34q).
- No generated ChuChu SH-4 functions are changed.

## Runtime/plugin build
- DreamcastRuntime: PASS.
- dcr_network: PASS.

## Protocol regression smokes
- Real-trace V.42 transition: ODP -> ADP -> raw HDLC flag -> SABME/UA -> PPP LCP: PASS (`ppp_tx=1`, `ppp_rx=2`, `lcp=1`).
- TCP sequence handling: normal, duplicate, overlap, gap, host reply: PASS.
- IPv4 Total Length tail clamp + intact 60-byte TCP payload: PASS.
- START submit trace rising-edge detection and PC sample: PASS.

## Performance microbenchmarks (same container, 2,000,000 iterations)
- Direct hot modem MMIO read: cpp34q ~192,665 us -> cpp34r ~12,473 us (~15.4x faster).
- End-to-end `dc_modem_read8` through the real plugin host: cpp34q ~194,572 us -> cpp34r ~38,157 us (~5.1x faster).
- Connected plugin tick with no work due: cpp34q ~230,828 us -> cpp34r ~10,952 us (~21.1x faster).

These are synthetic microbenchmarks, not a promise of the same FPS multiplier in the game. The real goal is removing the pathological mutex/4-KiB-copy/socket-poll overhead seen during ChuChu modem polling.

## Core suite
CTest defines 54 tests. The first run completed 53/54 before the execution-tool time limit, with no failures; the remaining `generated_program_build` test was then run separately and passed. Therefore every one of the 54 unique tests passed.
