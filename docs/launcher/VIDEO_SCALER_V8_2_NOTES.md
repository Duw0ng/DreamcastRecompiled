# Dreamcast Recompiled v1.2 — Video V8.2: blank screen repair

This package starts from **V8.1 BIBLIOTECA VISIBLE**. The original cpp38e
SH-4 guest ABI1 and HOT_BLOCK_RECOVERY code/cache remain intact.

## Observed problem in the four Windows logs

- 1080p (first run): DXGI swapchain reports **1x1** despite launcher selecting
  1920x1080; it is created before the Win32 client rectangle is initialized.
- 1080p (second run): chain reports 1920x1080 but contents are blank/incorrect.
- 960p: chain reports 1280x960 but contents are blank/blue.
- Native: DXGI 640x480 and the game appears normally.
- Logs continue showing valid PVR rendering/flips and direct DXGI `Present` calls.

## Changes

1. New independent, tested `runtime/include/dcr_video_output_extent.hpp`
   chooses the requested or previous valid size whenever `GetClientRect` is
   transiently 0x0/1x1; the chain is recreated only when a real size changes.
2. Custom output uses a dedicated `D3D11_RASTERIZER_DESC` with
   `D3D11_CULL_NONE`, `ScissorEnable=FALSE` and `FillMode=SOLID` for the
   host fullscreen triangle; no inherited Dreamcast culling is allowed.
3. Optional compatibility output via GDI: **Settings > Video > Salida grafica
   > gdi**. The guest and GPU still render at native resolution, then a GPU
   readback and Win32 `StretchDIBits` displays the resulting frame at the
   selected host size. GDI is slower; use it to diagnose or work around any
   driver-specific DX11 scaler issue.
4. The DirectX path still uses `Present(0,0)`. VSync has **not** been changed.
   The 16:9 mode is stretched presentation, not a wider in-game field of view.
5. The launcher's library fix and all its contents remain in place.

## How to test

Extract V8.2 separately, open `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`,
select ChuChu Rocket! and then Settings > Video:

1. **native, 4:3, dx11** — reference; should match cpp38e.
2. **960p, 4:3, dx11** — inspect entire image, including gameplay graphics.
3. **1080p, 4:3, dx11** — look for black sidebars and unchanged picture aspect.
4. **1080p, 16:9, dx11** — stretched image, no widescreen camera/FOV.
5. If modes 2–4 still show white/blue: **960p, 4:3, gdi**; compare its
   display. GDI may be slower. If that works, the bug is restricted to the
   custom DX11 output pass; provide the updated session log.

The Windows runtime must be compiled with this package's **new runtime source**.
The archived portable .exe itself has not been rebuilt under Windows here.
The log should include `video-scaler=v8.2` for customized DX11 output.

## Verification limits

Static and host-only scaling math checks are validated on Linux. C++ runtime
build and the launcher test suite pass. The DX11 Win32 shader cannot be
executed in this Linux container, and this patch is NOT claimed to be
proven on an RX580 without a new Windows test.
