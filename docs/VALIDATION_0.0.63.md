# DreamcastRecomp 0.0.63 validation

- Supplied ChuChu Rocket! CDI commercial closure: 2,776 functions / 236,519 known SH-4 instructions / 0 unknown / `RAW_SH4=0`.
- Required targets present in the function map: `0x8C02DC50`, `0x8C04FFC0`, `0x8C0278CC`, `0x8C0E23E0`.
- The unsafe one-hop veneer `0x8C035090` is deliberately not auto-promoted by uncertain-gap recovery.
- Main project regression: 47/47 CTest PASS.
- Final 2,776-function generated commercial runner: Clang 17 Debug (`-O0 -g0`) compile + link PASS, including `generated_program.cpp` and `dreamcast_program`.
- PVR/VMU/Maple/AICA/timing implementation is unchanged from 0.0.62.
