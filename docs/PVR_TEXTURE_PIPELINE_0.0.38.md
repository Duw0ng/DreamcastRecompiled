# PVR texture pipeline — 0.0.38

0.0.38 generalizes the software PVR sampler so the project can move from individual homebrew fixes toward a first 2D commercial title.

## TCW/TSP fields now modeled

- 21-bit TCW `TexAddr` (8-byte units)
- `PalSelect`
- `StrideSel`
- `ScanOrder` / twiddled vs non-twiddled
- `PixelFmt`
- VQ flag
- TSP filter mode
- U/V clamp
- U/V mirror/flip
- 8x8..1024x1024 texture dimensions
- packed 16-bit TA U/V vertices

The old 25-bit texture-address mask was harmless for common RGB textures because the reserved bits were normally zero, but it is incorrect for paletted TCWs because bits 21..26 are palette selection. 0.0.38 explicitly separates these fields.

## Paletted formats

4bpp and 8bpp textures read indices from twiddled VRAM and resolve them through the 1024-entry PVR palette table. 4bpp selects one of 64 16-color banks; 8bpp selects one of four 256-color banks. Palette entries respect the global palette format register.

## Filtering

Nearest filtering remains mode 0. Mode 1 performs bilinear interpolation in ARGB space. Modes 2/3 currently use the same base-level bilinear interpolation; real mip selection and the two-pass Dreamcast trilinear behavior remain future work.

## Addressing

- twiddled square textures use Morton ordering;
- rectangular twiddled textures are addressed as min-dimension square tiles, matching KOS's loader layout;
- non-twiddled textures use linear rows;
- stride-selected rows use `PVR_TEXTURE_MODULO * 32` pixels.

## Still missing

- exact BUMP lighting equation;
- mipmap level layout/selection and two-pass trilinear blending;
- full depth/culling behavior;
- modifier volumes/cheap shadows;
- YUV422 texture sampling/converter;
- complete floating/intensity/two-volume TA vertex formats.

Those gaps are kept explicit rather than substituted with title-specific behavior.
