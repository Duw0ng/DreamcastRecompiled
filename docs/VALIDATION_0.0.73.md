# Validation 0.0.73

- Main CTest suite: 47/47 PASS.
- ChuChu commercial closure: 2,835 functions, 243,202 known SH-4 instructions, 0 unknown, RAW_SH4=0.
- Closure delta vs 0.0.72: +15 / -0 functions.
- `0x8C0195D4` is discovered without a manual seed.
- Generated runtime compiles after modifier-volume list separation.
- Synthetic TA test submits an opaque modifier-volume header plus a 64-byte vertex whose continuation deliberately resembles another polygon header; result: 0 color triangles, unchanged framebuffer, one modifier volume consumed.
- Required Windows package file: `build_windows.bat`.
