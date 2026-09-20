# Validation 0.0.190

## Goal
Remove the in-game whole-frame CPU raster fallback caused by positive Dreamcast inverse-W values above 1.0, without restoring the per-frame affine depth normalization that regressed 2D rendering in 0.0.184.

## Change
- D3D11 pixel shader now writes `SV_Depth`.
- The original linearly interpolated Dreamcast inverse-W is mapped with `z = invW / (1 + invW)` for nonnegative finite depth.
- The mapping is stable and strictly monotonic, so guest depth ordering/comparison direction is preserved.
- Positive inverse-W values above 1.0 are no longer a reason for whole-frame GPU fallback.
- Negative/non-finite depth and unsupported texture state remain conservative fallback cases.
- 0.0.189 STARTRENDER completion behavior is retained.
