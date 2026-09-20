# DreamcastRecomp 0.0.37 — validation

## Main suite

```text
47 / 47 CTest tests passed
```

The emitter regression now explicitly requires generated support for PVR direct texture Store Queue apertures, 64-byte sprite assembly, RTT VRAM commits, SH-4 floating varargs, host-backed PVR statistics, and fast PVR shutdown.

## plasma.elf

The 0.0.36 Windows failure was:

```text
address=0x11165020 ... guest_pc=0x8C01A138
```

`0x8C01A138` lies in `sq_fast_cpy`; `0x11165020` is inside the PVR 64-bit direct texture upload aperture. 0.0.37 routes SQ commits in `0x11000000..0x117fffff` and `0x13000000..0x137fffff` into the VRAM backing.

Linux deterministic probe after the fix:

```text
TA packets=200
vertices=80
triangles=40
logical-frames=19
page-flips=19
```

A PPM dump contains the animated red plasma pattern instead of terminating on an unmapped memory write.

## bump.elf

0.0.36 Windows validation showed thousands of TA vertices but `triangles=0` and only a gray screen. The demo submits `pvr_sprite_txr_t`, a 64-byte primitive split across two 32-byte Store Queue bursts.

0.0.37 assembles the full sprite, expands it to two software triangles, and decodes the VQ `bricks.kmg` texture.

Representative deterministic probe:

```text
TA packets=100
vertices=80
triangles=40
sprites=20
logical-frames=9
page-flips=9
```

The generated frame visibly contains the 256x256 brick wall. Exact BUMP-map lighting is still a neutral fallback and remains a documented limitation.

## texture_render.elf

0.0.36 displayed moving geometry but flickered in render-to-texture mode, printed an impossible floating FPS value, returned zero from the guest PVR statistics, and appeared to hang/crash while shutting PVR down.

0.0.37:

- observes `pvr_scene_begin_txr` / `pvr_scene_begin_rtt` while still executing their guest bodies;
- commits RTT color output to the target VRAM texture without presenting that off-screen scene;
- samples the updated texture in the following visible scene;
- reads floating `printf` varargs from DR4+;
- fills a guest-compatible `pvr_stats_t` from runtime counters;
- replaces the expensive guest 8 MiB VRAM-clear shutdown loop with a no-observable-work host shutdown bridge.

Deterministic A-then-Start probe:

```text
6 frames in 31 ms = 193.548 FPS
VBlank Count: 2
Frame Count: 2
rtt=3
page-flips=2
exit=0
```

The exact wall-clock FPS is host-dependent; the validation criterion is that it is finite/plausible and no longer comes from integer-register garbage.

## 2ndMix regression

Final five-second native AICA capture after the PVR changes:

```text
Reachable functions: 190
mix-frames=220500
ARM faults=0
unsupported formats=0
bad reads=0
```

SHA-256:

```text
0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567
```

This is byte-identical to the stable 0.0.34-0.0.36 baseline.
