# Validation 0.0.142

Immediate experimental rollback: 0.0.141. Accepted stable rollback: 0.0.131.

## Change under test

Conservative Mouse Mania TA optimization: preserve 0.0.141 architecture and specialize only staged Type-7/Type-8 32-byte vertex runs. The hot decoder is called directly once the polygon header has fixed the format; UV16 uses one LE32 load; intensity float-to-U8 avoids `std::lround` while retaining its exact 0.0.141 result.

## Correctness guards

- No changes to AICA/CDDA, host audio worker, SH-4 timing, Store Queue/PREF provenance, GPU/MT renderer, indexed geometry, or 0.0.141 dirty-texture tracking.
- Float-to-U8 self-test compares the optimized path with the exact 0.0.141 formula for all 65,536 high-half IEEE patterns plus 131,072 deterministic 32-bit float patterns.
- Existing generic-vs-specialized vertex decoder equivalence self-test remains active.
- TA staging self-test now also exercises a Type-8 textured-intensity/UV16 stream, requires zero decoder fallback, and verifies `pvr_ta_typed_vertices[8]`.

## Results

- Main Release build: PASS (`--parallel 1`).
- CTest: 50/50 PASS.
- Fresh generated runtime configure/build: PASS.
- `generated_compile_test`: RC=0, including vertex-decoder, Type-8 staging, exact-U8 and texture dirty-region checks.
- Fresh `dreamcast_program`: RC=0 and reports DreamcastRecomp 0.0.142.
