# DreamcastRecomp 0.0.183 validation

## Runtime evidence from 0.0.182 GPU test

- D3D11/DXGI acceleration is active and reaches roughly 35–62 FPS in parts of the captured CT2 run.
- At the problematic transition the number of successful GPU frames stops at 984 while total renders/page flips continue increasing and GPU fallback frames keep rising. This proves guest execution continues while direct DXGI presents a stale GPU scanout.
- User also reports missing triangles in character models on GPU while the software renderer is visually correct.

## Fixes

1. If a completed guest page flip has no pending GPU snapshot, upload the CPU/MT `pvr_present_buffer` to the existing D3D11 scanout texture with `UpdateSubresource`.
2. Add `pvr-gcpu-scanout=` telemetry.
3. Keep all four CullMode state slots but use `D3D11_CULL_NONE` for all of them until Dreamcast TA strip winding/culling has an explicit validated mapping.
4. Apply both changes to the generated CT2 runtime and to the general C++ emitter.

## Validation

- Generated CT2 project: build/link PASS.
- `generated_compile_test`: RC=0.
- Generated runner: build/link PASS.
- General project build: PASS.
- General CTest: 51/51 PASS.

Windows/D3D11 visual acceptance remains the user test: character geometry should no longer lose faces, and once GPU fallback begins `pvr-gcpu-scanout` should rise while the displayed image continues past NOW LOADING.
