# Windows MSVC compilation fix: Winsock header order

## Symptom

`dc_runtime.cpp` fails with `C2011: sockaddr / sockaddr_in / fd_set redefinition`
and a cascade of 100+ `ws2tcpip.h` errors. The first warning says legacy
`winsock.h` had already been included before `winsock2.h`.

## Root cause

`dc_runtime.cpp` used to include `dcr_input_focus.hpp` near the top. That
new focus guard included `windows.h` **before** the runtime's later
`winsock2.h` and `ws2tcpip.h`. On the user's MSVC SDK, `windows.h` includes
legacy `winsock.h` unless `WIN32_LEAN_AND_MEAN` is defined first.

## Fix

- Include `winsock2.h` and `ws2tcpip.h` before `windows.h` and before the
  focus guard in `runtime/dc_runtime.cpp`.
- Make `dcr_input_focus.hpp` safe on its own, defining `WIN32_LEAN_AND_MEAN`
  and `NOMINMAX` before it includes `windows.h`.
- Add `WIN32_LEAN_AND_MEAN` as a compile definition for DreamcastRuntime.
- Bump only `DCR_RUNTIME_BUILD_ID`, forcing rebuilding the runtime DLLs through
  the existing SHA-based cache manifest. The SH-4 guest build ID and ABI1
  headers are deliberately unchanged.

## Testing

Compilation and launcher unit tests were performed on Linux. This is a
Windows-SDK-specific conflict, so validate the new runtime in Visual Studio
on Windows before considering this a fully verified MSVC fix.

The first boot after installing the fixed package rebuilds **only the
runtime and device plugins**, not the already compiled SH-4 game module.
Leave the existing per-game cache in `%LOCALAPPDATA%` untouched.
