# DreamcastRecomp 0.0.155 — validation

## FPS-first changes

- Default AOT FPU strategy is `superblock`; the proven 0.0.153 per-region cache remains selectable at runtime with `DCR_FPU_MODE=region` for one-binary A/B testing.
- Static superblocks use compile-time FR/XF sets and do not emit the 0.0.154 per-lane dynamic valid/dirty masks.
- Normal TA staging keeps the 32-byte aligned packet payload plus a one-byte hot source sidecar (`32+1`); exact source PC is cold optional provenance.
- Type-7/8 and TA capture metrics are batched, and telemetry-only SQ/FMOV/PREF writes are removed from the normal hot path. Detailed profiling remains available through the profiling/provenance modes.
- D3D11 `stripgpu`, finite-coordinate acceptance, PCW decoder locking, audio/CDDA and direct present remain unchanged from the validated previous lineage.

## Final validation

- Release/local suite: **50/50 CTest PASS**.
- Fresh generated FPU runtime: compiles and links.
- `generated_compile_test`: **RC=0**.
- Same generated `dreamcast_program` executed in both runtime-selectable FPU modes:
  - `DCR_FPU_MODE=region`: `R0=42 | FR0=1 | SR=0x1 | GBR=0x0 | PC=0xFFFFFFFF`
  - `DCR_FPU_MODE=superblock`: `R0=42 | FR0=1 | SR=0x1 | GBR=0x0 | PC=0xFFFFFFFF`
- Generated TA self-test remains included in `generated_compile_test` and completed successfully.

## Windows test workflow

1. Compile/run once with `run_commercial_recompiled.bat "path\\game.cdi"`. This defaults to `superblock`.
2. For an A/B without recompiling, relaunch the same generated executable through:
   - `run_commercial_recompiled_fpu_region.bat`
   - `run_commercial_recompiled_fpu_superblock.bat`
3. Compare the timestamped logs under `logs\\`, focusing on `fps=`, `fpu-mode=`, `fpu-cache=`, `fpu-super=`, `pvr-ta-pack=`, `pvr-ta-stage-ms=`, `pvr-cpu-est-ms=` and `pvr-gcpu-ms=`.
