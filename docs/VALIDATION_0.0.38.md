# DreamcastRecomp 0.0.38 — validation

## Main suite

```text
47 / 47 CTest tests passed
```

The emitter regression requires the palette decoder, bilinear sampler, stride path, U/V wrap modes, 16-bit U/V path and punch-through reference path in every standalone generated runtime.

## Full KallistiOS corpus

User-supplied corpus (not redistributed with DreamcastRecomp):

```text
ELF found:              155
ELF loaded/scanned:     155
normal dc_recomp emit:  155 / 155
_main RAW_SH4=0:        154 / 155
```

The sole decoder/DCIR exception remains `lua/basic/lua.elf::_llex` with words `0x0118`, `0x0119`, and `0x011B`.

## PVR texture acceptance set

All probes below were regenerated from the final 0.0.38 recompiler and built as standalone native programs. Packet-limit termination is intentional for looping demos.

### palette/4bpp

```text
TA packets=120
vertices=48
triangles=24
tex-samples=3686400
bilinear=3686400
paletted=3686400
logical-frames=11
page-flips=11
```

Deterministic PPM SHA-256: `bafd3a4733ae13f8a5d423af2fecc9df3e4df9ebf2633dfcf5048b990201a078`. The frame resolves the expected multicolor paletted pattern instead of sampling palette-selector bits as VRAM address bits.

### palette/8bpp

```text
TA packets=120
vertices=48
triangles=24
tex-samples=3686400
bilinear=3686400
paletted=3686400
logical-frames=11
page-flips=11
```

Deterministic PPM SHA-256: `6dd2f1f999c3df69e5215ab13db3e4478273799b0579e090105cefb7a4312d14`.

### palette/wormhole

```text
TA packets=120
vertices=48
triangles=24
tex-samples=3686400
bilinear=3686400
paletted=3686400
logical-frames=11
page-flips=11
```

Deterministic PPM SHA-256: `f59e5f8b6c4d803b233342c13d73134d46749b3a4189c40d95cb3adde470ffaa`. The frame visibly resolves the grayscale spiral/wormhole palette effect.

### plasma

```text
TA packets=120
vertices=48
triangles=24
tex-samples=3686400
bilinear=3686400
paletted=0
logical-frames=11
page-flips=11
```

Deterministic PPM SHA-256: `efe39b5a8f781d37fad849f412a123c6715355c2ede4806913e239e8463f1bf4`. The same texture path that was stable in 0.0.37 now uses the requested bilinear filter.

### bumpmap

A deterministic A-then-Start path still exits cleanly:

```text
TA packets=20
vertices=16
triangles=8
sprites=4
logical-frames=1
page-flips=1
exit=0
```

Exact PVR BUMP lighting remains deliberately unclaimed; VQ/sprite/base-texture behavior from 0.0.37 remains intact.

### texture_render

```text
6 frames
rtt=3
logical-frames=5
vblanks=2
page-flips=2
exit=0
```

RTT passes still stay off-screen and are reused as texture input. No regression to the flicker fix was observed in deterministic output.

### pvrmark

```text
TA packets=20020
vertices=20013
logical-frames=2
page-flips=2
```

PVRMark remains a TA/frontend stress regression. Exact raster/depth/culling fidelity is not yet claimed.

## 2ndMix general regression

Final five-second native ARM/AICA capture:

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

It is byte-identical to the stable 0.0.34-0.0.37 baseline.
