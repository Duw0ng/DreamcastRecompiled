# Flycast SH-4/FPU audit for DreamcastRecomp 0.0.166

Audit target: Flycast master commit `52c8ecef1e67935b98e7ea3cfb0acd36eed787d6` (2026-09-06).

Relevant upstream files:
- `core/hw/sh4/dyna/ssa.cpp`
- `core/hw/sh4/dyna/ssa_regalloc.h`
- `core/rec-x64/rec_x64.cpp`
- `core/rec-x64/x64_regalloc.h`
- `core/hw/sh4/dyna/shil_canonical.h`
- `core/rec-ARM64/rec_arm64.cpp`

## Findings transferred to 0.0.166

1. **Block-local version/liveness beats function-wide architectural caching.** Flycast versions SH-4 values inside each runtime block, preloads on first source use, keeps dirty scalar values in host registers, and writes back only when later code or an architectural barrier actually needs the value. DreamcastRecomp 0.0.155 superblocks instead build a function-wide FR/XF membership mask, which is safe for multi-entry AOT functions but can load/write many lanes at high-frequency barriers.

2. **Scalar FPU values should stay as host floating values.** Flycast x64 maps scalar FR values to XMM registers and emits scalar `addss/subss/mulss/divss` directly. 0.0.166 therefore changes the block-local cached representation from `uint32_t` locals plus repeated `bit_cast<float>` to native `float` locals. Architectural bit representation is restored only on loads/stores/barriers and bit-semantic instructions.

3. **Vector operations must not consume the scalar allocator.** Flycast's generic allocator deliberately flushes/handles vector operands separately when they exceed its scalar/vector allocation size. On Win64 it has ten callee-saved XMM registers (XMM6-XMM15), not enough to profitably pin a full 16-lane matrix plus scalar FR temporaries. 0.0.166 therefore keeps the FTRV XF matrix context-backed while scalar FR stays block-local.

4. **FMAC deserves direct host FMA.** Flycast x64 lowers `shop_fmac` to `vfmadd231ss` when FMA is available, otherwise `mulss + addss`. The supplied 0.0.165 session executed roughly 201 million FMAC operations, making this a high-value target. Generated Win64 Release builds in 0.0.166 expose `DCR_HOST_AVX2` (default ON) and use a force-inline FMA3 primitive under `/arch:AVX2`; portable builds keep `std::fma` single-rounding semantics.

5. **Do not blindly SIMD FIPR/FTRV.** Flycast contains vector implementations in some backends but ARM64 explicitly retains canonical fallback for better precision. Its canonical FIPR/FTRV uses double-precision accumulation before the final float result. DreamcastRecomp keeps the existing double-accumulated numerical shape.

## Deliberately not copied

- No function-wide GPR/FPR cache expansion.
- No cross-basic-block FR persistence in the production 0.0.166 path.
- No approximate SIMD reduction for FIPR/FTRV.
- No scheduler, host-sync, AICA, CDDA, PVR timing, TA ordering, texture, blend, depth or Present changes.

## 0.0.166 production structure

`blockssa166` is the production default. It reuses the proven cache-region boundaries but changes the representation and pressure model:

- only FR scalar inputs are loaded into native `float` locals;
- write-only FR values are created without an architectural preload;
- final dirty FR values are written once at a real region/control boundary;
- XF matrix values are read directly from the correct architectural bank only by FTRV;
- unsupported FPSCR modes still execute the unchanged exact fallback;
- legacy `superblock` remains runtime-selectable for A/B/rollback.

Heartbeat telemetry:

`fpu-ssa=<regions>/<scalar-loads>/<scalar-writebacks>/<matrix-lane-reads>/<fmac-ops>`

`host-fma=fma3|portable`

The live test, not packaged unit tests, determines whether this reduces Mouse Mania frame time.
