# PVR performance — DreamcastRecomp 0.0.27

0.0.27 profiles and fixes the first major live-preview performance bottlenecks found with real `2ndmix.elf`.

## Why 0.0.25 could look slow

The largest deterministic problem was host frame pacing. The old live path rendered a guest frame, blitted it to the Win32 window, **then slept a full 16 ms**. Render/present time was therefore added on top of the nominal 16 ms frame period.

Example: a host frame that needed 10 ms of work became roughly 26 ms after the unconditional sleep, or about 38 FPS before any other Windows/GDI overhead.

Other costs were:

- CPU software rasterization of triangle bounding boxes, barycentric weights, texture sampling, shading, blending and depth;
- synchronous Win32/GDI `StretchDIBits`, particularly when scaling 640x480 to 1280x960;
- a full 640x480 ARGB copy when rotating logical front/back buffers;
- PPM file output from deterministic graphics probes;
- bit-by-bit Morton addressing for twiddled textures;
- no interprocedural optimization between generated SH-4 code and the runtime.

The SH-4 recompilation itself was not the dominant cost in the current 2ndMix workload.

## 0.0.27 changes

- Deadline-based 60 Hz pacing: sleep only for the **remaining** time until the next absolute frame deadline.
- No extra sleep when the renderer is already late.
- Front/back framebuffer **swap** instead of copying 307,200 ARGB pixels every logical frame.
- PPM output only for explicit/final deterministic captures, not every logical frame.
- Incremental triangle edge stepping in the software rasterizer.
- Integer fast paths for common color modulation and SRCALPHA/INVSRCALPHA blending.
- Faster Morton/twiddled address expansion.
- CMake IPO/LTO in Release builds when the compiler supports it (MSVC LTCG / GCC LTO).
- `--pvr-profile` and `run_homebrew_2ndmix_perf.bat` for reproducible headless profiling.
- `--pvr-window-fps=N`; use 60 for normal live playback or 0 for an unthrottled diagnostic.

## Current real 2ndMix profile

A real 35,000-TA-packet run, 11 detected logical frames, without a window or frame file I/O:

```text
TA packets=35000
vertices=34922
triangles=11819
logical-frames=11

[PVR profile]
total=38.686 ms
raster=6.736 ms
present=0.000 ms
clear=1.608 ms
avg-frame=3.517 ms
unthrottled=284.338 fps
```

These numbers are from the current Linux build environment and are **not a promise of 284 FPS on Windows**. They show that the guest + software-PVR path itself has substantial headroom above 60 FPS. Windows live performance still includes GDI presentation and depends on the host machine.

## Windows diagnosis

Run:

```bat
run_homebrew_2ndmix_perf.bat
```

This removes the window and disk frame output. If this reports well above 60 FPS but the live window is slow, the remaining bottleneck is host presentation/GDI rather than SH-4 execution.

For a quick comparison, edit/run the generated program with `--pvr-window-scale=1`. If scale 1 is materially faster than scale 2, `StretchDIBits` scaling is a meaningful part of the host cost.
