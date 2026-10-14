# DreamcastRecompiled v1.2 cpp36b1c — MSVC Maple Input Fix

This is a Windows/MSVC compatibility hotfix on top of cpp36b1b.

## Fixed

- `dcr_maple.cpp` no longer feeds Win32 `POINT.x/POINT.y` (`LONG`) directly into `std::clamp` with `int` bounds.
- Mouse coordinates are normalized to `int` first, so MSVC 14.51 can deduce `std::clamp<int>` unambiguously.
- `NOMINMAX` and `WIN32_LEAN_AND_MEAN` defines are now guarded to avoid C4005 macro redefinition warnings when CMake/MSVC already supplies them.
- Runtime build id is bumped so the runtime/plugin build is refreshed once.

## Unchanged

- Guest ABI1 layout/fingerprint.
- Safe ChuChu portable guest DLL and its SHA-256.
- Auto-portable flow and safe-seed adoption.
- No SH-4 guest rebuild is required for this hotfix.
