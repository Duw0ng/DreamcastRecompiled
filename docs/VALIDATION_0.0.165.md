# DreamcastRecomp 0.0.165 validation

## Parent and scope

- Parent: live-tested 0.0.164.
- Immediate rollback: 0.0.164.
- Stable rollback: 0.0.131.
- Measured high-FPS reference: 0.0.156.
- Host/AICA cadence remains `host-sync-q=262144`.
- Post-Present host sync remains enabled.
- Production function-wide GPR cache remains disabled.
- No scheduler, AICA, CDDA, texture, blend, depth, cull, translucent sort or Present timing changes.

## Live evidence from 0.0.164

The supplied 0.0.164 session proved that `flystrip164` covered essentially the entire hot Type-7/8 strip workload with zero direct-strip fallbacks, while light scenes reached the mid-80 FPS range. The heavy Mouse Mania floor nevertheless remained around 39-43 FPS at roughly 20k+ TA packets/frame.

That isolates the next cost after vertex decode: opaque strip/index/state commit. 0.0.164 still wrote the sequential R32 strip indices during TA parsing and then copied that complete opaque index array again into D3D11 scratch before upload.

## 0.0.165 late-strip architecture

Eligible native opaque strips now retain a compact descriptor in the existing opaque submission-order vector:

- `topology=2`
- `first_index = firstVertex`
- `index_count = vertexCount`
- `state_index = already-resolved deferred state`

No R32 entries are appended to `pvr_deferred_opaque_indices` for that strip during TA replay.

At GPU scene submission, `pvr_gpu_build_opaque_stream()` walks opaque runs in original submission order:

- legacy triangle-list / already-materialized strip blocks are copied once from the fallback index arena;
- topology-2 descriptors synthesize `firstVertex..firstVertex+count-1` followed by `0xFFFFFFFF` directly into D3D11 index scratch;
- adjacent GPU runs with equivalent state/topology are merged after the final scratch positions are known.

This removes one complete index write/copy stage for the dominant native-strip path.

## Software fallback

The multithreaded software fallback does not materialize temporary indices for topology-2 runs. It reconstructs the strip triangles directly from the contiguous vertex range. This keeps D3D11 failure/unsupported-frame behavior available without paying the index cost during normal gameplay.

## Telemetry

New heartbeat field:

`pvr-latestrip=<runs>/<vertices>/<indices-materialized>/<TA-index-writes-avoided>/<GPU-run-merges>`

Expected normal D3D11 behavior:

- `runs` and `vertices` should track most of `pvr-flystrip` / native strips;
- `TA-index-writes-avoided` should grow rapidly during Mouse Mania;
- `indices-materialized` should be similar in final size but written only once, at GPU build time;
- `GPU-run-merges` quantifies state-compatible final batching.

Marker: `.../flystrip164/latestrip165`.

## Automated validation

- Release host build: PASS.
- CTest: 50/50 PASS.
- Generated Type-8 regression now requires zero TA-time opaque indices plus one topology-2 first/count descriptor: PASS.
- Incomplete Type-8 path remains on the previous rolling/fallback representation: PASS.
- Generated C++ project configure/build: PASS.
- `generated_compile_test`: PASS.
- Generated `dreamcast_hello`: PASS; returns with `PC=0xFFFFFFFF`.

Commercial ChuChu Rocket! was not executed in this environment. Performance claims require the user's Windows live log.
