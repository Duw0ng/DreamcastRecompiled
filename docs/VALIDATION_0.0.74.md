# Validation 0.0.74

- CMake/CTest: 47/47 PASS.
- ChuChu commercial closure: 2,840 functions / 243,414 known SH-4 / 0 unknown / `RAW_SH4=0`.
- Unseeded closure matches the clean diagnostic closure obtained by manually seeding runtime target `0x8C01958E`; the packaged analyzer does not use that title-specific seed.
- Generated runtime source compiles; oversized commercial `generated_program.cpp` was validated with Clang 17 at `-O0` within host limits.
- Dedicated generated-runtime PVR smoke validates skewed TA sprite fourth-vertex plane reconstruction and textured offset/specular RGB addition.
- `build_windows.bat` is a required package file.
