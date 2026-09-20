# Validation 0.0.76

- CMake/CTest: **47/47 PASS**.
- Fresh ChuChu Rocket! commercial generation: **2,884 functions / 263,789 known SH-4 instructions / 0 unknown / `RAW_SH4=0`**; closure unchanged from 0.0.75.
- Corrected PVR `DESTCOLOR` / `INVDESTCOLOR` blending to evaluate the opposite framebuffer color per channel.
- Previous intensity offset face color now initializes to white, matching the TA parser reference behavior.
- Added `pvr-i7`, `pvr-bl23`, and `pvr-i7probe` telemetry.
- `DCR_PVR_TYPE7_UNLIT=1` commercial smoke starts correctly and reports `pvr-i7probe=on`.
- Final generated `generated_program.cpp` (~47 MiB) compiles with Clang 17 at `-O0`, and `dreamcast_program` links successfully.
- `build_windows.bat` and `run_commercial_recompiled_type7_probe.bat` are required package files.
