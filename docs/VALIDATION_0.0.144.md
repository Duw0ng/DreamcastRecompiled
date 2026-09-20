# Validation 0.0.144

Immediate experimental rollback: 0.0.143. Accepted stable rollback: 0.0.131.

## Trace conclusion

The supplied 0.0.143 trace confirms the intended `stripstate` effect. At Mouse Mania density near 20k TA packets/frame, deferred-state cache traffic is roughly one quarter of the comparable 0.0.142 path, and sampled state/geometry CPU time is lower. The run also shows higher audio-starve/producer-gap history and lower absolute FPS outside the stress event, so the proven state optimization is retained rather than reverted based on the absolute FPS alone.

## Change under test

Preserve 0.0.143 and optimize only indexed strip geometry registration:

- indexed/deferred strips retain only arena indices in the rolling ring;
- triangle vertex references are read from the authoritative frame arena;
- GPU unit-Z eligibility is updated once per incoming vertex rather than for three overlapping vertices per triangle;
- state/texture GPU support is checked once when a staged strip resolves its deferred state;
- opaque triangles skip translucent-only sort-Z and sort-order calculation;
- hot arena/defer/append helpers are force-inlined.

No changes to Type-7/8 decoding, texture invalidation, triangle indices/order, translucent sorting semantics, D3D11 shaders, MT fallback, SH-4 timing/dispatch, AICA/CDDA, GD-ROM, Maple, or presentation.

## Correctness guards

- Existing generic-vs-specialized decoder, exact-U8, Type-8 staged bulk, texture dirty-region, native-PREF and vertex-plausibility tests remain active.
- Existing staged translucent strip still requires 8 unique vertices, 18 index references, 6 deferred translucent triangles, one state capture and zero per-triangle state-cache reuses.
- The same regression now also requires the full `pvr_strip_window` to remain untouched on the indexed path, proving that arena indices alone carry the strip while preserving output.
- Generated runtime self-tests execute successfully.

## Results

- Main Release build: PASS.
- CTest: 50/50 PASS.
- Fresh generated runtime configure/build: PASS.
- `generated_compile_test`: RC=0.
- Fresh generated `dreamcast_program`: RC=0 and reports DreamcastRecomp 0.0.144.
