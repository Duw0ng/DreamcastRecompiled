# Flycast parity/performance audit after 0.0.154

## Applied in 0.0.154

1. **TA coordinate acceptance:** Flycast's TA path does not reject finite vertices by an arbitrary screen-magnitude threshold. DreamcastRecomp now follows the same principle and delegates visibility to clipping.
2. **Host FPU register residency:** Flycast's x64 dynarec uses SSA/register allocation and can retain guest FPU values in XMM registers. DreamcastRecomp now applies a conservative AOT analogue: live FR/XF values are held in generated locals inside cache-safe single-precision regions, allowing MSVC/Clang to keep them resident.

## Next performance candidates (do not combine until 0.0.154 is measured)

1. Compact `PVRTAStreamPacket`: the current `alignas(32)` raw packet plus metadata rounds to 64 bytes. Split raw 32-byte TA data from metadata sidecar to reduce cache pressure during dense scenes.
2. Persistent GPU-native texture formats/palettes to reduce CPU decode/upload work where games exercise palette animation or 16-bit texture formats.
3. GPU-resident RTT reuse to avoid GPU→CPU→GPU round trips in RTT-heavy titles.

## Next compatibility candidates

- Modifier-volume stencil behavior and Two-Volume/Area1 selection.
- Fog table/density/color modes.
- Bump-map equation.
- Full mip chain/trilinear behavior.
- More complete framebuffer/SPG, GD-ROM, AICA DSP/AEG/LFO and Maple device coverage.
