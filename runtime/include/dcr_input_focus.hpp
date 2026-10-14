#pragma once

// cpp38e host-input focus guard. No guest ABI, SH-4, audio or PVR dependencies.
// The game accepts input only while its own PVR HWND is the Windows foreground
// window. Other windows in the same process (if any) are NOT equivalent.
#ifdef _WIN32
// This header can be included before ws2tcpip.h by the runtime. Without
// WIN32_LEAN_AND_MEAN, Windows.h pulls in legacy winsock.h first and MSVC
// fails with C2011/C2375 when winsock2.h is included later.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#endif

namespace dcrecomp_generated {

// Platform-neutral decision core for regression tests.
constexpr bool dcr_focus_allows_input(bool has_foreground_window,
                                      bool owned_by_this_process,
                                      bool is_game_window_class,
                                      bool minimized) noexcept {
    return has_foreground_window && owned_by_this_process &&
           is_game_window_class && !minimized;
}

inline bool dcr_game_window_has_focus() noexcept {
#ifdef _WIN32
    HWND foreground = GetForegroundWindow();
    if (!foreground) return false; // Never assume focus while a window is being created.
    DWORD foreground_pid = 0u;
    GetWindowThreadProcessId(foreground, &foreground_pid);
    if (foreground_pid != GetCurrentProcessId()) return false;
    char window_class[64]{};
    const int n = GetClassNameA(foreground, window_class, static_cast<int>(sizeof(window_class)));
    return dcr_focus_allows_input(true, true,
                                   n > 0 && std::strcmp(window_class, "DreamcastRecompPVRWindow") == 0,
                                   IsIconic(foreground) != FALSE);
#else
    // Headless/non-Windows builds have no Windows focus or host input polling.
    return true;
#endif
}

} // namespace dcrecomp_generated
