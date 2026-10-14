# cpp38a — PVR/TA Native Fast-Path Phase 1

cpp38a starts the PVR-first performance roadmap on top of the validated cpp37c GD-ROM checkpoint. It deliberately does **not** change Guest ABI1 or the SAFE game module.

## Hot-path changes

1. **One-copy SQ → TA capture**
   - The previous generic Store Queue submission copied each 32-byte packet into a local `PVRTAStreamRaw` and then copied/moved it again into the final staged vector.
   - cpp38a appends directly into the final staging slot: one 32-byte copy per ordinary SQ packet.

2. **CH2 main-RAM → TA bulk capture**
   - When CH2 DMA source is contiguous Dreamcast main RAM and the transfer is 32-byte aligned/granular, the full DMA block is copied directly into the contiguous TA staging array.
   - This replaces byte-by-byte `dc_read8()` traffic and per-packet temporary buffers.
   - Unsupported/partial cases retain the proven old path.

3. **CH2 main-RAM → VRAM bulk texture upload**
   - Contiguous texture DMA now uses `memcpy`, including VRAM wrap handling.
   - Non-main-RAM sources retain the old generic read path.

4. **SIMD Type-7/8 strip look-ahead**
   - Complete-strip detection scans 16 TA source tags at a time with SSE2 on x86/x64.
   - It stops on EndOfStrip or a non-vertex parameter exactly like the scalar path.

5. **Homogeneous ingress strip fast path**
   - The same SIMD scan detects strips whose vertices all came from one ingress source (normally SQ or CH2).
   - Direct Type-7/8 decode then uses one constant source for the full strip instead of loading/updating source telemetry per vertex.

## Diagnostics

Normal heartbeat contains:

`pvr38=on/capture1copy/ch2bulk/simdtag/homstrip`

The first real CH2 bulk hits emit one-time markers:

- `[PVR38] CH2->TA bulk main-RAM fast-path active ...`
- `[PVR38] CH2->VRAM bulk texture fast-path active ...`

Existing `pvr-flystrip`, `pvr-latestrip`, `pvr-opbulk`, `pvr-pktsrc`, FPS and GPU counters remain available.

## Compatibility rules

- Guest ABI1 structural fingerprint remains `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`.
- ChuChu SAFE `dcr_game_module.dll` remains SHA-256 `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4`.
- GD-ROM remains cpp37c native media/command/DMA/CDDA.
- All new PVR paths have conservative fallback to the cpp37c behavior.

## Real performance test

For a clean FPS comparison, run normally (do not use `--perf`):

`run_game.bat "C:\ruta\ccr.cdi" --reuse`

Use the same Mouse Mania scene and approximately the same duration as cpp37c.

For diagnostic counters only:

`TEST_CPP38A_PVR_PROFILE.bat "C:\ruta\ccr.cdi"`
