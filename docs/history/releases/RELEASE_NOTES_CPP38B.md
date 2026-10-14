# DreamcastRecompiled v1.2 cpp38b

## PVR GPU fast path Phase 2

- Directly builds the per-frame R32 index stream inside the DISCARD-mapped persistent D3D11 index buffer.
- Removes cpp38a's full `index_scratch -> D3D11 buffer` copy.
- Keeps late-strip materialization, opaque submission order, translucent sort order, run merging and R32 strip cuts unchanged.
- Retains cpp38a SQ one-copy capture, CH2 bulk texture/TA transfers and SIMD Type-7/8 strip scanning.

## Historical FPS recovery

- Restores the proven 262,144-cycle host/AICA sync cadence used by the 0.0.161-era high-FPS ChuChu builds.
- `DCR_HOST_SYNC_QUANTUM=524288` remains available for direct A/B testing.
- No SH-4 guest clock, PVR/SPG timing or ABI layout is changed.

## Compatibility

- Guest ABI1 fingerprint remains frozen.
- ChuChu SAFE guest DLL remains unchanged.
- GD-ROM cpp37c, Maple cpp36b1g and Network cpp34s are preserved.
- Application title: `DreamcastRecompiled - <GAME> [cpp38b]`.
- Portable launcher v1.2.
