# Flycast TA / renderer audit for DreamcastRecomp 0.0.165

Audit date: 2026-09-07

Public reference files:

- `core/hw/pvr/ta_vtx.cpp`
- `core/rend/dx11/dx11_renderer.cpp`
- repository: https://github.com/flyinghead/flycast

## Relevant Flycast structure

Current Flycast closes a polygon strip by setting the active `PolyParam` count from the contiguous vertex array and pushing the parameter/range. The TA parser therefore preserves strip identity as a first/count range instead of requiring per-triangle metadata at parse time.

Its DX11 renderer binds triangle-strip topology for ordinary polygon lists and submits `DrawIndexed(params->count, params->first, 0)` per stored polygon parameter. Translucent sorted geometry is handled separately as triangle lists when sorting semantics require it.

## What DreamcastRecomp already had by 0.0.164

- 32-byte staged TA stream.
- Object-Control-selected typed Type-7/8 decode.
- Contiguous per-frame vertex arena.
- Native D3D11 triangle strips with `0xFFFFFFFF` primitive restart for cull-safe opaque/punch strips.
- Compact deferred states and state-run merging.
- Exact translucent sorted-triangle fallback.

The remaining mismatch was data lifetime: DreamcastRecomp still expanded every eligible strip into an R32 runtime index stream during TA finalization, then copied the whole opaque index stream into the GPU scratch array before upload.

## 0.0.165 adaptation

0.0.165 adopts the first/count lifetime more literally without copying Flycast renderer internals wholesale:

1. TA finalization keeps native strips as `firstVertex/vertexCount/stateIndex` descriptors.
2. Opaque submission order is preserved in the same compact run vector.
3. The final R32 cut-index stream is created once at renderer submission.
4. Equivalent adjacent GPU runs are merged at that final stage.
5. Software fallback reads the vertex range directly instead of forcing early index materialization.
6. Translucent sorting is unchanged.

This is intentionally narrower than replacing the renderer with Flycast's polygon-list architecture. It removes a proven duplicate memory pass while preserving DreamcastRecomp's established D3D11/fallback behavior.

## Not copied

- Flycast per-strip translucent sorting modes/OIT architecture.
- Full Flycast PolyParam renderer state model.
- Renderer threading/context queue design.
- Any SH-4/AICA/scheduler changes.

Those remain separate future audit areas and should only be adopted if live profiling shows they dominate.
