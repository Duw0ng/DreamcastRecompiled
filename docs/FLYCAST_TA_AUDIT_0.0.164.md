# Flycast TA audit for DreamcastRecomp 0.0.164

Audit date: 2026-09-07. Reference: current `flyinghead/flycast` master, focusing on `core/hw/pvr/ta.cpp`, `ta_vtx.cpp`, and `ta_ctx.cpp`.

## What Flycast does that matters here

### 1. Compact TA packet front path

`ta.cpp` moves 32-byte TA packets into the active TA buffer while updating a compact front-state machine. Its multi-packet input path explicitly handles several 32-byte packets per loop rather than forcing every packet through a large generic dispatch layer.

### 2. Concrete vertex handlers + range-based strip close

`ta_vtx.cpp` uses Object Control to choose a concrete TA vertex handler. Type-7/Type-8 handlers append decoded vertices directly. `EndPolyStrip()` computes the strip count from the difference between the current vertex-list size and the polygon's first vertex, then stores the polygon parameter. In other words, strip ingestion is centered around contiguous vertices and a `first/count` range, not repeated per-triangle materialization while parsing.

### 3. Context recycling

`ta_ctx.cpp` keeps a pool of TA contexts. Recycled contexts are reset and returned to that pool, avoiding a design that continuously destroys/recreates working storage.

## Mapping to DreamcastRecomp

DreamcastRecomp already had several equivalent ideas before 0.0.164:

- staged 32-byte TA scene stream and front FSM;
- specialized Type-7/Type-8 decode;
- indexed persistent frame arenas;
- native D3D11 strip topology with primitive restart;
- vector `clear()` behavior that preserves capacity.

The remaining mismatch was that a complete typed strip still entered generic strip-append bookkeeping once per vertex before finalization. 0.0.164 adds a narrow Flycast-style whole-strip decoder: one arena growth, a tight concrete typed loop, and one existing finalizer invocation.

The version also warms persistent high-water capacities for the vertex arena, R32 indices and state runs, which complements the already persistent frame vectors.

## What 0.0.164 intentionally does NOT import

- No host/AICA scheduler timing change.
- No frame skipping.
- No game-specific ChuChu hacks.
- No PVR texture/depth/blend/cull semantic relaxation.
- No translucent-order relaxation.
- No wholesale renderer/OIT architecture transplant.

Those would mix correctness/timing changes into a release whose purpose is specifically to reduce TA CPU work.

## Live validation marker

Heartbeat field:

`pvr-flystrip=<runs>/<vertices>/<fallbacks>`

For ChuChu Rocket!, expected success is a rapidly increasing first/second counter and a fallback counter that remains very small relative to successful strips. The FPS comparison should use the same Mouse Mania packet-load ranges as 0.0.163.
