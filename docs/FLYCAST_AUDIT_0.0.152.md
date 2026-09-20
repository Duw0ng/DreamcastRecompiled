# Flycast parity/performance audit after 0.0.152

This audit separates changes likely to improve ChuChu Rocket! Mouse Mania performance from broader Dreamcast compatibility work.

## High-priority performance candidates

### 1. SH-4/FPU basic-block register cache

Flycast optimizes SHIL through SSA/constant propagation/dead-register removal and allocates SH-4 values to host registers. Its x64 backend has a dedicated register allocator and, on Windows, a pool of callee-saved XMM registers.

DreamcastRecomp is AOT rather than a JIT, so it does not need to copy that architecture literally. The useful idea is narrower: for hot basic blocks whose FPSCR bank/precision mode is stable, alias the active FR/XF bank once, load frequently used FR values into C++ locals, keep them live across adjacent FPU instructions, and write back only when the block or an observation barrier requires it.

The current emitter already specializes FTRV/FIPR/FMAC/FSRRA, but each guest instruction still resolves FPSCR/bank and materializes architectural array loads/stores. Mouse Mania telemetry shows these instructions increasing roughly fivefold versus ordinary gameplay, making block-local register residency the strongest CPU candidate after PVR strip compaction.

Safety gates for a first implementation:
- only PR=0 blocks;
- no FRCHG/FSCHG or FPSCR writes inside the cached region;
- flush before unknown/HLE calls or operations that can externally observe FPU state;
- preserve exact final float writeback semantics used by the existing FTRV/FIPR implementation;
- retain the old emitter as per-block fallback.

### 2. Compact TA staging records

`DCRuntime::PVRTAStreamPacket` is `alignas(32)` and contains a 32-byte TA packet plus source PC/source metadata. Because the structure's alignment is 32 bytes, its C++ size rounds to 64 bytes. The staged TA stream therefore consumes twice the raw packet footprint before vector overhead.

Flycast's parser operates over raw 32-byte TA DMA units and keeps parser state separately. DreamcastRecomp can approach that model without losing diagnostics by storing raw TA payloads contiguously and moving provenance to a compact side stream, or by enabling full provenance only while diagnostics are requested.

This is lower risk than a parser rewrite and should reduce cache/memory traffic in frames containing thousands of TA packets.

### 3. Persistent GPU texture-cache eviction

The current D3D11 cache clears the complete `gpu.textures` map when it reaches 512 entries. A more mature policy would retain entries by VRAM identity/dirty-page generation and evict incrementally. The 0.0.151 ChuChu log already showed very high texture reuse, so this is useful general optimization but not currently the primary Mouse Mania bottleneck.

## Compatibility/PVR parity still missing or partial

- Modifier-volume stencil semantics (OR/XOR/Inclusion/Exclusion/final shadow pass).
- Two-volume Area 0/Area 1 selection coupled to modifier volumes.
- Full fog table/density/color modes.
- Real bump-map equation.
- Full mip chains and PVR trilinear behavior.
- GPU-native paletted textures and palette lookup.
- Native 1555/565/4444 GPU formats where safe.
- GPU-resident RTT reuse without unnecessary VRAM round-trip.
- More complete framebuffer/SPG/interlace/scaler behavior.
- Broader GD-ROM command/timing fidelity.
- AICA DSP/AEG/LFO/ARM7 Thumb completeness.
- Full Maple multi-controller/peripheral coverage.

## Recommended sequence

1. Measure 0.0.152 `pvr-stripgpu` coverage and Mouse Mania FPS.
2. If index savings are substantial, keep the strip path and add cull-mode parity validation before widening coverage to CullMode 2/3.
3. Implement a conservative SH-4/FPU basic-block register cache with old-emitter fallback.
4. Compact TA staging records to 32-byte payload storage plus side metadata.
5. Continue PVR feature parity: modifier volumes + two volumes first, then fog/bump/mip/RTT/framebuffer.
