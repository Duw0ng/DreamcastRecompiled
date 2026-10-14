# DreamcastRecompiled v1.2 — Stable Base

Stable baseline: **cpp38e HOT_BLOCK_RECOVERY**
Frozen: **2026-10-07**

This package is the official stable development baseline for DreamcastRecompiled v1.2.
Future experimental branches (cpp38f and later) should start from this state unless explicitly rebased elsewhere.

## Validation target

ChuChu Rocket! was used as the primary regression/performance validation target.
Observed clean-run performance in the accepted cpp38e test log:

- ~333 PVR frames: 72.9 FPS
- ~12,794 PVR frames: 53.9 FPS
- ~14,373 PVR frames: 61.0 FPS
- SH-4 MMU remained disabled (`sh4-mmu=off/0/0`)
- No HOTPC profiler was required for the clean FPS validation

## Stability policy

Do not modify this stable package in-place. Create the next experimental package from this baseline and preserve this archive for rollback/comparison.
