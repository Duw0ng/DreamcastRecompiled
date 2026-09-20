# Validation 0.0.141

Experimental parent: 0.0.140. Stable rollback remains 0.0.131.

## Change under test

Flycast-style texture dirty regions adapted to DreamcastRecomp's fixed 128-slot decoded texture cache. Each slot registers its conservative VRAM page coverage. `pvr_mark_vram_write` ORs page membership into a two-word dirty mask, avoiding global texture invalidation and repeated page scans.

## Required checks

- Full core/test suite.
- Generated runtime compilation.
- `dc_pvr_texture_dirty_region_selftest`: unrelated VRAM write preserves clean active reuse; overlapping write marks the slot dirty and refreshes it in place.
- Existing TA staging and specialized decoder self-tests.
- Strict serial Windows build markers; no active `/MP`.

## Results

- Main Release build: PASS (`cmake --build ... --parallel 1`).
- CTest: 50/50 PASS.
- Fresh generated runtime configure/build: PASS.
- `generated_compile_test`: RC=0, including TA staging + dirty-region self-tests.
- Fresh `dreamcast_program`: RC=0 and reports DreamcastRecomp 0.0.141.
- Stable rollback remains 0.0.131.
