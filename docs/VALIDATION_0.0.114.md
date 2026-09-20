# VALIDATION 0.0.114

0.0.113 was rejected after live Windows profiling showed a regression from the 0.0.112 floor (~31–32 FPS) to ~22–24 FPS in the heavy rat scene.

0.0.114 is intentionally based on 0.0.112 and removes both 0.0.113 experiments that touched the TA/PREF path. The only retained 0.0.113 change is the isolated 64-bit FMOV memory path.

Validation:
- 47/47 CTest PASS.
- Commercial closure: 2965 functions, 295215 known SH-4 instructions, 0 unknown, RAW_SH4=0.
- 32 commercial shards retained.
- Generated commercial code contains dc_read64_fmov_hot/dc_write64_fmov_hot.
- Generated commercial code contains neither the 0.0.113 fast_vertex TA shortcut nor generated dc_pref_sq call sites.
- Heavy commercial shard generated_program_part_30.cpp compiles successfully with optimization.
- Windows build remains serial/low-memory: --parallel 1, no /MP, cache retained.

Live target: compare the same heavy scene against 0.0.112 (31–32 FPS minimum, up to 57 FPS maximum). If the floor recovers, TA/PREF was the 0.0.113 regression source; if not, revert FMOV64 as well.
