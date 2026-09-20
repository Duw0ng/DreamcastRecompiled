# PVR / TA audit against Flycast - DreamcastRecomp 0.0.148

0.0.148 keeps the 0.0.146 PVR/TA feature-parity work and 0.0.147 SH-4 hot leaves intact. The new change is architectural geometry storage, motivated by the supplied 0.0.147 Mouse Mania trace and checked against the current Flycast PVR renderer organization.

## Geometry representation

Flycast's `rend_context` keeps a contiguous vertex vector and index vector. Its `PolyParam` records carry `first` and `count` ranges plus polygon state; the D3D11 renderer consumes those ranges rather than constructing a heavyweight object per opaque triangle. Sorted translucent output is handled separately.

DreamcastRecomp 0.0.147 already had a contiguous decoded vertex arena but still materialized every opaque/punch-through triangle as `PVRDeferredTriangle {i0,i1,i2,state,sort_z,order}` and later expanded those structs back into an R32 index buffer.

0.0.148 removes that mismatch for non-translucent geometry:

- opaque/punch-through triangles append directly to `pvr_deferred_opaque_indices`;
- `PVRDeferredOpaqueRun {state_index, first_index, index_count}` preserves ordered state ranges;
- complete staged strips append their remaining indices as one contiguous block;
- adjacent identical-state blocks merge while being produced;
- the D3D11 builder still coalesces adjacent *equivalent* deferred states, preserving the previous draw-call behavior;
- the software MT fallback reads the same run/index representation;
- translucent geometry keeps `PVRDeferredTriangle`, `sort_z`, `order` and stable sorting unchanged.

This revision deliberately retains D3D11 `TRIANGLELIST`. Flycast normally draws unsorted polygon lists as `TRIANGLESTRIP`, but changing primitive topology would also change winding/cull behavior and draw batching. That is a separate fidelity/performance step and is not mixed into 0.0.148.

## Live-log conclusions carried into 0.0.148

- `stripbulk` has zero fallbacks in the supplied 0.0.147 trace.
- `pvr-ta-reg` shows no `TA_LIST_CONT` activity in the measured ChuChu Rocket scene.
- At the heavy peak, only about 9% of triangles are entering the translucent sorter; roughly 91% are suitable for the compact opaque representation.
- 0.0.147 maintains roughly the 0.0.146 matched-load FPS level rather than producing a clear large independent gain, so 0.0.148 targets a different measured cost instead of extending the same SH-4 hot-leaf idea.

## Remaining major Flycast parity gaps

Unchanged from the prior audit: modifier volumes/stencil shadow behavior, complete two-volume Area1, bump mapping, full fog LUT/density/colors, true mip chains/trilinear behavior, SrcSelect/DstSelect accumulation, TA YUV FIFO conversion, fuller framebuffer/scaler/interlace behavior, cycle-accurate SPG scheduling, guest-visible OPB/region-array detail, and broader translucent autosort/OIT parity.
