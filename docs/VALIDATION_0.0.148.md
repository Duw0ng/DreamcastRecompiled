# DreamcastRecomp 0.0.148 validation

- Parent: 0.0.147.
- Immediate experimental rollback: 0.0.147.
- Accepted stable rollback: 0.0.131.
- Scope: compact opaque/punch-through deferred representation: final R32 triangle-list indices plus compact state runs. Translucent per-triangle sort metadata is unchanged.
- Exact Type-8 strip regression: 4 vertices -> indices `0,1,2 / 1,2,3`, one compact opaque run, six references.
- Incomplete-strip regression: retains the rolling path and exact two-triangle output.
- Translucent regression: retains six `PVRDeferredTriangle` records for an 8-vertex strip and creates no opaque index records.
- Main Release build: PASS.
- CTest: 50/50 PASS before final version/documentation pass.
- Fresh generated runtime build: PASS.
- `generated_compile_test`: RC=0.
- `dreamcast_program`: RC=0 and identifies itself as DreamcastRecomp 0.0.148.
- No FPU arithmetic, QACR/PREF, TA packet ordering, PVR shader semantics, texture decoding, AICA/CDDA, GD-ROM, Maple or presentation changes.
