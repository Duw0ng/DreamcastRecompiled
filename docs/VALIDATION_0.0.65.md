# Validation — 0.0.65

- Fresh project CTest: 47/47 PASS.
- Supplied ChuChu Rocket! CDI static closure remains 2,776 functions / 236,519 known SH-4 instructions / 0 unknown / `RAW_SH4=0`.
- Generated commercial `dreamcast_program` compiles and links with Clang 17 in the portable validation environment.
- Direct runtime font smoke reads one narrow glyph and one wide JIS glyph through `dc_read8()` at the retail FONTROM aperture; both produce non-zero 1-bpp data and increment synthesis counters.
- Commercial-runtime smoke reads a wide slot from the same Japanese region exercised by the earlier retail font faults and receives non-zero packed glyph bytes.
- Generated CMake explicitly links `user32`, `gdi32`, and `winmm` on Windows; no external font file is part of the generated project.

Windows acceptance target: open ChuChu Rocket! Options and confirm the dynamic labels become visible while the 0.0.64 black-rectangle fix remains intact. Heartbeat should show non-zero `bfont-rom` synthesis activity.
