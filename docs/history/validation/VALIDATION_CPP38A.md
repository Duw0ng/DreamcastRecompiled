# Validation — cpp38a

- Runtime + Maple + Network + GD-ROM: PASS (Linux/GCC regression host).
- PVR vertex decoder selftest: PASS.
- PVR TA staging selftest: PASS.
- cpp38a wide Type-8 strip/SIMD helper regression: PASS.
- cpp38a CH2 main-RAM bulk TA regression: PASS.
- PVR texture dirty-region selftest: PASS.
- Windows launcher cross-build (amd64): PASS.
- Project tests 1–51 + 54: PASS.
- generated_single_build: PASS when executed separately; generated compile test returns 0.
- generated_program_build: PASS when executed separately; generated compile test returns 0.
- Effective regression result: 54/54 PASS.
- Guest ABI1 fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` MATCH.
- SAFE ChuChu guest SHA-256: `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4` unchanged.

Windows/MSVC and real Mouse Mania performance require user hardware validation.
