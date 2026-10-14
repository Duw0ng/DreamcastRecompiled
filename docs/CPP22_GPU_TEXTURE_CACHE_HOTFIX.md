# cpp22 — persistent GPU texture cache hotfix

## Problem observed in the Windows ChuChu Rocket run

The cpp20/cpp21 native GPU path was functionally active, but the raw texture snapshot was allocated anew for each deferred surface. The D3D11 texture cache used the snapshot object's address as its key, so an unchanged Dreamcast texture appeared to be a different host texture repeatedly.

The supplied run reached roughly 835k GPU texture decodes in about 4.8k frames (~173 decodes/frame on average), with zero software fallback materializations. The pipeline cache itself was healthy; the texture resource/decode churn was the dominant regression.

## cpp22 changes

- Raw Dreamcast texture snapshots are cached by texture identity plus the maximum write epoch of the VRAM pages actually covered by the texture.
- Unrelated VRAM writes no longer invalidate the raw snapshot.
- Palette snapshots are reused by palette write epoch + palette format.
- The existing pointer-keyed D3D11 texture cache now receives stable snapshot pointers while content is unchanged.
- A texture whose own VRAM pages change receives a fresh snapshot and one new compute decode.
- The 512-entry D3D cache no longer calls `clear()`; it evicts only the least-recently-used entry.
- New heartbeat telemetry: `pvr-gcache=rawHits/rawMisses/palHits/palMisses/lruEvictions`.
- No `DCRuntime` layout changes were made. This is deliberate so a previously compiled commercial closure can reuse all generated SH-4 object files.

## Fast test without recompiling the game

Keep these directories from the previous successful ChuChu build:

- `generated\official_v0.1.1\`
- `_cb_v0.1.1\`

Then run:

```bat
apply_cpp22_texture_cache_hotfix.bat
run_game.bat "C:\path\to\ccr.cdi" --reuse --renderer=dx11
```

Or drag the CDI onto `apply_cpp22_texture_cache_hotfix.bat`; after the incremental runtime relink it launches the cached game automatically.

The incremental build should compile `dc_runtime.cpp` and relink only. It should not compile `generated_program_part_XX.cpp` unless the existing build cache was deleted or invalidated.

## What to look for

In a healthy run, `pvr-gnative` compute-decode count should grow much more slowly than in cpp21 while `pvr-gcache` raw-hit count rises rapidly. `fallback_materializations` should remain near zero.
