# DreamcastRecomp 0.0.82 — validation

## Goal

Determine whether ChuChu Rocket!'s missing board/3D actors are absent before rasterization or merely hidden by the normal PVR pipeline, and separate Store Queue from CH2 ownership without using the old varying-Z heuristic.

## Test order

1. `run_commercial_recompiled_wireframe_probe.bat "RUTA\ChuChu Rocket!.cdi"`
2. `run_commercial_recompiled_sq_only_probe.bat "RUTA\ChuChu Rocket!.cdi"`
3. `run_commercial_recompiled_ch2_only_probe.bat "RUTA\ChuChu Rocket!.cdi"`

Wire colors in the all-source probe: SQ cyan, CH2 magenta, direct CPU yellow, unknown white.

## Heartbeat fields

- `pvr-sqpt=type0..type7`
- `pvr-ch2pt=type0..type7`
- `pvr-surf=SQ,CH2,CPU,unknown`
- `pvr-vsrc=SQ,CH2,CPU,unknown`
- `pvr-trisrc=SQ,CH2,CPU,unknown`
- `pvr-onsrc=SQ,CH2,CPU,unknown`
- `pvr-long=sq:header64/vertex64/sprite64/modvol64,ch2:...`
- `pvr-objtop=sq@PC/count,ch2@PC/count`
- `pvr-wire=on/triangles/pixels`
- `pvr-srcprobe=0|1|2/skipped`

## Interpretation

- Board visible in all-source wireframe: geometry exists; continue with depth/shading/state.
- Board visible only in SQ-only: focus SQ state/framing and downstream PVR behavior.
- Board visible only in CH2-only: focus CH2/global-state interaction.
- Board absent in all three: geometry is lost/invalid before rasterization; use source histograms and hottest OBJECT LIST SET PC to attack the exact ingress/framing path.
