# DreamcastRecomp 0.0.87 validation

## Why this release targets SH-4/runtime instead of PVR

The supplied 0.0.86 gameplay heartbeats reported 10.0 FPS with `pvr-mt=on/8/1377/12670` followed by `on/8/1387/12819`. Ten host frames therefore added 149 ms of multithread raster time (~14.9 ms/frame), while wall frame time was ~100 ms. PVR MT is working; most of the remaining frame budget is outside the rasterizer.

## Changes validated

- 256-cycle optional SH-4 runtime-tick batching with exact accumulated cycle accounting.
- Fast cache-hit generic dispatcher.
- Program-wide direct C++ calls for resolved cross-function CALL and tail-BRANCH edges.
- Legacy fallback remains for dynamic, relocated, traced, scene-observed, or unresolved targets.
- New `sh4tick`, `dispatch-fast`, and `direct` heartbeat telemetry.

## Regression

- 47/47 CTest PASS.
- ChuChu closure: 2,965 functions, 295,215 known instructions, 0 unknown, RAW_SH4=0.
- Fresh commercial generation completed successfully.
- Generated commercial `generated_program.cpp` passed Clang C++20 syntax validation after direct-dispatch emission.
- Generated runtime/ARM7/image/native-overrides/runner translation units passed Clang C++20 syntax validation.

The Windows acceptance target is gameplay FPS plus `pvr-mt`, `dispatch-fast`, `direct`, and `sh4tick` deltas.
