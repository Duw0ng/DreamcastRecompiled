# Validation 0.0.143

Immediate experimental rollback: 0.0.142. Accepted stable rollback: 0.0.131.

## Trace conclusion

The supplied 0.0.142 log contains two Mouse Mania events. At comparable ~14-15k Type-8 vertices/frame, staged TA cost is materially lower than 0.0.141. The second event is substantially heavier, peaking near ~19k Type-8 vertices/frame, so its ~41 FPS minimum is workload-driven rather than evidence that `typed78` fell back.

## Change under test

Preserve the complete 0.0.142 runtime and optimize only the post-decoder strip-registration path:

- captured-stream replay resolves deferred state once per strip;
- immediate TA submission retains the old per-triangle state-cache path;
- accepted-vertex finite/extreme validation uses equivalent IEEE-754 bit tests;
- the fixed 3-slot strip ring avoids integer modulo.

No changes to AICA/CDDA, SH-4 timing, Store Queue/PREF provenance, Type-7/8 decoding, texture dirty tracking, GPU shaders, MT fallback, triangle/index ordering, or presentation.

## Correctness guards

- Vertex plausibility self-test compares the bit predicate with the exact 0.0.142 `std::isfinite/std::fabs` predicate for 131,072 deterministic full-width vertex patterns and explicit zero/extreme/NaN/Inf boundaries.
- Existing generic-vs-specialized vertex decoder and exact float-to-U8 tests remain active.
- Existing Type-8 staged bulk-path test remains active and requires zero decoder fallback.
- New staged translucent-strip regression requires 8 unique decoded vertices, 18 index references, 6 deferred triangles, exactly one deferred-state capture and zero per-triangle state-cache reuses.
- Immediate non-staged submission is explicitly routed through the 0.0.142 helper, preserving invalidation semantics when guest writes can interleave with TA packets.

## Results

- Main Release build: PASS (`--parallel 1`).
- CTest: 50/50 PASS.
- Fresh generated runtime configure/build: PASS.
- `generated_compile_test`: RC=0.
- Fresh generated `dreamcast_hello`: RC=0 and reports DreamcastRecomp 0.0.143.
