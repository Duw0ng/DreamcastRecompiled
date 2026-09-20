# Validation 0.0.72

## Commercial closure

- Target observed in ChuChu 4P Battle: `0x8C0E5AD2`.
- Structural cause: literal function pointer is stored to an ops table in the architectural delay slot of a `BRA`.
- No title-specific seed is added.
- Closure: 2,820 functions / 242,110 known SH-4 instructions / 0 unknown / `RAW_SH4=0`.
- Added versus 0.0.71: `0x8C0E5AD2`, `0x8C0E5B62`, `0x8C0E5BF2`, `0x8C0E5CA8`, `0x8C0E5DEE`, `0x8C0F5E58`; removed: none.

## PVR gameplay depth

- Decode ISP/TSP `DepthMode` bits 29..31 and `CullMode` bits 27..28.
- Opaque/ordinary polygons honor their encoded depth comparison.
- Sorted translucent list uses GEQUAL and suppresses color-pass Z writes.
- Punch-through uses GEQUAL and forces Z writes.
- CullMode is telemetry-only in this release to isolate the depth change.
- Heartbeat exposes cumulative mode usage for the next retail gameplay trace.
