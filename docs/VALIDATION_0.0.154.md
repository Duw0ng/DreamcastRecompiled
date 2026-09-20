# DreamcastRecomp 0.0.154 — validation

0.0.154 targets the remaining CPU/cache pressure exposed by the 0.0.153 full-session trace: frequent FR/XF region synchronization and the 64-byte aligned TA staging record.

## Persistent FPU trace cache

Generated hot functions declare one FR/XF cache before their dispatch switch. Cache-safe regions lazily populate named `fc_fr*`/`fc_xf*` locals and mark dirty lanes, but no longer write those lanes back at every region end. Internal branches can carry the cache into the next basic block.

The generated `fc_tick` preserves the existing SH-4 batching rule: it accumulates cycles exactly like `dc_runtime_tick_fast`; when the 256-cycle batch is reached it flushes FR/XF and then calls the unchanged `dc_runtime_tick_full`. Architectural barriers and function/external exits flush explicitly. Ordinary uncached FPU helpers also flush before reading `ctx.fr_bits/xf_bits`.

Telemetry: `fpu-trace=entries/guard-reuses/lazy-loads/actual-writebacks/tick-flushes/barrier-flushes/exit-flushes/cross-block-keeps`.

## TA staging compaction

The previous `PVRTAStreamPacket` contained a 32-byte aligned payload plus provenance. Because the struct itself had 32-byte alignment its size rounded to 64 bytes. 0.0.154 uses:

- `PVRTAStreamRaw`: exactly 32 bytes, aligned to 32.
- `PVRTAStreamMeta`: provenance sidecar, at most 8 bytes.

The parser retains the same packet order, Type-7/8 typed loops, continuation state and source accounting. `pvr-ta-pack=32+8/64/<bytes-saved>` reports cumulative structural savings.

## Validation results

- Main Release build: PASS.
- Project CTest suite: **50/50 PASS**.
- Generated compile/self-test with TA staging regressions: PASS / exit 0.
- Generated FPU sample, persistent cache active: `R0=42 | FR0=2 | SR=0x1 | PC=0xFFFFFFFF`.
- Same generated sample with every cache guard forcibly false: identical final state.
- 0.0.153 `stripgpu`, finite-coordinate acceptance, audio/CDDA, GD-ROM, Maple and D3D11 direct-present logic are unchanged.

Windows ChuChu Rocket! testing should compare `fps`, `fpu-cache`, new `fpu-trace`, `pvr-ta-pack`, `pvr-ta-stage-ms`, `pvr-cpu-est-ms`, `pvr-stripgpu`, `pvr-badv`, `pvr-shortbad`, and `pvr-xaccept`.
