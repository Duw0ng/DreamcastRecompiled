# Validation — cpp36b1c

- Runtime + `dcr_network` + `dcr_maple` standalone CMake build: PASS on the container toolchain.
- Guest ABI1 fingerprint current vs frozen: MATCH.
- Safe ChuChu guest SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4`.
- `VERIFY_CPP36B.py`: PASS.
- `VERIFY_CPP36B1.py`: PASS.
- `VERIFY_CPP36B1A_SAFE_SEED.py`: PASS.
- `VERIFY_CPP36B1C.py`: PASS.

The reported Windows failure was a C++ type-deduction error: Win32 `POINT` members use `LONG`, while the clamp bounds were `int`. cpp36b1c casts the point coordinates to `int` before calculating/clamping mouse deltas.
