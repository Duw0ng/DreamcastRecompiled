# VALIDATION 0.0.115

## Scope

0.0.115 is based on 0.0.114. The 0.0.113 TA vertex fast path and generated PREF-inline path remain removed. FMOV64 64-bit transfer, 0.0.112 FPU specialization, CDDA 0.0.106 and the low-memory serial build remain.

## Profiling basis

The live heavy-rat sequence now establishes the true load curve: approximately 24 FPS at peak density, then 28, 32.9 and 38 FPS as geometry falls. Delta analysis of the 24->28 interval shows roughly 28 host frames/s, ~482k TA packets/s, ~296k triangles/s, ~28k D3D11 draw calls/s (about 994 draw calls/frame), and ~4.3k decoded-texture cache misses/s (about 153 decodes/frame).

## Changes

1. GPU runs may span adjacent polygon `surface_id` boundaries when every real PVR render-state field, texture snapshot and background flag are equal. Provenance fields are not render state. Triangle order is preserved.
2. Decoded texture cache is 128 entries / 4-way / 32 sets instead of 64-entry direct mapped. Victims use per-set round robin after invalid ways are exhausted.

## Validation

- CTest: 47/47 PASS.
- Exact ChuChu commercial closure: 2965 functions, 295215 reachable instructions, 295215 known SH-4, 0 unknown, RAW_SH4=0, 32 shards.
- Generated commercial `dc_runtime.cpp` compiles at Release optimization.
- Heavy commercial shard `generated_program_part_30.cpp` compiles independently.
- No change to CDDA/AICA/GD-ROM/timing/Maple/VMU semantics.
