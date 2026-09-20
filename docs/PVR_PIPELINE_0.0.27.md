# PVR frame lifecycle — DreamcastRecomp 0.0.27

0.0.27 separates the software PowerVR path into three buffers and explicit lifecycle events.

```text
TA packets -> registration framebuffer -> render buffer -> VBlank -> present buffer
```

## State

`pvr_framebuffer` is the scene currently being built from TA packets. Closing a scene swaps it into `pvr_render_buffer` and marks a render pending. A render start marks the software render busy/completed state. A virtual VBlank then swaps the completed render buffer into `pvr_present_buffer`.

Counters exposed by `dc_pvr_print_stats`:

```text
pvr_logical_frames
pvr_frames
pvr_guest_render_starts
pvr_heuristic_render_starts
pvr_render_completes
pvr_vblanks
pvr_page_flips
```

## Guest boundary vs probe fallback

A guest write to PVR register offset `0x0014` (`PVR_ISP_START`) is authoritative when it occurs. The older TA list-order heuristic remains as a fallback because packet-limited probes may terminate before the game reaches its normal render-start register write.

The software rasterizer still draws synchronously while TA packets are submitted, so `render_busy -> render_completed` currently happens without modeled PowerVR latency. The lifecycle is nevertheless separated now so later interrupt/timing work has explicit state to attach to instead of requiring another framebuffer architecture change.
