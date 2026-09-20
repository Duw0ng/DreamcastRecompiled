# DreamcastRecomp roadmap after 0.0.54

## 1. Cross START on Windows

The immediate goal is to confirm that `0x8C026F2E` executes and capture the next real missing target or hardware-semantic dependency. Do not add title-specific seeds unless structural discovery cannot represent the construct generically.

## 2. Performance: measure before changing timing

Use the 1-second heartbeat fields:

- `dispatch-cache=hits/misses` — generated/native call lookup efficiency.
- `host-aica-ms=` — accumulated host milliseconds spent executing ARM7 catch-up.
- `pvr-prof-ms=raster/present/clear` — accumulated PVR software-render/presentation/clear cost.
- `pvr-frames`, `pvr-vblanks`, `host-syncs`, `host-aica-drop` — guest progress and timing pressure.

Do not reduce ARM7 catch-up or alter guest PVR timing until these numbers identify the dominant host cost.

## 3. Idle-loop acceleration

`idle-skipped=0` remains a performance lead. Characterize repeated SH-4 polls of Holly/Maple/GD/TMU state and add event-aware fast-forward only where the wake condition is explicit and timing-safe.

## 4. Renderer correctness

Continue investigating the title-screen black texture rectangles from TA/TSP/TCW/texture state rather than using title-specific image/color fixes.
