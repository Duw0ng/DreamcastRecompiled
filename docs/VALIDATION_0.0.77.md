# DreamcastRecomp 0.0.77 — validation

## Purpose of this checkpoint

The 0.0.76 Windows 4P Battle run exposed two independent issues:

1. the native runner eventually stopped at `0x8C0EA39C -> 0x8C11E6C4` because that legitimate SH-4 target was absent from the static commercial closure;
2. the PVR window could keep rendering menus/HUD while the stage preview and board were absent, followed later by violet/magenta lines and very large black primitives.

0.0.77 fixes the first issue structurally and makes the second one much more deterministic to diagnose. The supplied commercial CDI is used only as a local validation input and is not included in the release package.

## Core regression

Fresh Linux build after the 0.0.77 changes:

```text
cmake --build build_linux_077 -j4
[100%] Built target dc_raw_recomp

ctest --test-dir build_linux_077 --output-on-failure -j4
100% tests passed, 0 tests failed out of 47
```

## Fresh ChuChu Rocket! commercial closure

The normal commercial pipeline was rerun from the user-supplied CDI:

```text
CDI
 -> dc_disc_probe
 -> IP.BIN + 1ST_READ.BIN + DCR_DISC_MAP_V2
 -> dc_boot_prepare
 -> BOOTSTRAP.BIN @ 0x8C008000
 -> dc_raw_recomp --entry=0x8C008300 --seed=0x8C010000
```

Fresh 0.0.77 result:

```text
Closure passes:         23
Synthetic entries:      2962
Reachable functions:    2962
Call-graph edges:       6394
External/unresolved:    468
Reachable instructions: 294906
Known SH-4:             294906
Unknown SH-4:           0
CFG blocks:             44136
DCIR ops:               301041
RAW_SH4:                0
Closure resolved calls: 129824
Closure unresolved:     6489
Scaled-index targets:   64
Rejected dirty fragments: 0
```

The emitted commercial C++ was also compiled and linked as a full Linux diagnostic runner with Clang 17 (`-std=c++20 -O0 -g0`). The 52 MiB `generated_program.cpp`, current `dc_runtime.cpp`, embedded image, native overrides and runner all compiled and linked successfully. A bounded smoke launch starts as:

```text
DreamcastRecomp 0.0.77 native runner
Reachable functions: 2962
Executing sub_8C008300 @ 0x8C008300u
```

and emits the new `pvr-clip`, `pvr-objset`, `pvr-badv` and `pvr-i7geom` heartbeat fields.

The generated map now contains:

```text
0x8C11E6C4,sub_8C11E6C4,468,468,0,54,23,16,0
```

and generated registration contains:

```cpp
runtime.register_target(0x8C11E6C4u, &recomp_8C11E6C4);
```

No ChuChu-specific target seed was added.

## Why `0x8C11E6C4` was previously invisible

The caller is not a simple contiguous function-pointer table. A branch chooses a PC-relative field anchor, a signed selector is scaled with SH-4 shifts to a 32-byte record stride, an indexed `MOV.L` fetches the method pointer, and the loaded register is immediately called with `JSR @Rn`.

For the failing family, a negative selector can reach records before the chosen literal anchor. 0.0.77 therefore promotes a candidate family only when all of the following are structurally proven:

- local PC-relative table/field literal;
- selector stride recovered from `SHLL*` operations;
- indexed longword load;
- loaded value feeds an immediate `JSR/JMP @Rn`;
- callable targets form a conservative contiguous run;
- backward scanning is allowed only when `EXTS.W` proves a signed selector.

This rule recovers 64 scaled-index targets in the supplied commercial image and transitively increases closure by 78 functions versus 0.0.76.

## TA/PVR packet-size audit

The 0.0.76 packet-size suspicion was checked against established Dreamcast TA layouts. The existing vertex-size matrix was already correct and is deliberately retained:

```text
32-byte vertices: 0, 1, 2, 3, 4, 7, 8, 9, 10
64-byte vertices: 5, 6, 11, 12, 13, 14
```

In particular, type 7 is a single 32-byte parameter containing:

```text
PCW | X | Y | Z | U | V | BaseIntensity | OffsetIntensity
```

Therefore 0.0.77 does **not** introduce an unsafe speculative type-7 size change.

## USER TILE CLIP / control parameters

The audit did reveal a real omission: TA USER TILE CLIP and OBJECT LIST SET control packets were not explicitly modeled by the bootstrap software renderer.

0.0.77 now:

- consumes TA parameter type 1 as USER TILE CLIP;
- reads inclusive tile coordinates from dwords at offsets 16/20/24/28;
- converts tiles to 32x32 pixel regions;
- carries PCW User_Clip mode on each polygon state;
- snapshots the current USER TILE CLIP rectangle into each polygon state so deferred translucent triangles retain the rectangle active when they were registered;
- mode 2 renders only pixels inside the user rectangle;
- mode 3 renders pixels outside the user rectangle;
- consumes TA parameter type 2 as OBJECT LIST SET instead of interpreting it as geometry.

New heartbeat fields:

```text
pvr-clip=commands/mode0,mode1,mode2,mode3/xmin,ymin,xmax,ymax/rejected/missing
pvr-objset=N
```

## First impossible-vertex diagnostic

0.0.77 detects the first decoded vertex with non-finite coordinates/UVs or coordinates far outside a sane diagnostic envelope. It prints a single detailed record:

```text
[PVR BAD-VTX] first impossible vertex packet=...
[PVR BAD-VTX] raw32=...   or raw64=...
```

The report includes the last guest PREF source PC, TA type, list, clip mode, color format, texture/UV/volume state, decoded XYZ/UV, and the raw TA bytes. The bad vertex is rejected so a malformed packet cannot immediately turn into a giant full-screen triangle or line.

Heartbeat adds:

```text
pvr-badv=N
```

## Type-7 A/B probes

The existing `run_commercial_recompiled_type7_probe.bat` remains a color-only test: type 7/8 modulation becomes white while UV/texture/depth/geometry/blending remain active.

0.0.77 adds `run_commercial_recompiled_type7_geometry_probe.bat`:

```text
DCR_PVR_TYPE7_GEOMETRY=1
```

For type 7/8 this forces white modulation and disables texture sampling while preserving XYZ, list, depth, blend, triangle topology and USER TILE CLIP. Heartbeat reports:

```text
pvr-i7geom=on
```

Interpretation:

- board appears as solid geometry -> continue with TCW/texture/mipmap/sampling;
- board remains absent -> continue with geometry/clip/depth/TA-stream state;
- `[PVR BAD-VTX]` appears -> inspect the first corrupt parameter before changing later raster stages.

## Windows acceptance run

Use the normal runner first:

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

Enter the same 4P Battle stage and capture the heartbeat at the first incorrect visual frame. The old `0x8C11E6C4` native-target error should no longer be the stop.

If the board is still absent, run:

```bat
run_commercial_recompiled_type7_geometry_probe.bat "RUTA\ChuChu Rocket!.cdi"
```

The decisive fields for the next iteration are `pvr-clip`, `pvr-badv`, `pvr-vt`, `pvr-i7`, `pvr-i7probe`, and `pvr-i7geom`.
