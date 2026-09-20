# DreamcastRecomp 0.0.184 validation

## Evidence from the supplied 0.0.183 run

- Visual correctness is restored: the user reports complete character geometry and successful progression back into gameplay.
- D3D11 itself is not intrinsically slow: accepted sections of the run reach roughly the previous high-FPS range.
- In the heavy path, successful GPU frames stop at 1,202 while total rendered frames continue to 2,101. The remaining 899 frames are MT software fallbacks and are copied correctly to DXGI by the 0.0.183 hybrid scanout fix.
- Source comparison proves 0.0.183 could reject a complete GPU frame solely because a non-background PVR vertex carried inverse-W outside `[0,1]`, including both ordinary triangles and staged/native strips.

## 0.0.184 change

1. Preserve the original PVR inverse-W in every GPU vertex for perspective-correct attributes.
2. Add a separate `depth_z` host coordinate.
3. Compute the finite non-background inverse-W range referenced by each frame.
4. Map that range affinely into the D3D11 depth interval.
5. Remove the old out-of-unit inverse-W frame-rejection checks from triangle and strip GPU eligibility.
6. Keep genuine unsupported texture/state fallback and the 0.0.183 CPU->DXGI scanout path unchanged.
7. Add `pvr-gdepth=FRAMES/VERTEX_REFS` telemetry.

A positive affine mapping is monotonic and commutes with linear interpolation, so the guest depth ordering/comparison relationship is retained while meeting D3D11's depth-coordinate requirements.

## Validation performed

- General project configure/build: PASS.
- General CTest: **51/51 PASS**.
- Full generated CT2 project, Clang Debug/O0: build/link PASS.
- Generated CT2 compile-test: **RC=0**.

## Windows acceptance target

A successful live test should show `pvr-gdepth` increasing in character-select/gameplay while `pvr-gpu` successful frames continue increasing beyond the old 1,202 plateau. `pvr-gcpu-scanout` should stop increasing on nearly every frame. Any remaining rise in GPU fallback after the depth-remap path is active points to a genuine unsupported texture/state path rather than inverse-W range.
