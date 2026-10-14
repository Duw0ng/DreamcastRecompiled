# Renderer backend selection — cpp19

## Implemented now

- `auto` — compatibility defaults per title profile.
- `dx11` — current Direct3D 11 GPU PVR renderer (`--pvr-gpu --pvr-render-done-scheduled --pvr-mt`).
- `software` — CPU/software PVR renderer (`--pvr-mt`).

Accepted DX11 aliases: `d3d11`, `direct3d11`, `directx11`, and `--gpu`.

## Reserved backends

The launcher recognizes the names below but intentionally stops with a not-implemented error until a real backend exists:

- `vulkan` / `vk`
- `dx12` / `d3d12` / `direct3d12` / `directx12`
- `dx10` / `d3d10` / `direct3d10` / `directx10`
- `dx9` / `d3d9` / `direct3d9` / `directx9`

This keeps the public CLI stable while renderer implementations are added later.

## Backend direction

The current GPU path is D3D11-specific in the generated runtime. Future renderers should sit behind the same PVR-facing operations (device/window creation, texture upload, render-target setup, draw submission, present, and optional readback) instead of branching PVR emulation logic per API. DX10 can likely share most shader/resource concepts with DX11 but needs a separate device/context implementation. DX9 requires a more distinct resource/shader/state path. Vulkan also requires explicit synchronization, command buffers, descriptor/resource management, and swapchain handling.

No reserved backend silently falls back to DX11; this is deliberate so renderer testing remains trustworthy.
