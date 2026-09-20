# DreamcastRecomp 0.0.126 — validation

## Live evidence

0.0.123 restored the known-good 0.0.120 performance. In the supplied maximum-rat profile, heartbeat #76→#77 spans 40 rendered frames while `perf-device` moves from `6410/8399/2554` to `6446/8762/2575`. That corresponds to roughly 0.9 ms/frame sampled in PVR clock scheduling and 9.1 ms/frame in HOST_SYNC. Over the same interval `host-aica-ms` rises only from 3345 to 3385 ms, so interpreted ARM7 catch-up accounts for only about 1.0 ms/frame. The remaining host-sync cost is dominated by host-paced PCM/CDDA production and associated output staging.

## 0.0.126 optimization

- Active AICA slots are tracked by a 64-bit mask and only those slots are visited at the 44.1 kHz mix rate.
- Stable per-slot pan/send/total-level gains are cached outside release envelopes.
- Master MVOL/mono gain is cached on register write rather than re-reading/recomputing it for every PCM frame.
- PCM16 sample fetch uses one contiguous little-endian read with explicit RAM-wrap fallback.
- WinMM staging writes one interleaved stereo frame directly and checks chunk submission only at the chunk boundary.
- All guest timing and audio math remain unchanged.

## Local validation

- CTest: 47/47 PASS.
- Freshly emitted host runtime/program: CMake configure/build PASS.
- `generated_compile_test`: exit 0.
