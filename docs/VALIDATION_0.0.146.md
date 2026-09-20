# Validation 0.0.146

Immediate experimental rollback: **0.0.145**. Accepted stable rollback: **0.0.131**.

## Scope

0.0.146 keeps the proven 0.0.145 TA staging/typed Type-7/8/strip-bulk performance path and concentrates on PowerVR2 guest-visible state and low-risk D3D11 state fidelity. A fresh audit was performed against the current Flycast PVR/TA implementation before packaging.

### Implemented in this build

- PowerVR2 register reset defaults aligned with Flycast for the registers currently exposed by DreamcastRecomp.
- Read-only handling for ID, revision, SPG status, TA next-OPB/current-ISP pointer and YUV count registers.
- TA_LIST_INIT initializes guest TA pointers and render-pass state.
- TA_LIST_CONT now preserves already-built scene geometry, advances the TA render-pass counter and resets only front-end/list/partial parsing state.
- TA_ITP_CURRENT advances with incoming 32-byte TA packets as a guest-visible approximation while the native renderer keeps its staged representation.
- SPG_STATUS derives scanline count from SPG_LOAD, reports programmed blank ranges and interlace field state instead of using a fixed 263-line status image.
- D3D11 applies PVR CullMode through dedicated none/front/back rasterizer states.
- The complete currently decoded PCW/ISP/TSP surface identity now retains Shadow, DCalcCtrl, CacheBypass, MipMapD, SupSample, ColorClamp, FogCtrl, DstSelect and SrcSelect so semantically different surfaces cannot merge into one draw state.
- TSP IgnoreTexA is applied in the pixel shader before ShadInstr.
- TSP ColorClamp uses FOG_CLAMP_MIN/MAX in the D3D11 pixel shader.
- Sampler cache layout reserves a separate mip-filter bit without colliding with U/V address-mode bits.
- Windows .bat/.ps1 package banners were audited and current build identifiers were normalized to DreamcastRecomp 0.0.146. Historical comparison references are intentionally retained.

### Deliberately unchanged

The proven 0.0.145 typed Type-7/8 decoder, exact-U8/UV16 conversion, staged TA FSM, strip-state reuse, arena-only indexed vertices, once-per-vertex Z validation, complete-strip bulk closure, page-scoped dirty textures, SH-4 execution/timing paths, AICA/CDDA, GD-ROM, Maple and normal presentation path are not rewritten by this build.

## Correctness guards

- Existing strip-bulk complete-EOL and incomplete-run fallback tests remain active.
- cpp_emitter_tests checks the new TA_LIST_CONT path and PVR reset constants.
- cpp_emitter_tests checks programmed SPG status/field handling.
- cpp_emitter_tests checks D3D11 front/back cull states.
- cpp_emitter_tests checks the newly retained PCW/ISP/TSP fields plus IgnoreTexA and ColorClamp shader behavior.
- Current heartbeat marker includes `tacont/spgprog/cullstate/tspfull/texa/clamp`.

## Results

- Main Release build: **PASS**.
- CTest: **50/50 PASS**.
- Fresh generated runtime configure/build: **PASS**.
- `generated_compile_test`: **RC=0**.
- Fresh generated `dreamcast_program`: **RC=0**, reports `DreamcastRecomp 0.0.146 native runner`.

## Known PVR/TA gaps from the fresh Flycast audit

These are intentionally not claimed as complete in 0.0.146:

- modifier-volume geometry/stencil semantics; current runtime still has a skipped-packet path;
- two-volume Area1 attributes/state for TA vertex formats 9-14;
- bump-map texel + shader equation; current texture decoder still has a neutral fallback;
- full fog-table/density/color shader semantics; FogCtrl is retained but not yet rendered;
- SrcSelect/DstSelect primary/secondary accumulation semantics;
- SupSample semantics;
- complete Dreamcast mip-chain upload and MipMapD/trilinear behavior (sampler state is prepared but textures remain one GPU mip level);
- TA YUV FIFO macroblock conversion and its full register/IRQ behavior;
- complete framebuffer/SPG/scaler/interlace behavior and cycle-accurate SPG IRQ timing;
- physical OPB/region-array/TA_OL_POINTERS fidelity;
- Flycast-equivalent translucent autosort/OIT behavior and remaining background/ISP edge cases.

See `docs/PVR_FLYCAST_AUDIT_0.0.146.md` for the detailed matrix.
