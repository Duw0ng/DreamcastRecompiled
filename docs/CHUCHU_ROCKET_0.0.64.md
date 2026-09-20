# ChuChu Rocket! checkpoint — 0.0.64

The accepted 0.0.63 path reaches the real Mode Select menu with persistent VMU storage, but large rectangular areas around menu artwork render as black instead of transparent.

## Root cause

Retail tracing shows the affected art is not missing: ARGB4444 atlases contain many alpha-zero texels and are submitted on translucent list 2 with `SRCALPHA/INVSRCALPHA` and MODULATEALPHA. Representative foreground layers use inverse-Z values around 0.286-0.333, while the full-screen RGB565 background tiles are farther at about 0.25 and are submitted later in the same list.

The old software renderer rasterized triangles immediately as TA packets arrived. A nearer transparent texel could therefore leave the framebuffer clear color unchanged but still write its larger inverse-Z. When the farther background arrived later, the depth test rejected it and the host clear color remained visible as a black rectangle.

## 0.0.64 behavior

Translucent-list triangles are stored with a snapshot of their PVR state and flushed at the scene boundary. They are stable-sorted by average inverse-Z from far to near; equal-depth triangles retain submission order. Opaque and punch-through paths remain immediate. RTT scenes flush before storing the software render target to VRAM.

The Mode2 vertex-alpha enable bit is also honored. Heartbeats expose `pvr-tsort=queued/flushes/peak`.

No commercial code-discovery changes are included in this release.
