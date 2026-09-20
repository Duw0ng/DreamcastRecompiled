# DreamcastRecomp 0.0.27 — minimal hardware bootstrap

## Purpose

This release is the first controlled transition from CPU-only recompilation to a small Dreamcast hardware/runtime layer. It is intentionally a **bootstrap and diagnostic renderer**, not a PowerVR2 emulator.

The design goal is simple: let real KallistiOS/homebrew code run until it reaches a concrete unsupported behavior, while preserving strict errors for hardware that has not yet been modeled.

## Memory / device map currently represented

| Region | Bootstrap behavior |
|---|---|
| Dreamcast main RAM | 16 MiB backing store with P1/P2 physical aliases |
| PowerVR VRAM | 8 MiB backing store; common 32/64-bit apertures currently approximate aliases |
| Holly/PVR MMIO | 32-bit register backing plus selected initialization semantics |
| AICA RAM | 2 MiB backing store |
| AICA MMIO | basic register backing only; no sound CPU/audio |
| SH-4 P4/CCN | basic control-register backing, including QACR0/QACR1 |
| Store Queues | both 32-byte queues modeled |
| TA FIFO | receives committed SQ packets and feeds the software probe |

## PVR submission path

The important 0.0.27 path is:

```text
real KallistiOS/homebrew SH-4
        ↓
pvr_prim / sq_fast_cpy
        ↓
SH-4 Store Queue writes
        ↓
PREF
        ↓
QACR address translation
        ↓
PVR TA input
        ↓
32-byte packet collector
        ↓
minimal triangle-strip decode
        ↓
640x480 software framebuffer
        ↓
PPM dump
```

This keeps the actual recompiled KallistiOS Store Queue path in the test rather than replacing `pvr_prim()` with a host drawing function.

## 2ndMix real probe

Using the supplied KallistiOS `2ndmix.elf` and the graphics-only probe switches:

```text
[DreamcastRecomp probe] seeded default KOS video mode
DreamcastRecomp 0.0.27 native runner
Reachable functions: 181
Executing _main @ 0x8C0110D0u

2ndMix/KallistiOS starting
Initializing new PVR system
Initializing stars
Init font
Image is 256x256 (65536 bytes)
Drawing into 0xa4156020
Loading music
[DreamcastRecomp probe] skipping AICA music bootstrap
Starting display
[PVR bootstrap] MMIO reads=54 writes=136 | SQ commits=297144 | TA packets=35000 | vertices=34922 | triangles=11819 | render-starts=0
[DreamcastRecomp ERROR] PVR probe packet limit reached (35000)
```

The packet-limit error is an intentional deterministic stop.

`2ndmix_pvr_probe.png` is a PNG conversion of the resulting PPM software framebuffer. The image contains real geometry from 2ndMix's submitted vertex stream, including a strong starfield pattern and geometry from later lists, but it is **not visually equivalent to Dreamcast output**.

## Independent KallistiOS direct-rendering probe

`pvrmark_strips_direct.elf` was used as an independent path. A 500-packet run produced:

```text
Beginning new test: 33333 polys per frame (1999980 per second at 60fps)

[PVR bootstrap] MMIO reads=12 writes=50 | SQ commits=262644 | TA packets=500 | vertices=499 | triangles=49 | render-starts=0
[DreamcastRecomp ERROR] PVR probe packet limit reached (500)
```

`pvrmark_direct_probe.png` is the corresponding software framebuffer probe.

## CPU/runtime fixes discovered while enabling graphics

Real graphics paths uncovered general correctness issues that are now fixed independently of PVR:

- duplicate symbol names must be resolved by exact address;
- real GCC uses indirect/tail-call patterns that need durable target discovery;
- BRAF can be fed by signed byte jump tables embedded in executable sections;
- KallistiOS Store Queue copies use `FPSCR.SZ=1` paired FMOV operations;
- libgcc division helpers share branch tails across symbol/range boundaries;
- generated native functions therefore need valid alternate/basic-block entry dispatch.

These fixes have dedicated regressions and remain useful even if the PVR bootstrap is disabled.

## Known limitations

The software graphics probe currently ignores or approximates major PowerVR2 behavior:

- true Tile Accelerator binning and polygon sorting;
- ISP/TSP rendering pipeline;
- polygon-header render-state interpretation;
- textures;
- alpha blending and translucent-list ordering;
- depth semantics beyond a simple software approximation;
- clipping/culling details;
- modifier volumes;
- render interrupts, vblank and asynchronous timing;
- exact VRAM interleave / bus behavior.

AICA mapping only removes an address-space barrier; 2ndMix's S3M player is skipped in the graphics probe because no ARM7/audio core exists yet. Maple input is likewise bypassed only for deterministic graphics probes.

## Recommended next hardware milestone

0.0.27 should improve the existing PVR path rather than broadening randomly:

1. decode KallistiOS polygon headers/context words;
2. track opaque vs translucent lists;
3. implement basic source-alpha blending;
4. decode texture state and sample uncompressed VRAM textures;
5. identify logical scene/frame boundaries;
6. begin modeling render-done/vblank state enough for KallistiOS frame progression;
7. continue validating both 2ndMix and `pvrmark_strips_direct` after every change.


## 0.0.27 additions

The PVR bootstrap now decodes common KallistiOS polygon headers, preserves vertex U/V, samples 16-bit VRAM textures, approximates alpha blending and can present the host software framebuffer live through a Win32/GDI window. See `PVR_LIVE_0.0.27.md`.
