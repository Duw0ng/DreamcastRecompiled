# DreamcastRecomp roadmap — after 0.0.48

## 0.0.48 checkpoint

- G2/SPU DMA completion and Holly event path active.
- Guest-cycle PVR VBlank scheduling separated from host-clock AICA catch-up.
- Relocation-aware PC-relative semantics for dynamically copied Katana VBR code.
- Commercial closure expanded structurally to 2,194 functions / 165,555 known SH-4, 0 unknown, RAW_SH4=0.
- ChuChu reaches sustained TA traffic and advancing logical PVR frame/render/page-flip counters.

## 0.0.49 — first useful commercial frame

Primary target: replace remaining heuristic uncertainty with the retail render lifecycle actually requested by the game.

1. Trace the scene/list completion state that currently produces frames while guest `ISP_START` remains zero.
2. Verify framebuffer/VRAM addresses, display base changes and render-done ordering used by Kamui.
3. Validate the first non-empty commercial frame in the Windows PVR window and fix only reusable PVR2/Katana semantics it exposes.
4. Keep the 47-test suite and the KallistiOS corpus as regressions.

## Parallel dependencies after first frame

- Implement the ARM7 `LDM/STM ^` user-bank transfer semantics needed by the uploaded commercial audio firmware.
- Continue Maple/Katana controller input through the actual title path.
- Follow subsequent GD-ROM asset loads and texture formats.
- Move from boot/title rendering to menu navigation, then to the first playable ChuChu board.

No title-specific address seeds or protected-memory workarounds should be introduced; each new retail boundary should be converted into a reusable Dreamcast/Katana mechanism.
