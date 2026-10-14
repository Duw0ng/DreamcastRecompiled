# cpp38e — HOT BLOCK RECOVERY

## Goal

Recover host execution cost exposed by cpp38d around ChuChu Rocket hot PCs `0x8C0FE5A0` and `0x8C1088DA` without skipping guest code, changing Dreamcast clocks, or adding another wait fast-forward.

## What changes

cpp38e keeps the cpp38c/cpp38d runtime behavior and HOTPC profiler, but HOTPC is **OFF by default** for clean FPS measurements.

For the cached `CHUCHU_ROCKET` ABI1 guest, cpp38e performs a one-time host-code rebuild from the **existing generated SH-4 source** (no CDI rediscovery and no SH-4 closure re-analysis) with these already-supported specializations:

- `DCR_ASSUME_MMU_OFF=1` — inlines the common 32-bit main-RAM path inside the guest DLL. This removes the imported `dc_read32_hot`/`dc_write32_hot` hop for ordinary SDRAM accesses. The latest cpp38d run remained `sh4-mmu=off/0/0`.
- `DCR_FIXED_SH4_TICK_BATCH=256` — constant-folds the already-used 256-cycle threshold.
- `DCR_DISABLE_HOT_TICK_CALL_COUNTER=1` — removes the per-fast-tick diagnostic increment.
- `DCR_LIGHTWEIGHT_HOT_METRICS=1` — removes successful hot dispatch counter writes.
- `DCR_LIGHTWEIGHT_CURRENT_PC=1` — avoids redundant `runtime.current_pc` stores on every generated block; `ctx.pc` remains authoritative and full scheduler boundaries still synchronize state.

Additionally, cpp38e fixes the direct-hint success path so `dynamic_direct_hint_hits` uses `DCR_HOT_METRIC_INC`. cpp38d was still incrementing that counter unconditionally; the analyzed ChuChu run accumulated roughly 820 million direct-hint hits around the measured 12.8k-frame point.

## What does NOT change

- No hardcoded skip for `0x8C0FE5A0` or `0x8C1088DA`.
- No guest clock increase.
- No PVR/SPG timing change beyond the already-existing cpp38c behavior.
- No IRQ/TMU/AICA/GD-ROM timing shortcut.
- No new SH-4 targets or closure changes.
- Guest ABI remains v1.

## First run

Run:

```bat
TEST_CPP38E_FAST.bat "C:\path\to\ccr.cdi"
```

If the ChuChu guest is still the cpp38d/cpp38b2 implementation, the first cpp38e run rebuilds `dcr_game_module.dll` once using the cached generated source. Subsequent runs reuse it without recompiling.

For the clean benchmark, compare the same gameplay segment against cpp38c/cpp38d BASE with HOTPC disabled. Because lightweight hot metrics intentionally stop counting successful hot-path events, do not compare `dispatch-cache`, `dispatch-hint` hit totals or `sh4_tick_calls` as if they were complete. Compare FPS, frame count, visible behavior, audio, PVR packets/triangles, crashes/faults, and full-tick/device behavior.

Optional diagnostic run after the clean benchmark:

```bat
TEST_CPP38E_HOTPC_DIAG.bat "C:\path\to\ccr.cdi"
```

To restore the pre-cpp38e guest DLL and generated CMake file:

```bat
RESTORE_CPP38D_GUEST_BASELINE.bat
```
