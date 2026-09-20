# DreamcastRecomp — next steps after 0.0.69

0.0.69 is intentionally focused on the first missing native callback observed after ChuChu Rocket! reaches the live 4P Battle board (`0x8C019B42`).

1. Verify 4P Battle continues past the first rendered board and identify the next gameplay blocker, if any.
2. Keep resolving missing runtime targets structurally with `RAW_SH4=0` until a stable playable slice exists.
3. Once gameplay remains alive long enough to inspect reliably, isolate the board-rendering glitch seen in the first 0.0.67 gameplay frame as a separate PVR correctness task.
4. Preserve the accepted Options BIOS-font fix, translucent-order fix, persistent A1 VMU, Maple DMA fixes and decoded-texture cache.
5. Defer the GPU-backed PVR backend until this gameplay path is stable.
