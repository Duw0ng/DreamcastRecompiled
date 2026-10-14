# Linux portability status — 2026-10-13

Audit based on **actual** GCC 14.2, CMake 3.31, Ninja 1.12 on an x86_64 Linux container. These are native builds, not Wine.

## Verified today

```
cmake -S . -B build/linux-core -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux-core --target dc_recomp dc_analyzer -j 3

cmake -S runtime -B build/linux-runtime -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux-runtime -j 2
ctest --test-dir build/linux-runtime --output-on-failure
```

- `dc_recomp` + `dc_analyzer`: **compiled**.
- `libDreamcastRuntime.so`: **compiled**.
- `dcr_maple.so`, `dcr_gdrom.so`, `dcr_network.so`: **compiled**.
- Runtime smoke tests: **2/2 passed** (`dcr_portable_ipbin_stage_tests`, `dcr_sh4_interp_tests`).
- `launcher/` Python/Tk code imports on Linux, but the Play function explicitly refuses to launch games because the production runner is Windows `.bat` and video/input backends remain Windows-specific.

## Remaining tasks before a Linux game can run

1. A platform-agnostic disc runner (e.g. Python CLI or `scripts/linux/run_game.sh`) replacing CMD/PowerShell and Win32/Windows CMake generator assumptions.
2. Linux native AOT modules (`.so`) with ABI-compatible generated code (the shipped safe guest DLLs are for Windows only). Preserve seed cache identities across OS without attempting to load Windows guest DLLs in Linux.
3. Window/event/audio/input abstraction: the current D3D11/DXGI/XInput and Windows focus code must get Vulkan/OpenGL (or initially SDL2 software), SDL2/evdev controllers, and appropriate Linux audio.
4. Verify software rendering path actually presents in a Linux window. A runtime `.so` compiling doesn't imply gameplay works.
5. Cache, saves, VMU sidecar, launcher path and online networking tests on Linux; distinguish Linux-native profile/cache from Windows cache.
6. CI running at least one legally distributable open test ROM/ELF to boot through render/audio/input, and continuous performance/correctness regressions.

## Recommended staging

- **L0 (achieved):** native Linux core + runtime/plugins build, native runtime unit tests.
- **L1:** standalone Linux-compatible CLI analysis and AOT generation.
- **L2:** Linux executable + a first visible frame, software video and keyboard.
- **L3:** playable Linux version with Vulkan/OpenGL, SDL2 controllers, AICA audio, VMU.
- **L4:** online/DCNet and multi-instance Linux parity, portable Linux/AppImage/Flatpak packaging.

Do not claim Linux full support just because compilation succeeds. A Linux artifact produced at L0 is a developer tool, not a playable release.
