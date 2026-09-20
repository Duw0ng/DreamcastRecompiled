# Windows / MSBuild FTK1011 short-build-path fix

The generated commercial C++ build directory is now `_cb` at the project root
instead of `generated\commercial_recompiled\cpp\build`.

Why: Visual Studio/MSBuild FileTracker may fail with `FTK1011` while creating
`.tlog` files when its CMake scratch path grows beyond the practical Windows path
limit. The user-reported failing path was 264 characters long.

The generated source remains in:
`generated\commercial_recompiled\cpp`

The executable is now:
`_cb\Release\dreamcast_program.exe`

All commercial runner/probe BAT files were updated to use the new executable path.

For a completely fresh rebuild:
`set DCR_CLEAN_RECOMPILE=1`
then run the normal commercial recompile BAT. This removes both
`generated\commercial_recompiled` and `_cb`.
