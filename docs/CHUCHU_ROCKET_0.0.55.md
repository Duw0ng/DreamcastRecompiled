# ChuChu Rocket! acceptance — 0.0.55

## Windows checkpoint from 0.0.54

The supplied Windows run crossed the title screen and the previously blocked `0x8C026F2E` state dispatcher, reaching the game's Japanese memory-card/save selection screen. This confirms the 0.0.54 closure work is active on the real commercial path.

On confirmation/START from that screen the runner failed at:

```text
Dreamcast memory access is outside mapped RAM/VRAM/MMIO:
address=0xA010A070 physical=0x0010A070 width=1 guest_pc=0x8C0227D0
```

The immediately preceding trace contains:

```text
0x8C0227A2 -> 0x8C0F21E0
0x8C0F21E0 -> 0x8C001002
```

`0x8C001002` is the retail BIOS font-vector HLE. Function 0 returns `0xA0100020`; the later `0xA010A070` byte access is therefore inside that expected font-ROM aperture, not random RAM corruption.

## 0.0.55 functional fix

The runtime now maps physical `0x00100000..0x0011FFFF` as a read-only 128 KiB synthetic BIOS-font ROM and therefore accepts P1/P2 aliases such as `0xA010A070`. No retail BIOS image or copyrighted font bytes are distributed; the backing is deterministic zero-filled data for now.

The KOS/native font-address HLE is unified with the retail BIOS path and also returns `0xA0100020` rather than allocating a separate main-RAM shadow.

## Controller acceptance

The user's 0.0.54 run showed sustained Maple DMA (`968` DMA runs / `967` GETCOND responses), so the controller protocol itself is active. 0.0.55 keeps the existing Dreamcast button bits and adds easier keyboard aliases plus source diagnostics:

- arrows: D-pad
- `Z`, Space or `J`: Dreamcast A
- `X` or `K`: Dreamcast B
- `C` or `U`: Dreamcast X
- `V` or `I`: Dreamcast Y
- Enter: START
- WASD: analog stick
- Q/E: analog triggers
- XInput controller: supported as before

Heartbeat fields `maple-input`, `maple-src` and `maple-changes` distinguish a host-key capture problem from a guest Maple-consumption problem in the next Windows run.

## Performance checkpoint

The 0.0.54 Windows profile near the save menu reported:

```text
pvr-frames=744
host-aica-ms=4236
pvr-prof-ms=105908/311/245
```

That is about 142 ms of accumulated software-raster work per rendered frame, while presentation and clear are negligible and AICA catch-up is much smaller. 0.0.55 therefore targets PVR raster/texture hot paths rather than changing AICA timing or lowering render resolution.

A controlled local 640x480 full-screen RGB565 bilinear microbenchmark measured a median of roughly 41.2 ms/frame with the untouched 0.0.54 runtime and roughly 24.7 ms/frame with the 0.0.55 runtime, about 40% less raster time (~1.67x throughput). This benchmark is intentionally synthetic; the next Windows ChuChu run is the authoritative game-level measurement.

## Static closure

Unchanged from 0.0.54:

- 2,273 reachable functions
- 170,808 known SH-4 instructions
- 0 unknown SH-4 instructions
- `RAW_SH4=0`
