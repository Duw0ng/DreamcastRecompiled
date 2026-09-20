# PVR / TA audit against Flycast - DreamcastRecomp 0.0.147

This audit records what is actually present in the 0.0.147 source tree. It deliberately distinguishes **decoded/retained state** from **fully rendered hardware semantics**. The reference was the current `flyinghead/flycast` PVR/TA source (`pvr_regs`, `ta`, `ta_vtx`, `ta_structs`, `TexCache` and D3D11 renderer/shaders).

| Area | 0.0.147 status | Notes |
|---|---|---|
| TA Store Queue / PREF ingestion | Implemented | Existing native guarded path retained. |
| Staged TA stream + replay FSM | Implemented | Proven path retained. |
| Vertex formats 0-14 decode selection | Implemented/partial semantics | Two-volume Area1 payload is still not fully rendered. |
| Type-7 / Type-8 typed hot loop | Implemented | Kept unchanged. |
| Complete-strip bulk closure | Implemented | Incomplete/EOL-not-visible path falls back. |
| Indexed deferred geometry arena | Implemented | Kept unchanged. |
| TA_LIST_INIT | Improved in 0.0.147 | Initializes guest TA pointer state. |
| TA_LIST_CONT | Improved in 0.0.147 | Continues same scene/render-pass without closing accumulated geometry. |
| TA_ITP_CURRENT / TA_NEXT_OPB guest view | Partial | Current pointer advances observationally; physical ISP/OPB memory is not reconstructed. |
| OPB / region array / TA_OL_POINTERS | Partial | Native renderer does not need physical tile bins; guest-visible parity remains open. |
| Sprites | Implemented for existing path | Further cross-title edge testing remains useful. |
| User tile clipping | Implemented | Existing path retained. |
| CullMode | Implemented in D3D11 in 0.0.147 | none/front/back mapping follows Flycast D3D11 orientation. |
| Depth / blend core state | Implemented for existing game path | Remaining ISP edge cases still require cross-title testing. |
| PCW Shadow | Retained only | Full modifier-volume/shadow result is not complete. |
| DCalcCtrl / CacheBypass | Retained only | Guest surface identity is now correct; hardware side effects remain open. |
| MipMapD | Retained only | Full mip chain upload is not implemented yet. |
| SupSample | Retained only | Sampling behavior remains open. |
| IgnoreTexA | Implemented in D3D11 in 0.0.147 | Applied before ShadInstr. |
| ColorClamp | Implemented in D3D11 in 0.0.147 | Uses FOG_CLAMP_MIN/MAX. |
| FogCtrl | Retained only | Full 128x2 LUT, density and fog colors remain open. |
| SrcSelect / DstSelect | Retained only | Primary/secondary accumulation semantics remain open. |
| 1555 / 565 / 4444 textures | Implemented | Existing decoder/cache path. |
| Paletted 4/8 bit | Implemented for existing path | Palette/cache edge cases still require broader title tests. |
| Twiddled / VQ | Implemented for existing path | Existing path retained. |
| YUV texture/FIFO | Partial | Full TA YUV FIFO macroblock converter/register/IRQ parity remains open. |
| Bump map | Not complete | Current decoder still uses neutral fallback instead of raw 4444 + bump equation. |
| Modifier volumes | Not complete | Parser/runtime still contains skipped modifier-volume packet accounting. |
| Two-volume Area1 | Not complete | `two_volumes` is recognized but volume-1 vertex attributes/state are not fully carried to D3D11. |
| Translucent sorting | Partial | Native stable sort exists; Flycast autosort/per-strip/OIT equivalence remains open. |
| SPG_STATUS | Improved in 0.0.147 | Programmed total lines, blank range and interlace field; still read-driven rather than cycle accurate. |
| H/V blank IRQ timing | Partial | Full SPG scheduler parity remains open. |
| Framebuffer / scaler / interlace | Partial | Normal GPU-resident path remains intentional; full guest framebuffer/scaler parity still needs a dedicated pass. |
| Render-to-texture | Partial | Existing native path retained; full FB_W register/packmode/scaler parity still needs audit. |
| Background polygon / HALF_OFFSET / perpendicular cases | Partial | Existing behavior supports current title; hardware edge cases remain. |

## Performance interpretation

The new 0.0.147 items are primarily correctness/coverage changes. CullMode can reduce raster work when a title actually culls faces, but this build is **not** presented as a guaranteed Mouse Mania FPS increase. Previous live traces still point most strongly at TA/geometry/index bookkeeping before GPU submission. The purpose of the new `pvr-ta-reg` and retained TSP state is to rule out hidden compatibility/state-merging problems while keeping the proven Type-7/8 hot path intact.

## Recommended next implementation order

1. Modifier-volume geometry + D3D11 stencil/shadow semantics.
2. Two-volume Area1 sidecar/state and selection driven by modifier result.
3. Raw BUMP 4444 upload + Flycast bump pixel equation.
4. Fog LUT/density/colors and all FogCtrl shader modes.
5. Real mip-chain upload + MipMapD/trilinear.
6. SrcSelect/DstSelect accumulation path.
7. TA YUV FIFO converter.
8. Framebuffer/scaler/interlace and SPG scheduler fidelity.
9. OPB/region-array guest-visible state and broader translucent/OIT audit.

## 0.0.147 note

This build does not claim additional PVR feature parity beyond 0.0.146. It preserves that audited PVR/TA behavior and attacks SH-4 memory-feed overhead exposed by the latest Mouse Mania trace.
