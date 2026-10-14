# Dreamcast Recompiled v1.2 — V8.4 DXGI compatibility fix

## Why this branch exists
User's RX 580 renders native 640x480 correctly and GDI renders 960p/1080p; V8.3's DX11 high-resolution fullscreen-triangle pass presents blank frames even with successful Present calls.

## Technical strategy
V8.4 *removes* the extra fullscreen shader from the presentation path. DXGI uses a 640x480 BGRA8 backbuffer and the original cpp38e `CopyResource + Present(0,0)` commands on every frame, even when the window is larger. The windowed blit-model swapchain uses `DXGI_MODE_SCALING_STRETCHED` to fit the client. This keeps all PVR geometry/texture work unchanged; it scales final output only.

- 16:9: larger client, stretched output (not genuine expanded FOV).
- 4:3: DXGI window client uses 4:3 geometry, e.g. 1440x1080 for the 1080p preset. This differs from the previous 1920x1080 window with black sidebars, because a native-sized DXGI backbuffer cannot itself draw bars without an additional presentation pass.
- GDI: retains previous aspect-preserving output with letterbox/pillarbox when applicable.
- VSync remains `Present(0,0)`.
- cpp38e guest ABI1 and `dcr_game_module.dll` do not change. Only the runtime rebuilds once, due to new build ID.

## Verification
Expected log contains `video-scaler=v8.4/dxgi-blit-native-copy` in DX11 custom and native modes. The DXGI swapchain's internal buffer remains 640x480, while the HWND's client area is larger. Windows native validation on the RX 580 is still required; Linux compile/tests cannot prove Windows compositing.
