# DreamcastRecomp 0.0.81 — 3D pipeline validation

## Why this build exists

ChuChu Rocket! currently shows its 2D UI correctly while the 3D board, stage preview, star-textured rotating background object and most 3D actors are absent. 0.0.80 also introduced colored fills in regions that were previously black, so 0.0.81 returns to the 0.0.79 PVR/VRAM behavior before changing anything else.

During the 0.0.81 audit a concrete SH-4 bug was found: generated FTRV read `ctx.xf_bits[k]` directly. The runtime already models FPSCR.FR correctly for architectural FR/XF access, so this bypassed the bank swap whenever FR=1. FTRV now uses `dc_get_xf_bits()`.

The ChuChu commercial closure contains 164 static FTRV sites and 24 FRCHG sites. Runtime frequency is reported in `fpu3d`.

## Heartbeat fields

`fpu3d=FTRV/FTRV_FR1/FIPR/FMAC/FSCA/FSRRA/FRCHG`

`pvr-3d=TRI/ONSCREEN/OFFSCREEN/COVERED/DEPTH_PASS/DEPTH_FAIL/WRITTEN`

`pvr-3dz=MIN,MAX`

`pvr-3dzm=NEVER,LESS,EQUAL,LEQUAL,GREATER,NOTEQUAL,GEQUAL,ALWAYS`

`pvr-3dprobe=BDWO/BG_SKIPS`

B = background plane suppressed  
D = depth forced to pass for detected 3D  
W = detected 3D forced to opaque white with texture/shading/blend bypassed  
O = only detected 3D triangles are rasterized

"3D" is a diagnostic heuristic: a non-background triangle with varying submitted inverse-W across its vertices.

## Test order

### 0. Normal

`run_commercial_recompiled.bat "ChuChu Rocket!.cdi"`

This is the most important first run because it contains the FTRV bank fix.

If the 3D appears here, capture `fpu3d`; a non-zero second value (`FTRV_FR1`) confirms that old builds executed FTRV while the register banks were swapped.

### 1. No background

`run_commercial_recompiled_no_background_probe.bat "ChuChu Rocket!.cdi"`

Only the PVR background plane is omitted. If the board/cube/actors appear, investigate background depth/composition.

Expected heartbeat marker: `pvr-3dprobe=B---/...`

### 2. 3D depth always

`run_commercial_recompiled_3d_depth_probe.bat "ChuChu Rocket!.cdi"`

All normal shading/textures remain active; only the depth test for detected 3D is forced to pass.

If this restores 3D, investigate inverse-W depth comparison/write ordering.

Expected marker: `pvr-3dprobe=-D--/...`

### 3. 3D white

`run_commercial_recompiled_3d_white_probe.bat "ChuChu Rocket!.cdi"`

Detected 3D keeps its normal depth test but bypasses texture, vertex shading and blending and writes opaque white.

If this restores shapes while the depth probe does not, investigate shading/texture/alpha/blend.

Expected marker: `pvr-3dprobe=--W-/...`

### 4. 3D isolate

`run_commercial_recompiled_3d_isolate_probe.bat "ChuChu Rocket!.cdi"`

This is the decisive raster probe: background off, depth always pass, 3D white, 2D omitted.

If meaningful board/cube/actor geometry appears, XYZ reaches the rasterizer and the fault is in a later PVR stage. If it remains empty while `pvr-3d` has onscreen triangles/pixels, inspect the raster classification. If `pvr-3d` itself is near zero/offscreen, inspect SH-4 transformation and TA geometry.

Expected marker: `pvr-3dprobe=BDWO/...`

## Culling note

The current software PVR stores and reports CullMode but does not actually reject triangles using CullMode. Therefore culling cannot currently be the cause of the missing 3D in this runtime, and a NO-CULL probe would not test anything.
