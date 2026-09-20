# Validation 0.0.75

- CMake/CTest: **47/47 PASS**.
- Fresh ChuChu Rocket! commercial generation from the CDI: **2,884 functions / 263,789 known SH-4 instructions / 0 unknown / `RAW_SH4=0`**.
- `Strided callback targets: 32`; runtime target `0x8C1317B0` is included structurally with no title-address seed.
- The unseeded strided-record detector reproduces exactly the clean closure obtained by the diagnostic experiment that seeded all 32 callbacks.
- Final generated commercial `generated_program.cpp` (about 47 MiB) compiles with Clang 17 at `-O0`; `dreamcast_program` and `generated_compile_test` both link successfully, and `generated_compile_test` runs successfully.
- Final generated-runtime PVR smoke: **PASS**.
  - Direct RGB565 mipmapped 8x8 texture decodes the largest level after the 48-byte lower-level prefix.
  - VQ mipmapped 8x8 texture keeps the codebook at `TexAddr` and starts the largest index plane at `2048 + 6` bytes.
  - Mipmapped texture dimensions are square (`TexV` ignored).
  - Perspective-correct UV interpolation selects the expected texel in a triangle where affine interpolation intentionally selects the wrong one.
- Heartbeat diagnostics added: `pvr-mip=triangles/samples/decodes`, `pvr-persp=triangles`.
- `build_windows.bat` is a required package file and must be present in the final ZIP.
